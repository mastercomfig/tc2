//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Handles Game Coordinator backend responses to Game Servers.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tf_gc_api.h"

#include "econ_game_account_server.h"
#include "tf_gcmessages.pb.h"
#include "gc_clientsystem.h"
#include "tf_gamerules.h"

//
// Private ConVars used to expose backend status to the engine and master server
//
bool g_bAllowRegistrationChange = false;

void OnRegistrationMessageChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
	if ( !g_bAllowRegistrationChange )
	{
		g_bAllowRegistrationChange = true;
		((ConVar*)var)->SetValue( pOldValue );
		g_bAllowRegistrationChange = false;

		Warning( "This variable is controlled by the Game Coordinator and cannot be changed manually.\n" );
	}
}

static ConVar sv_registration_successful( "sv_registration_successful", "0", FCVAR_DONTRECORD | FCVAR_HIDDEN | FCVAR_NOTIFY, "Nonzero if we were able to login OK", OnRegistrationMessageChanged );
static ConVar sv_registration_message( "sv_registration_message", "No account specified", FCVAR_DONTRECORD | FCVAR_HIDDEN | FCVAR_NOTIFY, "Error message of other status text", OnRegistrationMessageChanged );
static ConVar tf_server_identity_disable_quickplay( "tf_server_identity_disable_quickplay", "0", FCVAR_ARCHIVE | FCVAR_NOTIFY, "Disable this server from being chosen by the quickplay matchmaking." );

const char *GameCoordinator_GetRegistrationString()
{
	return sv_registration_message.GetString();
}

void GameCoordinator_NotifyLevelShutdown()
{
	// TODO(mcoms): do we need this?
#ifndef SOURCESDK
	GCSDK::CProtoBufMsg< CMsgGC_GameServer_LevelInfo > msgLevelInfo( k_EMsgGC_GameServer_LevelInfo );
	msgLevelInfo.Body().set_level_loaded( false );
	GCClientSystem()->BSendMessage( msgLevelInfo );
#endif
}

void GameCoordinator_NotifyGameState()
{
	// TODO(mcoms): do we need this?
#ifndef SOURCESDK
	if ( !TFGameRules() )
	{
		GameCoordinator_NotifyLevelShutdown();
		return;
	}

	GCSDK::CProtoBufMsg< CMsgGC_GameServer_LevelInfo > msgLevelInfo( k_EMsgGC_GameServer_LevelInfo );
	msgLevelInfo.Body().set_level_loaded( true );
	msgLevelInfo.Body().set_level_name( gpGlobals->mapname.ToCStr() );
	GCClientSystem()->BSendMessage( msgLevelInfo );
#endif
}

class CGC_GameServer_AuthResult : public GCSDK::CGCClientJob
{
public:
	CGC_GameServer_AuthResult( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg< CMsgGC_GameServer_AuthResult > msg( pNetPacket );
		
		bool bIsOfficial = msg.Body().is_valve_server();
		bool bSuccess = msg.Body().authenticated();
		const std::string& strMsg = msg.Body().message();

		if ( bIsOfficial )
		{
			// The backend has explicitly trusted this server.
			Msg( "WARNING: Game server status 'Gordon'.\n" );
			engine->LogPrint( "WARNING: Game server status 'Gordon'.\n" );
		}

		g_bAllowRegistrationChange = true;

		if ( bSuccess )
		{
			const char *pStanding = GameServerAccount_GetStandingString( (eGameServerScoreStanding)msg.Body().game_server_standing() );
			const char *pStandingTrend = GameServerAccount_GetStandingTrendString( (eGameServerScoreStandingTrend)msg.Body().game_server_standing_trend() );
			Msg( "Game server authentication: SUCCESS! Standing: %s. Trend: %s\n", pStanding, pStandingTrend );
			UTIL_LogPrintf( "Game server authentication: SUCCESS! Standing: %s. Trend: %s\n", pStanding, pStandingTrend );
			
			if ( !strMsg.empty() )
			{
				Msg( "   %s\n", strMsg.c_str() );
				UTIL_LogPrintf( "   %s\n", strMsg.c_str() );
			}

			// Expose the 'Gordon' status to the engine tags if we're official
			if ( bIsOfficial )
				sv_registration_message.SetValue( "Status 'Gordon'" );
			else
				sv_registration_message.SetValue( "" );
		}
		else
		{
			Warning( "Game server authentication: FAILURE!\n" );
			UTIL_LogPrintf( "Game server authentication: FAILURE!\n" );

			if ( !strMsg.empty() )
			{
				Warning( "   %s\n", strMsg.c_str() );
				UTIL_LogPrintf( "   %s\n", strMsg.c_str() );
				sv_registration_message.SetValue( strMsg.c_str() );
			}
			else
			{
				sv_registration_message.SetValue( "failed" );
			}
		}

		sv_registration_successful.SetValue( bSuccess );
		g_bAllowRegistrationChange = false;
		return true;
	}
};

GC_REG_JOB( GCSDK::CGCClient, CGC_GameServer_AuthResult, "CGC_GameServer_AuthResult", k_EMsgGC_GameServer_AuthResult, GCSDK::k_EServerTypeGCClient );
