//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Persistent WebSocket connection to the Comtress GC / Director
//
//=============================================================================

#include "cbase.h"
#include "comtress_gc_websocket.h"
#include "gc_clientsystem.h"
#pragma warning(push)
#pragma warning(disable: 4005)
#include "ixwebsocket/IXWebSocket.h"
#include "ixwebsocket/IXNetSystem.h"
#pragma warning(pop)
#include "gcsdk/gcclient.h"
#include "gcsdk/netpacket.h"
#include "gcsdk/msgbase.h"
#include "gcsdk/msgprotobuf.h"
#include "tier0/threadtools.h"
#include <queue>
#include <vector>

#ifdef CLIENT_DLL
#include "clientsteamcontext.h"
#include "steam/isteamuser.h"
#else
#include "steam/steam_api.h"
#include "enginecallback.h"
#endif

//-----------------------------------------------------------------------------
// Purpose: Pimpl implementation for IXWebSocket
//-----------------------------------------------------------------------------

#ifdef _WIN32
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "Normaliz.lib")

// Adjust path depending on where the executable is built relative to src/thirdparty/openssl/lib/win64/
// For Source Engine, the linker paths usually include the necessary paths or we specify absolute/relative.
// We can just rely on the server_base.vpc / client_base.vpc library paths, but just in case:
#pragma comment(lib, "..\\..\\thirdparty\\openssl\\lib\\win64\\libcrypto.lib")
#pragma comment(lib, "..\\..\\thirdparty\\openssl\\lib\\win64\\libssl.lib")
#endif

class CComtressGCWebsocketClient::CImpl
{
public:
	CImpl() : m_bConnected( false ), m_bStarted( false ), m_bNetworkInit( false ) {}
	~CImpl() {}

	void Init()
	{
		ix::initNetSystem();
		m_bNetworkInit = true;

		// Setup WebSocket
		// TODO: read from a cvar or config
		m_WebSocket.setUrl("wss://api.teamcomtress.com/wsgs/v1");

		m_WebSocket.setOnMessageCallback(
			[this](const ix::WebSocketMessagePtr& msg)
			{
				if (msg->type == ix::WebSocketMessageType::Message)
				{
					// Binary message received
					m_Mutex.Lock();
					m_qMessages.push(msg->str);
					m_Mutex.Unlock();
				}
				else if (msg->type == ix::WebSocketMessageType::Open)
				{
					DevMsg("Comtress GC WebSocket: Connected\n");
					m_bConnected = true;

					// Flush outbound message queue
					m_Mutex.Lock();
					for ( const std::string& queuedMsg : m_vecMessageQueue )
					{
						m_WebSocket.sendBinary( queuedMsg );
					}
					m_vecMessageQueue.clear();
					m_Mutex.Unlock();
				}
				else if (msg->type == ix::WebSocketMessageType::Close)
				{
					DevMsg("Comtress GC WebSocket: Disconnected\n");
					m_bConnected = false;
					m_bStarted = false; // Allow reconnection later if needed
				}
				else if (msg->type == ix::WebSocketMessageType::Error)
				{
					DevWarning("Comtress GC WebSocket Error: %s\n", msg->errorInfo.reason.c_str());
					m_bConnected = false;
				}
			});
	}

	void Think()
	{
		if ( !m_bNetworkInit || m_bStarted )
			return;

#ifdef CLIENT_DLL
		if ( steamapicontext && steamapicontext->SteamUser() && steamapicontext->SteamUser()->BLoggedOn() )
		{
			uint8 ticket[1024];
			uint32 ticketLen = 0;
			if ( steamapicontext->SteamUser()->GetAuthSessionTicket( ticket, sizeof(ticket), &ticketLen, nullptr ) )
			{
				char hexTicket[2049];
				Q_memset( hexTicket, 0, sizeof(hexTicket) );
				for ( uint32 i = 0; i < ticketLen; i++ )
				{
					V_snprintf( &hexTicket[i*2], 3, "%02x", ticket[i] );
				}
				
				ix::WebSocketHttpHeaders headers;
				headers["Authorization"] = CUtlString( "Steam " ) + hexTicket;
				m_WebSocket.setExtraHeaders(headers);
				
				m_bStarted = true;
				m_WebSocket.start();
			}
		}
#else
		if ( !engine->IsDedicatedServer() )
		{
			if ( steamapicontext && steamapicontext->SteamUser() && steamapicontext->SteamUser()->BLoggedOn() )
			{
				uint8 ticket[1024];
				uint32 ticketLen = 0;
				if ( steamapicontext->SteamUser()->GetAuthSessionTicket( ticket, sizeof(ticket), &ticketLen, nullptr ) )
				{
					char hexTicket[2049];
					Q_memset( hexTicket, 0, sizeof(hexTicket) );
					for ( uint32 i = 0; i < ticketLen; i++ )
					{
						V_snprintf( &hexTicket[i*2], 3, "%02x", ticket[i] );
					}
					
					ix::WebSocketHttpHeaders headers;
					headers["Authorization"] = CUtlString( "Steam " ) + hexTicket;
					m_WebSocket.setExtraHeaders(headers);
					
					m_bStarted = true;
					m_WebSocket.start();
				}
			}
		}
		else
		{
			static ConVarRef sv_private_token("sv_private_token");
			if ( sv_private_token.IsValid() && V_strlen( sv_private_token.GetString() ) > 0 )
			{
				ix::WebSocketHttpHeaders headers;
				headers["Authorization"] = CUtlString( "Token " ) + sv_private_token.GetString();
				m_WebSocket.setExtraHeaders(headers);
				
				m_bStarted = true;
				m_WebSocket.start();
			}
		}
#endif
	}

	void Shutdown()
	{
		m_WebSocket.stop();
		if ( m_bNetworkInit )
		{
			ix::uninitNetSystem();
			m_bNetworkInit = false;
		}
	}

	void Update()
	{
		std::queue<std::string> qProcess;
		{
			m_Mutex.Lock();
			std::swap(qProcess, m_qMessages);
			m_Mutex.Unlock();
		}

		while (!qProcess.empty())
		{
			const std::string& strMsg = qProcess.front();
			
			if ( strMsg.size() >= 2 * sizeof(uint32_t) )
			{
				// Try to dispatch it to the legacy GC system
				GCSDK::CNetPacket *pNetPacket = new GCSDK::CNetPacket();
				pNetPacket->Init( strMsg.size(), strMsg.data() );
				
				GCSDK::CIMsgNetPacketAutoRelease pMsgNetPacket( pNetPacket );
				
				// Parse the header to get the EMsg
				uint32_t eMsg = *(uint32_t*)pNetPacket->PubData();
				uint32_t headerLen = *(uint32_t*)(pNetPacket->PubData() + sizeof(uint32_t));
				
				if ( strMsg.size() >= 2 * sizeof(uint32_t) + headerLen )
				{
					CMsgProtoBufHeader hdr;
					hdr.ParseFromArray( pNetPacket->PubData() + 2 * sizeof(uint32_t), headerLen );

					GCSDK::JobMsgInfo_t info( eMsg & ~GCSDK::k_EMsgProtoBufFlag, hdr.job_id_source(), hdr.job_id_target(), GCSDK::k_EServerTypeGCClient );
					
					GCClientSystem()->GetGCClient()->GetJobMgr().BRouteMsgToJob( GCClientSystem()->GetGCClient(), pMsgNetPacket.Get(), info );
				}
				else
				{
					DevWarning("Comtress GC WebSocket: Received malformed packet (header length exceeds payload size)\n");
				}
			}
			else
			{
				DevWarning("Comtress GC WebSocket: Received malformed packet (too small)\n");
			}
			
			qProcess.pop();
		}
	}

	bool BSendMessage( uint32 unMsgType, const uint8 *pubData, uint32 cubData )
	{
		std::string payload((const char*)pubData, cubData);
		
		if ( m_bConnected )
		{
			m_WebSocket.sendBinary(payload);
		}
		else
		{
			m_Mutex.Lock();
			m_vecMessageQueue.push_back(payload);
			m_Mutex.Unlock();
		}
		return true;
	}

	bool BConnected() const { return m_bConnected; }

private:
	ix::WebSocket m_WebSocket;
	bool m_bConnected;
	bool m_bStarted;
	bool m_bNetworkInit;

	CThreadFastMutex m_Mutex;
	std::queue<std::string> m_qMessages;
	std::vector<std::string> m_vecMessageQueue;
};

//-----------------------------------------------------------------------------
// CComtressGCWebsocketClient implementation
//-----------------------------------------------------------------------------
CComtressGCWebsocketClient* ComtressGCWebsocket()
{
	static CComtressGCWebsocketClient* s_pComtressGCWebsocket = new CComtressGCWebsocketClient();
	return s_pComtressGCWebsocket;
}

CComtressGCWebsocketClient::CComtressGCWebsocketClient()
{
	m_pImpl = new CImpl();
}

CComtressGCWebsocketClient::~CComtressGCWebsocketClient()
{
	delete m_pImpl;
}

void CComtressGCWebsocketClient::Init()
{
	m_pImpl->Init();
}

void CComtressGCWebsocketClient::Shutdown()
{
	m_pImpl->Shutdown();
}

void CComtressGCWebsocketClient::Update()
{
	m_pImpl->Think(); // Try to connect if we haven't
	m_pImpl->Update(); // Pump messages
}

class CWebsocketSender : public GCSDK::CProtoBufMsgBase::IProtoBufSendHandler
{
public:
	CWebsocketSender( CComtressGCWebsocketClient* pClient ) : m_pClient( pClient ) {}
	virtual bool BAsyncSend( GCSDK::MsgType_t eMsg, const uint8 *pubMsgBytes, uint32 cubSize ) OVERRIDE
	{
		return m_pClient->BSendMessage( eMsg, pubMsgBytes, cubSize );
	}
private:
	CComtressGCWebsocketClient* m_pClient;
};

bool CComtressGCWebsocketClient::BSendMessage( const GCSDK::CProtoBufMsgBase& msg )
{
	CWebsocketSender sender( this );
	return msg.BAsyncSend( sender );
}

bool CComtressGCWebsocketClient::BSendMessage( const GCSDK::CGCMsgBase& msg )
{
	return m_pImpl->BSendMessage( 0, msg.PubPkt(), msg.CubPkt() );
}

bool CComtressGCWebsocketClient::BSendMessage( uint32 unMsgType, const uint8 *pubData, uint32 cubData )
{
	return m_pImpl->BSendMessage( unMsgType, pubData, cubData );
}

bool CComtressGCWebsocketClient::BConnected() const
{
	return m_pImpl->BConnected();
}
