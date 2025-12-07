//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: HTTP/WS game state
//
//===========================================================================//

//#define ASIO_NO_EXCEPTIONS
#define _WIN32_WINNT 0x0602
#define CROW_STATIC_DIRECTORY "tc2/loose/resource/html/"
#define CROW_STATIC_ENDPOINT "/ui/<path>"

#include <crow.h>

#undef _WIN32_WINNT

#include "cbase.h"

#include "gamestate.h"

#include <codecvt>
#include <tier0/platform.h>
#include <tier3/tier3.h>

#include "vgui/IVGui.h"

static CGameStateManager s_GameStateManager;
CGameStateManager* GetGameStateManager() { return &s_GameStateManager; }

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

	static bool LessFunc(CWebRpcMessage* const& lhs, CWebRpcMessage* const& rhs)
	{
		return lhs->m_iRpcId > rhs->m_iRpcId;
	}
};

class CHTTPServerThread : public CThread
{
public:
	CHTTPServerThread() :
	m_Incoming(0, 0, CWebRpcReturn::LessFunc),
	m_Outgoing(0, 0, CWebRpcMessage::LessFunc)
	{
		SetName("GameStateHTTPThread");
	}

	// Return 0 for success
	virtual int Run() OVERRIDE
	{
		m_Crow = new crow::SimpleApp();
		crow::SimpleApp& app = *m_Crow;
		CROW_ROUTE(app, "/")([]() {
			return "Hello world!";
		});
		CROW_WEBSOCKET_ROUTE(app, "/ws")
		.onopen([&](crow::websocket::connection& conn) {
			{
				AUTO_LOCK(m_clientsMutex)
				if (!m_connectedClients.empty())
					return;
				m_connectedClients.push_back(&conn);
			}
		})
		.onclose([&](crow::websocket::connection& conn, const std::string& reason, uint16_t) {
			{
				// TODO: sessions
				AUTO_LOCK(m_clientsMutex)
				m_connectedClients.clear();
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

			{
				AUTO_LOCK( m_clientsMutex )
				if (m_connectedClients.empty())
				{
					m_connectedClients.push_back(&conn);
				}
			}

			std::string params;
			if (data.has("p") && data["p"].t() == crow::json::type::String)
			{
				params = data["p"].s();
			}

			{
				AUTO_LOCK(m_OutgoingMutex)
				CWebRpcMessage* pMessage = new CWebRpcMessage;
				pMessage->m_iRpcId = id;
				pMessage->m_strMethod = command;
				pMessage->m_strParams = params;
				m_Outgoing.Insert(pMessage);
			}
		});
		auto f = app.bindaddr("127.0.0.1").port(58270).run_async();
		app.wait_for_server_start();
		while (1)
		{
			m_hThreadEvent.Wait();
			if (m_bThreadShouldExit)
			{
				m_Crow->stop();
				f.wait();
				delete m_Crow;
				m_Crow = NULL;
				return 0;
			}

			{
				AUTO_LOCK(m_clientsMutex)
				AUTO_LOCK(m_IncomingMutex)

				crow::websocket::connection* conn = m_connectedClients.empty() ? NULL : m_connectedClients[0];

				while (m_Incoming.Count() > 0)
				{
					CWebRpcReturn* pReturn = m_Incoming.ElementAtHead();
					crow::json::wvalue data;
					data["i"] = pReturn->m_iRpcId;
					if (!pReturn->m_strValue.empty())
					{
						data["r"] = pReturn->m_strValue;
					}
					if (conn)
					{
						conn->send_text(data.dump());
					}
					delete pReturn;
					m_Incoming.RemoveAtHead();
				}
			}
		}
	}

	void ConsumeMessages()
	{
		if (!ThreadInMainThread())
			return;

		AUTO_LOCK(m_IncomingMutex)
		AUTO_LOCK(m_OutgoingMutex)

		while (m_Outgoing.Count() > 0)
		{
			CWebRpcMessage* pMessage = m_Outgoing.ElementAtHead();
			std::unordered_map<std::string, std::function<std::string(const std::string&)>>::iterator funcIter;
			if ((funcIter = m_Methods.find(pMessage->m_strMethod)) != m_Methods.end())
			{
				const auto& ret = funcIter->second(std::string{ pMessage->m_strParams });
				QueueReturnNoLock(pMessage->m_iRpcId, ret);
			}
			else
			{
				QueueReturnNoLock(pMessage->m_iRpcId, "");
			}
			delete pMessage;
			m_Outgoing.RemoveAtHead();
		}
	}

	void QueueReturn(int64_t iRpcId, const std::string& strValue)
	{
		AUTO_LOCK(m_IncomingMutex)
		QueueReturnNoLock(iRpcId, strValue);
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

		m_Incoming.Insert(pReturn);

		m_hThreadEvent.Set();
	}


	bool m_bThreadShouldExit = false;
	CThreadEvent m_hThreadEvent;

	crow::SimpleApp* m_Crow;

	CUtlPriorityQueue<CWebRpcMessage*> m_Outgoing;
	CThreadMutex m_OutgoingMutex;
	CUtlPriorityQueue<CWebRpcReturn*> m_Incoming;
	CThreadMutex m_IncomingMutex;

	std::vector<crow::websocket::connection*> m_connectedClients;
	CThreadMutex m_clientsMutex;

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
		engine->ClientCmd_Unrestricted(params.c_str());
		return str;
	}));

	RegisterMethod("getsg", std::function([](const std::string& params)
	{
		std::string str;
		size_t iSpacePos = params.find(' ');
		if (iSpacePos == std::string::npos)
			return str;

		std::string settingName = params.substr(0, iSpacePos);
		std::string settingOpt = params.substr(iSpacePos + 1);

		if (settingName == "preset")
		{
			str = "3";
		}
		else if (settingName == "shadows")
		{
			str = "3";
		}
		else if (settingName == "lighting")
		{
			str = "3";
		}
		else if (settingName == "effects")
		{
			str = "3";
		}
		else if (settingName == "postprocess")
		{
			str = "3";
		}
		else if (settingName == "sound")
		{
			str = "3";
		}

		return str;
	}));

	RegisterMethod("setsg", std::function([](const std::string& params)
	{
		std::string str;
		size_t iSpacePos = params.find(' ');
		if (iSpacePos == std::string::npos)
			return str;

		std::string settingName = params.substr(0, iSpacePos);
		std::string settingOpt = params.substr(iSpacePos + 1);

		if (settingName == "preset")
		{
		}
		else if (settingName == "shadows")
		{
		}
		else if (settingName == "lighting")
		{;
		}
		else if (settingName == "effects")
		{
		}
		else if (settingName == "postprocess")
		{
		}
		else if (settingName == "sound")
		{
		}

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
