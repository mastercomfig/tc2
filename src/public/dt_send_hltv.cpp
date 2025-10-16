//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//


#include "dt_send_hltv.h"
#include "dt_send.h"

#include "enginecallback.h"
#include "hltvdirector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if !defined(_STATIC_LINKED) || defined(GAME_DLL)

void SendProxy_AddHLTV( CSendProxyRecipients *pRecipients )
{
	// also send to the HLTV slot
	if ( engine->IsDedicatedServer() )
	{
		IHLTVServer *pHLTVServer = HLTVDirector()->GetHLTVServer();
		if ( pHLTVServer )
		{
			int iSlot = pHLTVServer->GetHLTVSlot();
			if ( iSlot >= 0 )
			{
				pRecipients->SetRecipient( iSlot );
			}
		}
	}
	else
	{
		pRecipients->SetRecipient( 0 );
	}
}

#endif
