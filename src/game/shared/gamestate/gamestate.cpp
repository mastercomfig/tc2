//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: HTTP/WS game state
//
//===========================================================================//

//#define ASIO_NO_EXCEPTIONS
#ifdef _WIN32
#define _WIN32_WINNT 0x0602
#endif
#define CROW_STATIC_DIRECTORY ""
#define CROW_STATIC_ENDPOINT "/<path>"

#include <crow.h>
#include <unordered_set>

#undef _WIN32_WINNT

#include "cbase.h"

#include "gamestate.h"

#include <codecvt>
#include <tier0/platform.h>
#include <tier3/tier3.h>

#ifdef TF_CLIENT_DLL
#include "tf_hud_mainmenuoverride.h"
#include "tf_matchmaking_dashboard.h"
#endif

#include "game/client/iviewport.h"
#include "materialsystem/materialsystem_config.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"

static CGameStateManager s_GameStateManager;
CGameStateManager* GetGameStateManager() { return &s_GameStateManager; }


struct CWebRpcEvent
{
	int64_t m_iEventId;
	std::string m_strEvent;
	std::string m_strParams;

	static bool LessFunc(CWebRpcEvent* const& lhs, CWebRpcEvent* const& rhs)
	{
		return lhs->m_iEventId > rhs->m_iEventId;
	}
};

struct CWebRpcReturn
{
	int64_t m_iRpcId;
	std::string m_strValue;

	static bool LessFunc(CWebRpcReturn* const& lhs, CWebRpcReturn* const& rhs)
	{
		return lhs->m_iRpcId > rhs->m_iRpcId;
	}
};

struct CWebRpcMessage
{
	int64_t m_iRpcId;
	std::string m_strMethod;
	std::string m_strParams;
	bool m_bPrivileged;

	static bool LessFunc(CWebRpcMessage* const& lhs, CWebRpcMessage* const& rhs)
	{
		return lhs->m_iRpcId > rhs->m_iRpcId;
	}
};

class CHTTPServerThread : public CThread
{
public:
	CHTTPServerThread() :
	m_IncomingReturns(0, 0, CWebRpcReturn::LessFunc),
	m_IncomingEvents(0, 0, CWebRpcEvent::LessFunc),
	m_OutgoingGameMsgs(0, 0, CWebRpcMessage::LessFunc),
	m_UnprivilegedMethods{
		"localize",
		"getcvar",
		"setcvar",
		"cmd",
		"getmodes",
		"getmode",
		"playsound",
		"getingame"
	},
	m_UnprivilegedEvents{
		"ingame",
		"gsi"
	}
	{
		SetName("GameStateHTTPThread");
	}

	int64_t GetClientId( crow::websocket::connection& conn )
	{
		return *static_cast<int64_t*>( conn.userdata() );
	}

	// Return 0 for success
	virtual int Run() OVERRIDE
	{
		Msg("Initializing game state HTTP system...\n");
		m_Crow = new crow::SimpleApp();
		crow::SimpleApp& app = *m_Crow;
		const char* pGameDir = CommandLine()->ParmValue("-game", CommandLine()->ParmValue( "-defaultgamedir", "hl2" ) );
		CFmtStr1024 pStaticDir("%s/%s", pGameDir, "loose/resource/html");
		crow::Blueprint ui_bp("ui", pStaticDir.Get());
		app.register_blueprint(ui_bp);
		CROW_BP_ROUTE(ui_bp, "/")([]
		{
			return "Hello UI!";
		});
		CROW_ROUTE(app, "/")([]
		{
			return "Hello world!";
		});
		CROW_WEBSOCKET_ROUTE(app, "/ws")
		.onaccept([&]( const crow::request& req, void** userdata) {
			{
				AUTO_LOCK( m_clientsMutex )
				int64_t clientId = m_iNextClientId++;
				*userdata = new int64_t(clientId);
				if ( m_iPrivilegedClientId == 0 )
				{
					m_iPrivilegedClientId = clientId;
				}
			}
			return true;
		})
		.onopen([&](crow::websocket::connection& conn) {
			{
				AUTO_LOCK(m_clientsMutex)
				m_connectedClients.push_back(&conn);
				Msg("Game state connection created.\n");
			}
		})
		.onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t status_code) {
			{
				// TODO: sessions
				AUTO_LOCK( m_clientsMutex )
				int64_t* userdata = static_cast<int64_t*>( conn.userdata() );
				std::size_t size = m_connectedClients.size();
				int64_t clientId = *userdata;
				if ( m_iPrivilegedClientId == clientId )
				{
					m_iPrivilegedClientId = 0;
				}
				m_SubscribedClientIds.erase( clientId );
				for ( std::size_t i = 0; i < size; i++ )
				{
					if ( GetClientId( *m_connectedClients[i] ) == clientId )
					{
						m_connectedClients[i] = m_connectedClients[size - 1];
						m_connectedClients.pop_back();
						break;
					}
				}
				delete userdata;
				if ( status_code == 1002 )
				{
					DebuggerBreakIfDebugging();
				}
				Msg("Game state connection closed (%d): %s\n", status_code, reason.c_str());
			}
		})
		.onmessage([&](crow::websocket::connection& conn, const std::string& in, bool is_binary) {
			if (is_binary)
			{
				return;
			}

			if (in.empty())
				return;

			if (in.length() > (1024ULL * 1024ULL))
				return;

			crow::json::rvalue data = crow::json::load(in);
			if (!data)
				return;

			if (!data.has("i"))
				return;

			if (data["i"].t() != crow::json::type::Number)
				return;

			const auto& id = data["i"].i();

			if (!data.has("m"))
				return;

			if (data["m"].t() != crow::json::type::String)
				return;

			const auto& command = data["m"].s();
			std::string strCommand = command;

			bool bIsPrivileged;

			{
				AUTO_LOCK( m_clientsMutex )
				if ( m_connectedClients.empty() )
				{
					// should never happen.
					Assert( 0 );
					DevMsg( "connected clients empty!\n" );
					m_connectedClients.push_back( &conn );
				}
				int64_t clientId = GetClientId( conn );
				if ( !V_strcmp(strCommand.c_str(), "disconnect") )
				{
					// immediately send back a message for closing
					crow::json::wvalue resp;
					resp["i"] = id;
					DevMsg( "ack: disconnect\n" );
					conn.send_text( resp.dump() );
					m_iPrivilegedClientId = 0;
					return;
				}
				if ( !V_strcmp( strCommand.c_str(), "subscribe" ) )
				{
					// immediately send back a message for subscribing
					crow::json::wvalue resp;
					resp["i"] = id;
					DevMsg( "ack: subscribe\n" );
					conn.send_text( resp.dump() );
					if ( m_iPrivilegedClientId != clientId )
					{
						m_SubscribedClientIds.insert( clientId );
					}
					GetGameStateManager()->InitSubscriptions();
					return;
				}
				if ( !V_strcmp( strCommand.c_str(), "unsubscribe" ) )
				{
					// immediately send back a message for unsubscribing
					crow::json::wvalue resp;
					resp["i"] = id;
					DevMsg( "ack: unsubscribe\n" );
					conn.send_text( resp.dump() );
					m_SubscribedClientIds.erase( clientId );
					if ( m_SubscribedClientIds.size() < 1 )
					{
						GetGameStateManager()->StopSubscriptions();
					}
					return;
				}
				bIsPrivileged = m_iPrivilegedClientId == clientId;
			}

			std::string params;
			if (data.has("p") && data["p"].t() == crow::json::type::String)
			{
				params = data["p"].s();
			}

			//DevMsg("%s\n", in.c_str());

			{
				AUTO_LOCK(m_OutgoingMutex)
				CWebRpcMessage* pMessage = new CWebRpcMessage;
				pMessage->m_iRpcId = id;
				pMessage->m_strMethod = command;
				pMessage->m_strParams = params;
				pMessage->m_bPrivileged = bIsPrivileged;
				m_OutgoingGameMsgs.Insert(pMessage);
			}
		});
		Msg("Configured game state HTTP system.\n");
		// TODO(mcoms): fix race from writing messages during websocket start
		auto f = app.bindaddr("127.0.0.1").port(58270).concurrency(1).run_async();
		Msg("Running game state HTTP system.\n");
		app.wait_for_server_start();
		Msg("Started game state HTTP system.\n");
		s_GameStateManager.MarkReady();
		while (1)
		{
			m_hThreadEvent.Wait();
			if (m_bThreadShouldExit)
			{
				Msg("Stopping game state HTTP system.\n");
				m_Crow->stop();
				f.wait();
				delete m_Crow;
				m_Crow = NULL;
				return 0;
			}

			{
				AUTO_LOCK(m_clientsMutex)
				AUTO_LOCK(m_IncomingReturnsMutex)
				AUTO_LOCK(m_IncomingEventsMutex)

				while (m_IncomingReturns.Count() > 0)
				{
					CWebRpcReturn* pReturn = m_IncomingReturns.ElementAtHead();
					crow::json::wvalue data;
					data["i"] = pReturn->m_iRpcId;
					if (!pReturn->m_strValue.empty())
					{
						data["r"] = pReturn->m_strValue;
					}
					//DevMsg("%s\n", data.dump().c_str());
					for ( auto conn : m_connectedClients )
					{
						if ( conn )
						{
							DevMsg( "ret(%d): %s\n", pReturn->m_iRpcId, pReturn->m_strValue );
							conn->send_text( data.dump() );
						}
					}
					delete pReturn;
					m_IncomingReturns.RemoveAtHead();
				}

				while (m_IncomingEvents.Count() > 0)
				{
					CWebRpcEvent* pEvent = m_IncomingEvents.ElementAtHead();
					if (!pEvent->m_strEvent.empty())
					{
						crow::json::wvalue data;
						data["e"] = pEvent->m_strEvent;
						if (!pEvent->m_strParams.empty())
						{
							data["d"] = pEvent->m_strParams;
						}
						const bool bUnprivileged = m_UnprivilegedMethods.find( pEvent->m_strEvent ) != m_UnprivilegedMethods.end();
						bool bNeedsSubscription = false;
						if ( !V_stricmp( pEvent->m_strEvent.c_str(), "gsi" ) )
						{
							bNeedsSubscription = true;
						}
						for ( auto conn : m_connectedClients )
						{
							if ( !conn )
							{
								continue;
							}
							int64_t clientId = GetClientId( *conn );
							if ( ( bUnprivileged || m_iPrivilegedClientId == clientId ) && ( !bNeedsSubscription || m_SubscribedClientIds.find(clientId) != m_SubscribedClientIds.end() ) )
							{
								DevMsg( "send: %s(%s)\n", pEvent->m_strEvent.c_str(), pEvent->m_strParams.c_str() );
								conn->send_text( data.dump() );
							}
						}
					}
					delete pEvent;
					m_IncomingEvents.RemoveAtHead();
				}
			}
		}
	}

	void ConsumeMessages()
	{
		if (!ThreadInMainThread())
			return;

		AUTO_LOCK(m_IncomingReturnsMutex)
		AUTO_LOCK(m_OutgoingMutex)

		while (m_OutgoingGameMsgs.Count() > 0)
		{
			CWebRpcMessage* pMessage = m_OutgoingGameMsgs.ElementAtHead();
			bool bExecutionAllowed = true;
			if ( !pMessage->m_bPrivileged )
			{
				if ( m_UnprivilegedMethods.find( pMessage->m_strMethod ) == m_UnprivilegedMethods.end() )
				{
					bExecutionAllowed = false;
				}
			}
			if ( bExecutionAllowed )
			{
				std::unordered_map<std::string, std::function<std::string( const std::string& )>>::iterator funcIter;
				if ( ( funcIter = m_Methods.find( pMessage->m_strMethod ) ) != m_Methods.end() )
				{
					const auto& ret = funcIter->second( std::string{ pMessage->m_strParams } );
					QueueReturnNoLock( pMessage->m_iRpcId, ret );
				}
				else
				{
					QueueReturnNoLock( pMessage->m_iRpcId, "" );
				}
			}
			else
			{
				QueueReturnNoLock( pMessage->m_iRpcId, "" );
			}
			delete pMessage;
			m_OutgoingGameMsgs.RemoveAtHead();
		}
	}

	void QueueReturn(int64_t iRpcId, const std::string& strValue)
	{
		AUTO_LOCK(m_IncomingReturnsMutex)
		QueueReturnNoLock(iRpcId, strValue);
	}

	void QueueEvent(const std::string& strEvent, const std::string& strParams)
	{
		AUTO_LOCK(m_IncomingEventsMutex)
		QueueEventNoLock(strEvent, strParams);
	}

	void Shutdown()
	{
		m_bThreadShouldExit = true;
		m_hThreadEvent.Set();
	}

	void RegisterMethod(std::string methodName, const std::function<std::string(const std::string& params) >& method)
	{
		if (!ThreadInMainThread())
			return;

		m_Methods.emplace(methodName, method);
	}

	void UnregisterMethod(std::string methodName)
	{
		if (!ThreadInMainThread())
			return;

		m_Methods.erase(methodName);
	}

private:
	void QueueReturnNoLock(int64_t iRpcId, const std::string& strValue)
	{
		CWebRpcReturn* pReturn = new CWebRpcReturn;
		pReturn->m_iRpcId = iRpcId;
		pReturn->m_strValue = strValue;

		m_IncomingReturns.Insert(pReturn);

		m_hThreadEvent.Set();
	}

	void QueueEventNoLock(const std::string& strEvent, const std::string& strParams)
	{
		CWebRpcEvent* pEvent = new CWebRpcEvent;
		pEvent->m_iEventId = m_iEventId++;
		pEvent->m_strEvent = strEvent;
		pEvent->m_strParams = strParams;

		m_IncomingEvents.Insert(pEvent);

		m_hThreadEvent.Set();
	}


	bool m_bThreadShouldExit = false;
	CThreadEvent m_hThreadEvent;

	crow::SimpleApp* m_Crow;

	CUtlPriorityQueue<CWebRpcMessage*> m_OutgoingGameMsgs;
	CThreadMutex m_OutgoingMutex;
	CUtlPriorityQueue<CWebRpcReturn*> m_IncomingReturns;
	CThreadMutex m_IncomingReturnsMutex;
	CUtlPriorityQueue<CWebRpcEvent*> m_IncomingEvents;
	CThreadMutex m_IncomingEventsMutex;

	std::unordered_set<std::string> m_UnprivilegedMethods;
	std::unordered_set<std::string> m_UnprivilegedEvents;

	std::unordered_set<int64_t> m_SubscribedClientIds;

	int64_t m_iNextClientId = 1;
	int64_t m_iPrivilegedClientId = 0;

	std::vector<crow::websocket::connection*> m_connectedClients;
	CThreadMutex m_clientsMutex;

	int64_t m_iEventId = 1;

	std::unordered_map<std::string, std::function<std::string(const std::string& psQuery)>> m_Methods;
};

CGameStateManager::CGameStateManager()
	: CAutoGameSystemPerFrame("gamestate")
{
}

bool CGameStateManager::Init() 
{
	if (m_bInit)
		return true;

	Assert(!m_pServerThread);

	Msg("Initializing Game State HTTP thread.\n");

	m_pServerThread = new CHTTPServerThread();
	m_pServerThread->Start();

	m_bInit = true;

	RegisterMethod("localize", std::function([](const std::string& params)
	{
		std::string str;
		if (g_pVGuiLocalize)
		{
			std::wstring text = g_pVGuiLocalize->Find(params.c_str());
			std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
			str = converter.to_bytes(text);
			return str;
		}
		return str;
	}));

	RegisterMethod("getcvar", std::function([](const std::string& params)
	{
		std::string str;
		ConVarRef cvar(params.c_str(), true);
		if (cvar.IsValid())
		{
			str = cvar.GetString();
			return str;
		}
		return str;
	}));

	RegisterMethod("setcvar", std::function([](const std::string& params)
	{
		std::string str;
		size_t iSpacePos = params.find(' ');
		if (iSpacePos == std::string::npos)
			return str;

		std::string cvarName = params.substr(0, iSpacePos);
		std::string strVal = params.substr(iSpacePos + 1);
		UIConVarRef cvar(g_pVGui ? g_pVGui->GetVGUIEngine() : NULL, cvarName.c_str(), true);
		if (cvar.IsValid())
		{
			int iIntVal = Q_atoi(strVal.c_str());
			float flFloatVal = Q_atof(strVal.c_str());
			if ( fabsf((float)iIntVal - flFloatVal) < 0.000001f )
			{
				cvar.SetValue(iIntVal);
			}
			else
			{
				cvar.SetValue(flFloatVal);
			}
		}
		return str;
	}));

	RegisterMethod("cmd", std::function([](const std::string& params)
	{
		std::string str;
		// security check: no multiple commands
		size_t iSemicolonPos = params.find(';');
		if (iSemicolonPos != std::string::npos)
			return str;
		std::string cmdName;
		size_t iSpacePos = params.find(' ');
		if (iSpacePos == std::string::npos)
			cmdName = params;
		else
			cmdName = params.substr(0, iSpacePos);

		ConCommandBase* pCmd = g_pCVar->FindCommandBase(cmdName.c_str());
		if (!pCmd)
		{
			return str;
		}

		// security check: no restricted flags
		if (pCmd->IsFlagSet(FCVAR_CHEAT) || pCmd->IsFlagSet(FCVAR_DEVELOPMENTONLY))
		{
			return str;
		}

		engine->ClientCmd_Unrestricted(params.c_str());
		return str;
	}));

	RegisterMethod("getmodes", std::function([](const std::string& params)
	{
		int iCount = 0;
		vmode_s* pList = NULL;
		engine->GetVideoModes(iCount, pList);

		std::string str;
		for (int i = 0; i < iCount; i++, pList++)
		{
			CFmtStr res("%d %d;", pList->width, pList->height);
			str += res.Get();
		}
		return str.substr(0, str.length() - 1);
	}));

	RegisterMethod("getmode", std::function([](const std::string& params)
	{
		const MaterialSystem_Config_t& config = materials->GetCurrentConfigForVideoCard();

		CFmtStr mode("%d %d %d %d", config.m_VideoMode.m_Width, config.m_VideoMode.m_Height, config.Windowed() ? 1 : 0, config.NoWindowBorder() ? 1 : 0);
		std::string str = mode.Get();
		
		return str;
	}));

	RegisterMethod("playsound", std::function([](const std::string& params)
	{
		vgui::surface()->PlaySound(params.c_str());
		std::string str;
		return str;
	}));

	RegisterMethod("getingame", std::function([](const std::string& params)
	{
		std::string str = engine->IsInGame() && !engine->IsLevelMainMenuBackground() ? "1" : "0";
		return str;
	}));

#ifdef TF_CLIENT_DLL
	RegisterMethod("mmcmd", std::function([](const std::string& params)
	{
		GetMMDashboard()->OnCommand(params.c_str());
		std::string str;
		return str;
	}));
#endif

	RegisterMethod("uicmd", std::function([](const std::string& params)
	{
		std::string str;
#ifdef TF_CLIENT_DLL
		IViewPortPanel* pMMOverride = gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE );
		if (pMMOverride)
		{
			((CHudMainMenuOverride*)pMMOverride)->OnCommand( params.c_str() );
		}
#else
		// TODO: implement for other games please
#endif

		return str;
	}));

	RegisterMethod("openurl", std::function([](const std::string& params)
	{
		std::string str;
		UTIL_OpenWebPage(params.c_str());
		return str;
	}));

	return true;
}

void CGameStateManager::Update(float frametime)
{
	Assert(m_bInit);
	Assert(m_pServerThread);

	m_pServerThread->ConsumeMessages();
}

void CGameStateManager::Shutdown()
{
	if (!m_bInit)
        return;

	Msg("Shutting down game state HTTP system.\n");

	Assert(m_pServerThread);

	m_pServerThread->Shutdown();
	m_pServerThread->Join(2000);
	if (m_pServerThread->IsAlive())
	{
		m_pServerThread->Stop();
	}
	delete m_pServerThread;
	m_pServerThread = NULL;
}

void CGameStateManager::FireGameEvent(IGameEvent* event)
{
	const char* pszEvent = event->GetName();
	std::string data = pszEvent;
	data += " ";
	KeyValuesDumpAsString( event->GetDataKeys(), &data );
	QueueEvent( "gsi", data );
}

void CGameStateManager::RegisterMethod(std::string methodName,
                                       const std::function<std::string(const std::string& params)>& method)
{
	if (!m_bInit)
		return;

	Assert(m_pServerThread);

	m_pServerThread->RegisterMethod(methodName, method);
}

void CGameStateManager::UnregisterMethod(std::string methodName)
{
	if (!m_bInit)
		return;

	Assert(m_pServerThread);

	m_pServerThread->UnregisterMethod(methodName);
}

void CGameStateManager::QueueEvent(const std::string& strEvent, const std::string& strParams)
{
	if (!m_bInit)
		return;

	Assert(m_pServerThread);

	m_pServerThread->QueueEvent(strEvent, strParams);
}

void CGameStateManager::InitSubscriptions()
{
	if ( m_bListeningToEvents )
		return;

	m_bListeningToEvents = true;

	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "player_connect" );
	ListenForGameEvent( "player_disconnect" );
	ListenForGameEvent( "player_changeclass" );
	ListenForGameEvent( "player_team" );
	ListenForGameEvent( "player_info" );
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "player_spawn" );
	ListenForGameEvent( "player_hurt" );
	ListenForGameEvent( "round_start" );
	ListenForGameEvent( "round_end" );
	ListenForGameEvent( "server_spawn" );
	ListenForGameEvent( "client_disconnect" );
	ListenForGameEvent( "controlpoint_starttouch" );
	ListenForGameEvent( "controlpoint_endtouch" );
	ListenForGameEvent( "ctf_flag_captured" );
	ListenForGameEvent( "teamplay_broadcast_audio" );
	ListenForGameEvent( "teamplay_capture_blocked" );
	ListenForGameEvent( "teamplay_flag_event" );
	ListenForGameEvent( "teamplay_game_over" );
	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_round_stalemate" );
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_timer_time_added" );
	ListenForGameEvent( "teamplay_update_timer" );
	ListenForGameEvent( "teamplay_win_panel" );
	ListenForGameEvent( "teamplay_setup_finished" );
	ListenForGameEvent( "teamplay_alert" );
	ListenForGameEvent( "teamplay_teambalanced_player" );
	ListenForGameEvent( "teamplay_point_startcapture" );
	ListenForGameEvent( "teamplay_round_active" );
	ListenForGameEvent( "tf_game_over" );
	ListenForGameEvent( "object_destroyed" );
	ListenForGameEvent( "object_detonated" );
	ListenForGameEvent( "tournament_stateupdate" );
	ListenForGameEvent( "num_cappers_changed" );
}

void CGameStateManager::StopSubscriptions()
{
	if ( !m_bListeningToEvents )
		return;

	m_bListeningToEvents = false;

	StopListeningForAllEvents();
}
