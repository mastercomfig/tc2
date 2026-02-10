//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=====================================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "gcsdk/webapi_response.h"
#include "tf_stv_support.h"

#include "hltvdirector.h"
#include "tf_team.h"
#include "tf_gamerules.h"

// Global singleton
static CTFDemoTVSupport g_DemoTVSupport;

extern ConVar mp_tournament;

ConVar tv_demo_enable( "tv_demo_enable", "0", FCVAR_GAMEDLL | FCVAR_DONTRECORD, "TV Demo support - enable automatic .dem file recording and features. 0 - Manual, 1 - Auto-record all matches, 2 - Auto-record matchmaking matches, 3 - Auto-record tournament (mp_tournament) matches, 4 - Auto-record competitive matches", true, 0, true, 4 );
ConVar tv_demo_dir( "tv_demo_dir", "demos", FCVAR_GAMEDLL | FCVAR_DONTRECORD, "TV Demo support - will put all files into this folder under the gamedir. 24 characters max." );
ConVar tv_demo_prefix( "tv_demo_prefix", "", FCVAR_GAMEDLL | FCVAR_DONTRECORD, "TV Demo support - will prefix files with this string. 24 characters max." );
ConVar tv_demo_log( "tv_demo_log", "1", FCVAR_GAMEDLL | FCVAR_DONTRECORD, "TV Demo support - log to associated file per demo.", true, 0, true, 1 );
ConVar tv_demo_rounds_only( "tv_demo_rounds_only", "1", FCVAR_GAMEDLL | FCVAR_DONTRECORD, "TV Demo support - only record during the rounds of a match, no pre or post-game.", true, 0, true, 1 );

CON_COMMAND_F( tv_demo_record, "TV Demo support - start recording a demo.", FCVAR_GAMEDLL | FCVAR_DONTRECORD )
{
	g_DemoTVSupport.StartRecording();
}

CON_COMMAND_F( tv_demo_stop, "TV Demo support - stop recording a demo.", FCVAR_GAMEDLL | FCVAR_DONTRECORD )
{
	g_DemoTVSupport.StopRecording();
}

CON_COMMAND_F( tv_demo_status, "TV Demo support - show the current recording status.", FCVAR_GAMEDLL | FCVAR_DONTRECORD )
{
	g_DemoTVSupport.Status();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFDemoTVSupport::CTFDemoTVSupport() : CAutoGameSystemPerFrame( "CTFDemoTVSupport" )
{
	m_bRecording = false;
	m_DemoSpecificEventList.Clear();
	m_DemoSpecificEventList.SetStatusCode( k_EHTTPStatusCode200OK );
	m_pRoot = NULL;
	m_pDetailsNode = NULL;
	m_flNextRecordStartCheckTime = -1.f;
	m_nStartingTickCount = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFDemoTVSupport::Init()
{
	ListenForGameEvent( "ds_stop" );
	ListenForGameEvent( "teamplay_game_over" );
	ListenForGameEvent( "tf_game_over" );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFDemoTVSupport::LevelInitPostEntity()
{
	if ( !engine->IsDedicatedServer() )
		return;

	m_flNextRecordStartCheckTime = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFDemoTVSupport::LevelShutdownPostEntity()
{
	if ( !engine->IsDedicatedServer() )
		return;

	StopRecording();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFDemoTVSupport::FrameUpdatePostEntityThink()
{
	if ( !engine->IsDedicatedServer() || !HLTVDirector() || !HLTVDirector()->GetHLTVServer() )
		return;

	if ( tv_demo_enable.GetInt() > 0 )
	{
		if ( !m_bRecording )
		{
			if ( tv_demo_enable.GetInt() == 2 )
			{
				// IsCompetitiveMode got updated to include Casual. So tv_demo_enable 1 just means "if we're in a matchmaking match"
				if ( TFGameRules() && !TFGameRules()->IsCompetitiveMode() && !TFGameRules()->IsEmulatingMatch() )
					return;
			}
			else if ( tv_demo_enable.GetInt() == 3 )
			{
				if ( !mp_tournament.GetBool() )
					return;
			}
			else if ( tv_demo_enable.GetInt() == 4 )
			{
				if ( TFGameRules() && TFGameRules()->IsCompetitiveGame() )
					return;
			}

			if ( tv_demo_rounds_only.GetBool() )
			{
				if ( !TFGameRules() )
					return;

				const float flRestartTime = TFGameRules()->GetRoundRestartTime();
				const bool  bInCountdown = flRestartTime > 0.0f && flRestartTime - gpGlobals->curtime <= 7.5f;
				if ( !bInCountdown && TFGameRules()->State_Get() != GR_STATE_PREROUND && TFGameRules()->State_Get() != GR_STATE_RND_RUNNING )
					return;
			}

			if ( ( m_flNextRecordStartCheckTime < 0 ) || ( m_flNextRecordStartCheckTime < gpGlobals->curtime ) )
			{
				if ( !StartRecording() )
				{
					// we'll try again in 1 second
					m_flNextRecordStartCheckTime = gpGlobals->curtime + 1.f;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFDemoTVSupport::Status( void )
{
	if ( !engine->IsDedicatedServer() )
		return;

	char szStatus[64] = { 0 };

	if ( m_bRecording )
	{
		V_sprintf_safe( szStatus, "(TV Demo Support) Currently recording to %s\n", m_szFolderAndFilename );
	}
	else
	{
		V_strcpy_safe( szStatus, "(TV Demo Support) Not currently recording\n" );
	}

	Msg( szStatus );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFDemoTVSupport::FireGameEvent( IGameEvent* event )
{
	if ( !engine->IsDedicatedServer() || !HLTVDirector() || !HLTVDirector()->GetHLTVServer() )
		return;

	if ( !m_bRecording )
		return;

	const char* pszEvent = event->GetName();

	if ( tv_demo_rounds_only.GetBool() && ( FStrEq( pszEvent, "teamplay_game_over" ) || FStrEq( pszEvent, "tf_game_over" ) ) )
	{
		// Last branch so we can check tv_demo_rounds_only without messing up else logic.
		StopRecording();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFDemoTVSupport::IsValidPath( const char* pszFolder )
{
	if ( !pszFolder )
		return false;

	if ( Q_strlen( pszFolder ) <= 0 ||
	     Q_strstr( pszFolder, "\\\\" ) || // to protect network paths
	     Q_strstr( pszFolder, ":" ) ||    // to protect absolute paths
	     Q_strstr( pszFolder, ".." ) ||   // to protect relative paths
	     Q_strstr( pszFolder, "\n" ) ||   // CFileSystem_Stdio::FS_fopen doesn't allow this
	     Q_strstr( pszFolder, "\r" ) )    // CFileSystem_Stdio::FS_fopen doesn't allow this
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFDemoTVSupport::StartRecording( void )
{
	if ( !engine->IsDedicatedServer() || !HLTVDirector() || !HLTVDirector()->GetHLTVServer() )
		return false;

	// are we already recording?
	if ( m_bRecording )
	{
		Msg( "(TV Demo Support) Already recording\n" );
		return false;
	}

	// start recording the demo
	char szTime[k_RTimeRenderBufferSize] = { 0 };

	time_t     tTime = CRTime::RTime32TimeCur();
	struct tm  tmStruct;
	struct tm* ptm = Plat_localtime( &tTime, &tmStruct );

	V_sprintf_safe( szTime, "%04u-%02u-%02u_%02u-%02u-%02u", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec );

	char szPrefix[24] = { 0 };
	V_sprintf_safe( szPrefix, "%s", tv_demo_prefix.GetString() );
	V_sprintf_safe( m_szFilename, "%s%s", szPrefix, szTime );

	if ( Q_strlen( tv_demo_dir.GetString() ) > 0 )
	{
		// check folder
		if ( !IsValidPath( tv_demo_dir.GetString() ) )
		{
			Msg( "(TV Demo Support) invalid folder.\n" );
			return false;
		}

		V_sprintf_safe( m_szFolder, "%s", tv_demo_dir.GetString() );

		// make sure the folder exists
		g_pFullFileSystem->CreateDirHierarchy( m_szFolder, "GAME" );

		V_sprintf_safe( m_szFolderAndFilename, "%s%c%s", m_szFolder, CORRECT_PATH_SEPARATOR, m_szFilename );
	}
	else
	{
		m_szFolder[0] = '\0';
		V_sprintf_safe( m_szFolderAndFilename, "%s", m_szFilename );
	}

	char szCommand[MAX_PATH + 11];
	V_sprintf_safe( szCommand, "tv_record %s", m_szFolderAndFilename );
	engine->ServerCommand( szCommand );

	m_DemoSpecificEventList.Clear();
	m_DemoSpecificEventList.SetStatusCode( k_EHTTPStatusCode200OK );
	m_pRoot = m_DemoSpecificEventList.CreateRootValue( "summary" );
	m_DemoSpecificEventList.SetJSONAnonymousRootNode( true );
	if ( tv_demo_log.GetBool() )
	{
		m_pDetailsNode = m_pRoot->CreateChildObject( "details" );
		char mapname[MAX_MAP_NAME];
		
		Q_FileBase( gpGlobals->mapname.ToCStr(), mapname, sizeof( mapname ) );
		m_pDetailsNode->SetChildStringValue( "map_name", mapname );
		m_pDetailsNode->SetChildStringValue( "gamemode", TFGameRules()->GetGameTypeName() );
		if ( TFGameRules()->IsCompetitiveGame() )
		{
			// C_TFTeam::UpdateTeamName... can we get an equivalent for this on the server?
			static ConVarRef mp_tournament_redteamname( "mp_tournament_redteamname" );
			const char* pRedTeamName = "RED";
			if ( mp_tournament_redteamname.GetString() && mp_tournament_redteamname.GetString()[0] )
			{
				pRedTeamName = mp_tournament_redteamname.GetString();
			}
			m_pDetailsNode->SetChildStringValue( "team1", pRedTeamName );

			static ConVarRef mp_tournament_blueteamname( "mp_tournament_blueteamname" );
			const char* pBluTeamName = "BLU";
			if ( mp_tournament_blueteamname.GetString() && mp_tournament_blueteamname.GetString()[0] )
			{
				pBluTeamName = mp_tournament_blueteamname.GetString();
			}
			m_pDetailsNode->SetChildStringValue( "team2", pBluTeamName );
		}
	}

	m_bRecording = true;
	m_nStartingTickCount = gpGlobals->tickcount;

	char szMessage[MAX_PATH] = { 0 };
	V_sprintf_safe( szMessage, "(TV Demo Support) Start recording %s\n", m_szFolderAndFilename );
	Msg( szMessage );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFDemoTVSupport::StopRecording( bool bFromEngine /* = false */ )
{
	if ( !engine->IsDedicatedServer() || !HLTVDirector() || !HLTVDirector()->GetHLTVServer() )
		return;

	if ( !m_bRecording )
		return;

	m_bRecording = false;
	m_nStartingTickCount = 0;

	// stop recording the demo
	if ( !bFromEngine )
	{
		engine->ServerCommand( "tv_stoprecord" );
	}

	char szMessage[MAX_PATH] = { 0 };
	V_sprintf_safe( szMessage, "(Demo Support) End recording %s\n", m_szFolderAndFilename );
	Msg( szMessage );

	if ( tv_demo_log.GetBool() )
	{
		int nTickCount = gpGlobals->tickcount - m_nStartingTickCount;
		m_pDetailsNode->SetChildInt32Value( "ticks", nTickCount );

		// write out the associated bookmark and kill-streak data file
		char szTempFilename[MAX_PATH] = { 0 };
		V_sprintf_safe( szTempFilename, "%s.json", m_szFolderAndFilename );

		CUtlBuffer buffer( 0, 0, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::EXTERNAL_GROWABLE );
		m_DemoSpecificEventList.BEmitFormattedOutput( GCSDK::k_EWebAPIOutputFormat_JSON, buffer, 0 );
		g_pFullFileSystem->WriteFile( szTempFilename, "GAME", buffer );
	}

	m_DemoSpecificEventList.Clear();
	m_pRoot = NULL;
	m_pDetailsNode = NULL;
}
