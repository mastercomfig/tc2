//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Persistent WebSocket connection to the Comtress GC / Director
//
//=============================================================================

#ifndef COMTRESS_GC_WEBSOCKET_H
#define COMTRESS_GC_WEBSOCKET_H

#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/msgprotobuf.h"

class CComtressGCWebsocketClient
{
public:
	CComtressGCWebsocketClient();
	~CComtressGCWebsocketClient();

	void Init();
	void Shutdown();
	void Update();

	bool BSendMessage( const GCSDK::CProtoBufMsgBase& msg );
	bool BSendMessage( const GCSDK::CGCMsgBase& msg );
	bool BSendMessage( uint32 unMsgType, const uint8 *pubData, uint32 cubData );

	bool BConnected() const;

private:
	class CImpl;
	CImpl *m_pImpl;
};

extern CComtressGCWebsocketClient* ComtressGCWebsocket();

#endif // COMTRESS_GC_WEBSOCKET_H
