//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "mp_shareddefs.h"
#include "teamplayroundbased_gamerules.h"

#ifdef CLIENT_DLL
	#include "iclientmode.h"
	#include <vgui_controls/AnimationController.h>
	#include <igameevents.h>
	#include "c_team.h"
	#include "c_playerresource.h"
	#define CTeam C_Team

#else
	#include "viewport_panel_names.h"
	#include "team.h"
	#include "mapentities.h"
	#include "gameinterface.h"
	#include "eventqueue.h"
	#include "team_control_point_master.h"
	#include "team_train_watcher.h"
	#include "serverbenchmark_base.h"
	#include "hltvdirector.h"

#if defined( REPLAY_ENABLED )	
	#include "replay/ireplaysystem.h"
	#include "replay/iserverreplaycontext.h"
	#include "replay/ireplaysessionrecorder.h"
#endif // REPLAY_ENABLED
#endif

#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
	#include "tf_gamerules.h"
	#ifdef GAME_DLL
		#include "player_vs_environment/tf_population_manager.h"
		#include "tf_team.h"
		#include "tf_gc_server.h"
		#include "tf_objective_resource.h"
		#include "player_resource.h"
		#include "tf_player_resource.h"
		#include "tf_autobalance.h" // Added by user instruction
	#else
		#include "tf_gc_client.h"
		#include "c_tf_objective_resource.h"
	#endif // GAME_DLL
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
CUtlVector< CHandle<CTeamControlPointMaster> >		g_hControlPointMasters;

extern bool IsInCommentaryMode( void );

#if defined( REPLAY_ENABLED )
extern IReplaySystem *g_pReplay;
#endif // REPLAY_ENABLED
#endif

extern ConVar spec_freeze_time;
extern ConVar spec_freeze_traveltime;

#ifdef CLIENT_DLL
void RecvProxy_TeamplayRoundState( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CTeamplayRoundBasedRules *pGamerules = ( CTeamplayRoundBasedRules *)pStruct;
	int iRoundState = pData->m_Value.m_Int;
	pGamerules->SetRoundState( iRoundState );
}

void RecvProxy_StopWatch( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*(bool*)(pOut) = ( pData->m_Value.m_Int > 0 );

	IGameEvent *event = gameeventmanager->CreateEvent( "stop_watch_changed" );
	if ( event )
	{
		// Client-side once it's actually happened
		gameeventmanager->FireEventClientSide( event );
	}
}
#endif 

BEGIN_NETWORK_TABLE_NOBASE( CTeamplayRoundBasedRules, DT_TeamplayRoundBasedRules )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iRoundState ), 0, RecvProxy_TeamplayRoundState ),
	RecvPropBool( RECVINFO( m_bInWaitingForPlayers ) ),
	RecvPropInt( RECVINFO( m_iWinningTeam ) ),
	RecvPropInt( RECVINFO( m_bInOvertime ) ),
	RecvPropBool( RECVINFO( m_bInSetup ) ),
	RecvPropInt( RECVINFO( m_iSetupTime ) ),
	RecvPropInt( RECVINFO( m_bSwitchedTeamsThisRound ) ),
	RecvPropBool( RECVINFO( m_bStopWatchShouldBeTimedWin ) ),
	RecvPropBool( RECVINFO( m_bAwaitingReadyRestart ) ),
	RecvPropTime( RECVINFO( m_flRestartRoundTime ) ),
	RecvPropTime( RECVINFO( m_flRestartRoundStartTime ) ),
	RecvPropTime( RECVINFO( m_flMapResetTime ) ),
	RecvPropInt( RECVINFO( m_nRoundsPlayed ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_flNextRespawnWave), RecvPropTime( RECVINFO(m_flNextRespawnWave[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_TeamRespawnWaveTimes), RecvPropFloat( RECVINFO(m_TeamRespawnWaveTimes[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bTeamReady), RecvPropBool( RECVINFO(m_bTeamReady[0]) ) ),
	RecvPropBool( RECVINFO( m_bStopWatch ), 0, RecvProxy_StopWatch ),
	RecvPropBool( RECVINFO( m_bMultipleTrains ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bPlayerReady), RecvPropBool( RECVINFO(m_bPlayerReady[0]) ) ),
	RecvPropBool( RECVINFO( m_bCheatsEnabledDuringLevel ) ),
	RecvPropTime( RECVINFO( m_flCountdownTime ) ),
	RecvPropTime( RECVINFO( m_flStateTransitionTime ) ),
	RecvPropBool( RECVINFO( m_bGamePaused ) ),
	RecvPropBool( RECVINFO( m_bPauseEnabled ) ),
	RecvPropTime( RECVINFO( m_flUnpauseCurTime ) ),
#else
	SendPropInt( SENDINFO( m_iRoundState ), 5 ),
	SendPropBool( SENDINFO( m_bInWaitingForPlayers ) ),
	SendPropInt( SENDINFO( m_iWinningTeam ), 3, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bInOvertime ) ),
	SendPropBool( SENDINFO( m_bInSetup ) ),
	SendPropInt( SENDINFO( m_iSetupTime ) ),
	SendPropBool( SENDINFO( m_bSwitchedTeamsThisRound ) ),
	SendPropBool( SENDINFO( m_bStopWatchShouldBeTimedWin ) ),
	SendPropBool( SENDINFO( m_bAwaitingReadyRestart ) ),
	SendPropTime( SENDINFO( m_flRestartRoundTime ) ),
	SendPropTime( SENDINFO( m_flRestartRoundStartTime ) ),
	SendPropTime( SENDINFO( m_flMapResetTime ) ),
	SendPropInt( SENDINFO( m_nRoundsPlayed ), 4, SPROP_UNSIGNED ),
	SendPropArray3( SENDINFO_ARRAY3(m_flNextRespawnWave), SendPropTime( SENDINFO_ARRAY(m_flNextRespawnWave) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_TeamRespawnWaveTimes), SendPropFloat( SENDINFO_ARRAY(m_TeamRespawnWaveTimes) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bTeamReady), SendPropBool( SENDINFO_ARRAY(m_bTeamReady) ) ),
	SendPropBool( SENDINFO( m_bStopWatch ) ),
	SendPropBool( SENDINFO( m_bMultipleTrains ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bPlayerReady), SendPropBool( SENDINFO_ARRAY(m_bPlayerReady) ) ),
	SendPropBool( SENDINFO( m_bCheatsEnabledDuringLevel ) ),
	SendPropTime( SENDINFO( m_flCountdownTime ) ),
	SendPropTime( SENDINFO( m_flStateTransitionTime ) ),
	SendPropBool( SENDINFO( m_bGamePaused ) ),
	SendPropBool( SENDINFO( m_bPauseEnabled ) ),
	SendPropTime( SENDINFO( m_flUnpauseCurTime ) ),
#endif
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED( TeamplayRoundBasedRulesProxy, DT_TeamplayRoundBasedRulesProxy )

#ifdef CLIENT_DLL
void RecvProxy_TeamplayRoundBasedRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	Assert( pRules );
	*pOut = pRules;
}

BEGIN_RECV_TABLE( CTeamplayRoundBasedRulesProxy, DT_TeamplayRoundBasedRulesProxy )
	RecvPropDataTable( "teamplayroundbased_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_TeamplayRoundBasedRules ), RecvProxy_TeamplayRoundBasedRules )
END_RECV_TABLE()

void CTeamplayRoundBasedRulesProxy::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );
	// Reroute data changed calls to the non-entity gamerules 
	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	Assert( pRules );
	pRules->OnPreDataChanged(updateType);
}
void CTeamplayRoundBasedRulesProxy::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	// Reroute data changed calls to the non-entity gamerules 
	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	Assert( pRules );
	pRules->OnDataChanged(updateType);
}

#else
void* SendProxy_TeamplayRoundBasedRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	Assert( pRules );
	pRecipients->SetAllRecipients();
	return pRules;
}

BEGIN_SEND_TABLE( CTeamplayRoundBasedRulesProxy, DT_TeamplayRoundBasedRulesProxy )
	SendPropDataTable( "teamplayroundbased_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_TeamplayRoundBasedRules ), SendProxy_TeamplayRoundBasedRules )
END_SEND_TABLE()

BEGIN_DATADESC( CTeamplayRoundBasedRulesProxy )
	// Inputs.
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetStalemateOnTimelimit", InputSetStalemateOnTimelimit ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRulesProxy::InputSetStalemateOnTimelimit( inputdata_t &inputdata )
{
	TeamplayRoundBasedRules()->SetStalemateOnTimelimit( inputdata.value.Bool() );
}
#endif

#ifdef GAME_DLL
void WinlimitChanged(IConVar *var, const char *pOldValue, float flOldValue)
{
	IGameEvent * event = gameeventmanager->CreateEvent("winlimit_changed");
	if (event)
	{
		gameeventmanager->FireEvent(event);
	}
}
#endif // GAME_DLL

ConVar mp_capstyle( "mp_capstyle", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Sets the style of capture points used. 0 = Fixed players required to cap. 1 = More players cap faster, but longer cap times." );
ConVar mp_blockstyle( "mp_blockstyle", "1", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Sets the style of capture point blocking used. 0 = Blocks break captures completely. 1 = Blocks only pause captures." );
ConVar mp_respawnwavetime( "mp_respawnwavetime", "10.0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Time between respawn waves." );
ConVar mp_capdeteriorate_time( "mp_capdeteriorate_time", "90.0", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "Time it takes for a full capture point to deteriorate." );
ConVar mp_tournament( "mp_tournament", "0", FCVAR_REPLICATED | FCVAR_NOTIFY );
ConVar mp_tournament_post_match_period( "mp_tournament_post_match_period", "90", FCVAR_REPLICATED, "The amount of time (in seconds) before the server resets post-match.", true, 5, true, 300 );

ConVar mp_tournament_required_for_pause( "mp_tournament_required_for_pause", "1", FCVAR_REPLICATED );

#if defined( TF_CLIENT_DLL ) || defined( TF_DLL )
ConVar mp_highlander( "mp_highlander", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Allow only 1 of each player class type." );
ConVar mp_sixes("mp_sixes", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enforces standard Sixes competitive ruleset.");
#endif
#ifdef TF_DLL
extern ConVar tf_competitive_preround_duration;
extern ConVar tf_competitive_preround_countdown_duration;
#endif // TF_DLL

//Arena Mode
ConVar tf_arena_preround_time( "tf_arena_preround_time", "10", FCVAR_NOTIFY | FCVAR_REPLICATED, "Length of the Pre-Round time", true, 5.0, true, 15.0 );
ConVar tf_arena_round_time( "tf_arena_round_time", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );
ConVar tf_arena_max_streak( "tf_arena_max_streak", "3", FCVAR_NOTIFY | FCVAR_REPLICATED, "Teams will be scrambled if one team reaches this streak" );
ConVar tf_arena_use_queue( "tf_arena_use_queue", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enables the spectator queue system for Arena." );

ConVar mp_teams_unbalance_limit( "mp_teams_unbalance_limit", "1", FCVAR_REPLICATED,
					 "Teams are unbalanced when one team has this many more players than the other team. (0 disables check)",
					 true, 0,	// min value
					 true, 30	// max value
					 );

ConVar mp_maxrounds( "mp_maxrounds", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "max number of rounds to play before server changes maps", true, 0, false, 0 );

ConVar mp_winlimit( "mp_winlimit", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Max score one team can reach before server changes maps", true, 0, false, 0
#ifdef GAME_DLL
	, WinlimitChanged
#endif // GAME_DLL
	);

ConVar mp_timelimit_added_winlimit( "mp_timelimit_added_winlimit", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Added to current highest team score to set a new winlimit when time runs out.", true, 0, false, 0 );


ConVar mp_disable_respawn_times( "mp_disable_respawn_times", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "0 - Enable respawn times, 1 - Instant respawn but keep freeze time, 2 - Instant respawn without freeze time, 3 - Instant respawn with cancelable freeze time" );
ConVar mp_bonusroundtime( "mp_bonusroundtime", "15", FCVAR_REPLICATED, "Time after round win until round restarts", true, 5, true, 15 );
ConVar mp_stalemate_meleeonly( "mp_stalemate_meleeonly", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Restrict everyone to melee weapons only while in Sudden Death." );
ConVar mp_forceautoteam( "mp_forceautoteam", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Automatically assign players to teams when joining." );

ConVar mp_pause_setting( "mp_pause_setting", "0", FCVAR_REPLICATED, "0 - Disable game pauses, 1 - limited pauses, 2 - unlimited pauses" );
ConVar mp_pause_cooldown( "mp_pause_cooldown", "2", FCVAR_REPLICATED, "" );
ConVar mp_pause_cooldown_time( "mp_pause_cooldown_time", "300", FCVAR_REPLICATED, "Number of seconds before a player is allowed to pause again" );
ConVar mp_pause_count( "mp_pause_count", "2", FCVAR_REPLICATED, "Number of times a player is allowed to pause the game." );
//ConVar mp_pause_countdown( "mp_pause_countdown", "0", FCVAR_REPLICATED, "Countdown to pause the game." );
ConVar mp_unpause_countdown( "mp_unpause_countdown", "3", FCVAR_REPLICATED, "Countdown to unpause the game." );
ConVar mp_pause_force_unpause_time( "mp_pause_force_unpause_time", "300", FCVAR_REPLICATED, "Number of seconds after which the game will automatically unpause" );
ConVar mp_pause_game_pause_silently( "mp_pause_game_pause_silently", "0", FCVAR_REPLICATED, "" );
ConVar mp_pause_same_team_resume_time( "mp_pause_same_team_resume_time", "5", FCVAR_REPLICATED, "Number of seconds resuming is restricted to the same team, after that either team can pause" );
ConVar mp_pause_same_team_resume_time_disconnected( "mp_pause_same_team_resume_time_disconnected", "30", FCVAR_REPLICATED, "Number of seconds resuming is restricted to the same team if someone disconnected, after that either team can pause" );
// TODO(mcoms)
ConVar mp_unpause_mass_disconnect_cooldown( "mp_unpause_mass_disconnect_cooldown", "86400", FCVAR_REPLICATED, "" );

#if defined( _DEBUG ) || defined( STAGING_ONLY )
ConVar mp_developer( "mp_developer", "0", FCVAR_ARCHIVE | FCVAR_REPLICATED | FCVAR_NOTIFY, "1: basic conveniences (instant respawn and class change, etc).  2: add combat conveniences (infinite ammo, buddha, etc)" );
#endif // _DEBUG || STAGING_ONLY

#ifdef GAME_DLL
ConVar mp_showroundtransitions( "mp_showroundtransitions", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show gamestate round transitions." );
ConVar mp_enableroundwaittime( "mp_enableroundwaittime", "1", FCVAR_REPLICATED, "Enable timers to wait between rounds." );
ConVar mp_showcleanedupents( "mp_showcleanedupents", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Show entities that are removed on round respawn." );
ConVar mp_restartround( "mp_restartround", "0", FCVAR_GAMEDLL, "If non-zero, the current round will restart in the specified number of seconds" );	

ConVar mp_stalemate_timelimit( "mp_stalemate_timelimit", "240", FCVAR_REPLICATED, "Timelimit (in seconds) of the stalemate round." );
ConVar mp_autoteambalance( "mp_autoteambalance", "2", FCVAR_NOTIFY, "Automatically balance the teams based on mp_teams_unbalance_limit. 0 = off, 1 = legacy system, 2 = new system", true, 0, true, 2 );

#ifdef TF2_OG
#define DEFAULT_STALEMATE "1"
#else
#define DEFAULT_STALEMATE "0"
#endif
ConVar mp_stalemate_enable( "mp_stalemate_enable", DEFAULT_STALEMATE, FCVAR_NOTIFY, "Enable/Disable stalemate mode." );
ConVar mp_match_end_at_timelimit( "mp_match_end_at_timelimit", "0", FCVAR_NOTIFY, "Allow the match to end when mp_timelimit hits instead of waiting for the end of the current round." );

ConVar mp_holiday_nogifts( "mp_holiday_nogifts", "0", FCVAR_NOTIFY, "Set to 1 to prevent holiday gifts from spawning when players are killed." );

const char *m_pszRoundStateStrings[] = 
{
	"GR_STATE_INIT",
	"GR_STATE_PREGAME",
	"GR_STATE_STARTGAME",
	"GR_STATE_PREROUND",
	"GR_STATE_RND_RUNNING",
	"GR_STATE_TEAM_WIN",
	"GR_STATE_RESTART",
	"GR_STATE_STALEMATE",
	"GR_STATE_GAME_OVER",
	"GR_STATE_BONUS",
	"GR_STATE_BETWEEN_RNDS",
};

COMPILE_TIME_ASSERT( ARRAYSIZE( m_pszRoundStateStrings ) == GR_NUM_ROUND_STATES );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------	
void cc_SwitchTeams( const CCommand& args )
{
	if ( UTIL_IsCommandIssuedByServerAdmin() )
	{
		CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );

		if ( pRules )
		{
			pRules->SetSwitchTeams( true );
			mp_restartgame.SetValue( 5 );
			pRules->ShouldResetScores( false, false );
			pRules->ShouldResetRoundsPlayed( false );
		}
	}
}

static ConCommand mp_switchteams( "mp_switchteams", cc_SwitchTeams, "Switch teams and restart the game" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------	
void cc_ScrambleTeams( const CCommand& args )
{
	if ( UTIL_IsCommandIssuedByServerAdmin() )
	{
		CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );

		if ( pRules )
		{
			pRules->SetScrambleTeams( true );
			mp_restartgame.SetValue( 5 );
			pRules->ShouldResetScores( true, false );

			if ( args.ArgC() == 2 )
			{
				// Don't reset the roundsplayed when mp_scrambleteams 2 is passed
				if ( atoi( args[1] ) == 2 )
				{
					pRules->ShouldResetRoundsPlayed( false );
				}
			}
		}
	}
}

static ConCommand mp_scrambleteams( "mp_scrambleteams", cc_ScrambleTeams, "Scramble the teams and restart the game" );
ConVar mp_scrambleteams_auto( "mp_scrambleteams_auto", "1", FCVAR_NOTIFY, "Server will automatically scramble the teams if criteria met.  Only works on dedicated servers." );
ConVar mp_scrambleteams_auto_windifference( "mp_scrambleteams_auto_windifference", "2", FCVAR_NOTIFY, "Number of round wins a team must lead by in order to trigger an auto scramble." );

ConVar mp_shuffleteams_auto( "mp_shuffleteams_auto", "1", FCVAR_NOTIFY, "Server will automatically shuffle players without resetting if criteria met." );
ConVar mp_shuffleteams_auto_seriesdifference( "mp_shuffleteams_auto_seriesdifference", "2", FCVAR_NOTIFY, "Number of series wins a team must lead by in order to trigger an auto shuffle." );
ConVar mp_shuffleteams_auto_score_disparity( "mp_shuffleteams_auto_score_disparity", "1.5", FCVAR_NOTIFY, "Total score ratio (WinningTeam to LosingTeam) indicating an imbalanced stomp, triggering a shuffle." );

// Classnames of entities that are preserved across round restarts
static const char *s_PreserveEnts[] =
{
	"player",
	"viewmodel",
	"worldspawn",
	"soundent",
	"ai_network",
	"ai_hint",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sprite",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_wall",
	"func_illusionary",
	"info_node",
	"info_target",
	"info_node_hint",
	"point_commentary_node",
	"point_viewcontrol",
	"func_precipitation",
	"func_team_wall",
	"shadow_control",
	"sky_camera",
	"scene_manager",
	"trigger_soundscape",
	"commentary_auto",
	"point_commentary_node",
	"point_commentary_viewpoint",
	"bot_roster",
	"info_populator",
	"", // END Marker
};

CON_COMMAND_F( mp_forcewin, "Forces team to win", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	if ( pRules )
	{
		int iTeam;		
		if ( args.ArgC() == 1 )
		{
			// if no team specified, use player 1's team
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
			if ( pPlayer )
			{
				iTeam = pPlayer->GetTeamNumber();
			}
			else
			{
				Msg( "Unable to determine default team. Usage: mp_forcewin <opt: team#> <opt: reason>\n" );
				return;
			}
		}
		else if ( args.ArgC() == 2 || args.ArgC() == 3 )
		{
			// if team # specified, use that
			iTeam = atoi( args[1] );
		}
		else
		{
			Msg( "Usage: mp_forcewin <opt: team#>\n" );
			return;
		}

		int iWinReason;
		if ( iTeam == TEAM_UNASSIGNED )
		{
			iWinReason = WINREASON_STALEMATE;
		}
		else
		{
			iWinReason = WINREASON_ALL_POINTS_CAPTURED;
			if ( args.ArgC() == 3 )
			{
				int iSpecifiedReason = atoi( args[2] );
				if ( iSpecifiedReason < WINREASON_COUNT )
				{
					iWinReason = iSpecifiedReason;
				}
			}
		}
		pRules->SetWinningTeam( iTeam, iWinReason );
	}
}

#endif // GAME_DLL

// Utility function
bool FindInList( const char **pStrings, const char *pToFind )
{
	int i = 0;
	while ( pStrings[i][0] != 0 )
	{
		if ( Q_stricmp( pStrings[i], pToFind ) == 0 )
			return true;
		i++;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamplayRoundBasedRules::CTeamplayRoundBasedRules( void )
{
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		m_flNextRespawnWave.Set( i, 0 );
		m_TeamRespawnWaveTimes.Set( i, -1.0f );
		m_bTeamReady.Set( i, false );
			
#ifdef GAME_DLL
		m_flOriginalTeamRespawnWaveTime[i] = -1.0f;
#endif
	}

	m_flStopWatchTotalTime = -1.0f;
	SetAllowBetweenRounds( true );

	m_iRoundState.Set( GR_STATE_INIT );
	m_bInOvertime.Set( false );
	m_bInSetup.Set( false );
	m_iSetupTime.Set( 0 );
	m_bSwitchedTeamsThisRound.Set( false );
	m_iWinningTeam.Set( TEAM_UNASSIGNED );
	m_iWinReason.Set( WINREASON_NONE );
	m_bInWaitingForPlayers.Set( false );
	m_bAwaitingReadyRestart.Set( false );
	m_flRestartRoundTime.Set( -1.0f );
	m_flRestartRoundStartTime.Set( -1.0f );
	m_flMapResetTime.Set( 0.0f );
	m_bStopWatch.Set( false );
	m_bMultipleTrains.Set( false );
	m_bCheatsEnabledDuringLevel.Set( false );
	m_nRoundsPlayed.Set( 0 );
	m_flCountdownTime.Set( -1.0f );
	m_nTotalPausedTicks.Set( 0 );
	m_nPauseStartTick.Set( -1 );
	m_bGamePaused.Set( false );
	m_bPauseEnabled.Set( true );
	m_flUnpauseCurTime.Set( -1.0f );
	m_flPauseTime = 0.0f;
	m_flPauseCurTime = -1.0f;
	m_bForcePause = false;

	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		m_bPlayerReady.Set( i, false );
	}

#ifdef GAME_DLL
	ListenForGameEvent( "server_changelevel_failed" );

	m_pCurStateInfo = NULL;
	State_Transition( GR_STATE_PREGAME );

	m_bResetTeamScores = true;
	m_bResetPlayerScores = true;
	m_bResetRoundsPlayed = true;
	InitTeams();
	ResetMapTime();
	ResetScores();
	SetForceMapReset( true );
	SetRoundToPlayNext( NULL_STRING );

	m_bPrevRoundWasWaitingForPlayers = false;
  	m_iszPreviousRounds.RemoveAll();
	SetFirstRoundPlayed( NULL_STRING );

	m_bAllowStalemateAtTimelimit = false;
	m_bChangelevelAfterStalemate = false;
	m_flRoundStartTime = 0.0f;
	m_flNewThrottledAlertTime = 0.0f;
	m_flStartBalancingTeamsAt = 0.0f;
	m_bPrintedUnbalanceWarning = false;
	m_flFoundUnbalancedTeamsTime = -1.0f;
	m_flWaitingForPlayersTimeEnds = -1.0f;
	m_flLastTeamWin = -1.0f;
	m_bUseAddScoreAnim = false;

	if ( IsInTournamentMode() )
	{
		m_bAwaitingReadyRestart.Set( true );
	}

	m_flAutoBalanceQueueTimeEnd = -1.0f;
	m_nAutoBalanceQueuePlayerIndex = -1;
	m_nAutoBalanceQueuePlayerScore = -1;

	SetDefLessFunc( m_GameTeams );

	ResetPlayerAndTeamReadyState();

	m_nLastEventFiredTime = 0.f;

	m_hWaitingForPlayersTimer = NULL;
	m_bStopWatchShouldBeTimedWin = false;
#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetTeamRespawnWaveTime( int iTeam, float flValue ) 
{ 
	if ( flValue < 0 )
	{
		flValue = 0;
	}

	// initialized to -1 so we can try to determine if this is the first spawn time we have received for this team
	if ( m_flOriginalTeamRespawnWaveTime[iTeam] < 0 )
	{
		m_flOriginalTeamRespawnWaveTime[iTeam] = flValue;
	}

	m_TeamRespawnWaveTimes.Set( iTeam, flValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::AddTeamRespawnWaveTime( int iTeam, float flValue ) 
{ 
	float flAddAmount = flValue;
	float flCurrentSetting = m_TeamRespawnWaveTimes[iTeam];
	float flNewValue;

	if ( flCurrentSetting < 0 )
	{
		flCurrentSetting = mp_respawnwavetime.GetFloat();
	}

	// initialized to -1 so we can try to determine if this is the first spawn time we have received for this team
	if ( m_flOriginalTeamRespawnWaveTime[iTeam] < 0 )
	{
		m_flOriginalTeamRespawnWaveTime[iTeam] = flCurrentSetting;
	}

	flNewValue = flCurrentSetting + flAddAmount;

	if ( flNewValue < 0 )
	{
		flNewValue = 0;
	}

	m_TeamRespawnWaveTimes.Set( iTeam, flNewValue );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: don't let us spawn before our freezepanel time would have ended, even if we skip it
//-----------------------------------------------------------------------------
float CTeamplayRoundBasedRules::GetNextRespawnWave( int iTeam, CBasePlayer *pPlayer ) 
{ 
	if ( State_Get() == GR_STATE_STALEMATE )
		return 0;

	// If we are purely checking when the next respawn wave is for this team
	if ( pPlayer == NULL )
	{
		return GetNextRespawnWave(iTeam);
	}

	// The soonest this player may spawn
	float flMinSpawnTime = GetMinTimeWhenPlayerMaySpawn( pPlayer );
	if ( ShouldRespawnQuickly( pPlayer ) )
	{
		return flMinSpawnTime;
	}

	// the next scheduled respawn wave time
	float flNextRespawnTime = GetNextRespawnWave( iTeam );

	// the length of one respawn wave. We'll check in increments of this
	float flRespawnWaveMaxLen = GetRespawnWaveMaxLength( iTeam );

	if ( flRespawnWaveMaxLen <= 0 )
	{
		return flNextRespawnTime;
	}

	// Keep adding the length of one respawn until we find a wave that
	// this player will be eligible to spawn in.
	while ( flNextRespawnTime < flMinSpawnTime )
	{
		flNextRespawnTime += flRespawnWaveMaxLen; 
	}

	return flNextRespawnTime; 
}

//-----------------------------------------------------------------------------
// Purpose: Is the player past the required delays for spawning
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::HasPassedMinRespawnTime( CBasePlayer *pPlayer )
{
	float flMinSpawnTime = GetMinTimeWhenPlayerMaySpawn( pPlayer ); 

	return ( gpGlobals->curtime > flMinSpawnTime );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTeamplayRoundBasedRules::GetMinTimeWhenPlayerMaySpawn( CBasePlayer *pPlayer )
{
	// Min respawn time is the sum of
	//
	// a) the length of one full *unscaled* respawn wave for their team
	//		and
	// b) death anim length + freeze panel length

	const int iRespawnTimeMode = GetRespawnTimeMode();
	const float flDeathAnimLength = iRespawnTimeMode >= 2 ? 0.5f : ( TF_DEATH_ANIMATION_TIME + spec_freeze_traveltime.GetFloat() + spec_freeze_time.GetFloat() );
	float fMinDelay = flDeathAnimLength;

	if ( !ShouldRespawnQuickly( pPlayer ) )
	{
		fMinDelay += GetRespawnWaveMaxLength( pPlayer->GetTeamNumber(), false );
	}

	return pPlayer->GetDeathTime() + fMinDelay;
}

int CTeamplayRoundBasedRules::GetRespawnTimeMode()
{
	return mp_disable_respawn_times.GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::LevelInitPostEntity( void )
{
	BaseClass::LevelInitPostEntity();

#ifdef GAME_DLL
	m_bCheatsEnabledDuringLevel = sv_cheats && sv_cheats->GetBool();
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTeamplayRoundBasedRules::GetRespawnTimeScalar( int iTeam )
{
	// For long respawn times, scale the time as the number of players drops
	int iOptimalPlayers = 8;	// 16 players total, 8 per team

	int iNumPlayers = GetGlobalTeam(iTeam)->GetNumPlayers();

	float flScale = RemapValClamped( iNumPlayers, 1, iOptimalPlayers, 0.25, 1.0 );
	return flScale;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::FireGameEvent( IGameEvent * event )
{
#ifdef GAME_DLL
	const char *eventName = event->GetName();
	if ( g_fGameOver && !Q_strcmp( eventName, "server_changelevel_failed" ) )
	{
		Warning( "In gameover, but failed to load the next map. Trying next map in cycle.\n" );
		nextlevel.SetValue( "" );
		ChangeLevel();
	}
#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetForceMapReset( bool reset )
{
	m_bForceMapReset = reset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::Think( void )
{
	if ( !IsGamePaused() )
	{
		if ( m_flPauseCurTime > 0.0f && m_flPauseCurTime <= gpGlobals->curtime && ( m_pausingPlayerId.IsValid() || m_bForcePause ) )
		{
			m_flUnpauseCurTime = -1.0f;
			m_flPauseCurTime = -1.0f;
			m_bForcePause = false;
			m_unpausingPlayerId.Clear();
			m_bGamePaused = true;
		}
	}

	if ( IsGamePaused() )
	{
		m_flPauseTime += gpGlobals->frametime;

		if ( m_flPauseTime > mp_pause_force_unpause_time.GetFloat() || ( m_flUnpauseCurTime > 0.0f && m_flUnpauseCurTime <= gpGlobals->curtime && ( m_pausingPlayerId.IsValid() || m_bForcePause ) ) )
		{
			m_bForcePause = false;
			m_pausingPlayerId.Clear();
			m_bGamePaused = false;
			m_flPauseTime = 0.0f;
		}
		
		return;
	}

	if ( g_fGameOver )   // someone else quit the game already
	{
		// check to see if we should change levels now
		if ( m_flIntermissionEndTime && ( m_flIntermissionEndTime < gpGlobals->curtime ) )
		{
			if ( !IsX360() )
			{
				ChangeLevel(); // intermission is over
			}
			else
			{
				IGameEvent * event = gameeventmanager->CreateEvent( "player_stats_updated" );
				if ( event )
				{
					event->SetBool( "forceupload", true );
					gameeventmanager->FireEvent( event );
				}
				engine->MultiplayerEndGame();
			}

			// Don't run this code again
			m_flIntermissionEndTime = 0.f;
		}

		return;
	}

	State_Think();

	if ( m_hWaitingForPlayersTimer )
	{
		Assert( m_bInWaitingForPlayers );
	}

	if ( gpGlobals->curtime > m_flNextPeriodicThink )
	{
		// Don't end the game during win or stalemate states
		if ( State_Get() != GR_STATE_TEAM_WIN && State_Get() != GR_STATE_STALEMATE && State_Get() != GR_STATE_GAME_OVER ) 
		{
			if ( CheckWinLimit() )
				return;

			if ( CheckMaxRounds() )
				return;
		}

		CheckRestartRound();
		CheckWaitingForPlayers();

		m_flNextPeriodicThink = gpGlobals->curtime + 1.0;
	}

	// Watch dog for cheats ever being enabled during a level
	if ( !m_bCheatsEnabledDuringLevel && sv_cheats && sv_cheats->GetBool() )
	{
		m_bCheatsEnabledDuringLevel = true;
	}

	// Bypass teamplay think.
	CGameRules::Think();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::TimerMayExpire( void )
{
#ifndef CSTRIKE_DLL
	// team_train_watchers can also prevent timer expiring ( overtime )
	CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher*>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
	while ( pWatcher )
	{
		if ( !pWatcher->TimerMayExpire() )
		{
			return false;
		}

		pWatcher = dynamic_cast<CTeamTrainWatcher*>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
	}
#endif

	return BaseClass::TimerMayExpire();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CheckChatText( CBasePlayer *pPlayer, char *pText )
{
	CheckChatForReadySignal( pPlayer, pText );

	BaseClass::CheckChatText( pPlayer, pText );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CheckChatForReadySignal( CBasePlayer *pPlayer, const char *chatmsg )
{
	if ( IsInTournamentMode() == false )
	{	
		if( IsWaitingForTeams() && FStrEq( chatmsg, mp_clan_ready_signal.GetString() ) )
		{
			int iTeam = pPlayer->GetTeamNumber();
			if ( iTeam > LAST_SHARED_TEAM && iTeam < GetNumberOfTeams() )
			{
				SetTeamReadyState( true, iTeam );

				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_team_ready" );
				if ( event )
				{
					event->SetInt( "team", iTeam );
					gameeventmanager->FireEvent( event );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::GoToIntermission( void )
{
	if ( IsInTournamentMode() == true )
		return;

	BaseClass::GoToIntermission();

	// set all players to FL_FROZEN
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer )
		{
			pPlayer->AddFlag( FL_FROZEN );
		}
	}

	// Print out map stats to a text file
	//WriteStatsFile( "stats.xml" );

	State_Enter( GR_STATE_GAME_OVER );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetInWaitingForPlayers( bool bWaitingForPlayers  )
{
	// never waiting for players when loading a bug report
	if ( IsLoadingBugBaitReport() || gpGlobals->eLoadType == MapLoad_Background )
	{
		m_bInWaitingForPlayers = false;
		return;
	}

	if( m_bInWaitingForPlayers == bWaitingForPlayers  )
		return;

	if ( IsInArenaMode() && ( m_flWaitingForPlayersTimeEnds < 0 ) && !IsInTournamentMode() )
	{
		m_bInWaitingForPlayers = false;
		return;
	}

	m_bInWaitingForPlayers = bWaitingForPlayers;

	if ( m_bInWaitingForPlayers )
	{
		m_flWaitingForPlayersTimeEnds = gpGlobals->curtime + mp_waitingforplayers_time.GetFloat();
	}
	else
	{
		m_flWaitingForPlayersTimeEnds = -1.0f;

		if ( m_hWaitingForPlayersTimer )
		{
			UTIL_Remove( m_hWaitingForPlayersTimer );
		}

		RestoreActiveTimer();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetOvertime( bool bOvertime ) 
{ 
	if ( m_bInOvertime == bOvertime )
		return;

	if ( bOvertime )
	{
		UTIL_LogPrintf( "World triggered \"Round_Overtime\"\n" );
	}

	m_bInOvertime = bOvertime;

	if ( m_bInOvertime )
	{
		// tell train watchers that we've transitioned to overtime

#ifndef CSTRIKE_DLL
		CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher*>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
		while ( pWatcher )
		{
			variant_t emptyVariant;
			pWatcher->AcceptInput( "OnStartOvertime", NULL, NULL, emptyVariant, 0 );

			pWatcher = dynamic_cast<CTeamTrainWatcher*>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetSetup( bool bSetup ) 
{ 
	if ( m_bInSetup == bSetup )
		return;

	m_bInSetup = bSetup;
	if ( bSetup )
	{
		m_iSetupTime = GetActiveRoundTimer() ? GetActiveRoundTimer()->GetSetupTimeLength() : 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CheckWaitingForPlayers( void )
{
	// never waiting for players when loading a bug report, or training
	if ( IsLoadingBugBaitReport() || gpGlobals->eLoadType == MapLoad_Background || !AllowWaitingForPlayers() )
		return;

	if ( mp_waitingforplayers_restart.GetBool() )
	{
		if ( m_bInWaitingForPlayers )
		{
			m_flWaitingForPlayersTimeEnds = gpGlobals->curtime + mp_waitingforplayers_time.GetFloat();

			if ( m_hWaitingForPlayersTimer )
			{
				variant_t sVariant;
				sVariant.SetInt( m_flWaitingForPlayersTimeEnds - gpGlobals->curtime );
				m_hWaitingForPlayersTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			}
		}
		else
		{
			SetInWaitingForPlayers( true );
		}

		mp_waitingforplayers_restart.SetValue( 0 );
	}

	bool bCancelWait = ( mp_waitingforplayers_cancel.GetBool() || IsInItemTestingMode() ) && !IsInTournamentMode();

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		bCancelWait = true;
#endif // _DEBUG || STAGING_ONLY

	if ( bCancelWait )
	{
		// Cancel the wait period and manually Resume() the timer if 
		// it's not supposed to start paused at the beginning of a round.
		// We must do this before SetInWaitingForPlayers() is called because it will
		// restore the timer in the HUD and set the handle to NULL
#ifndef CSTRIKE_DLL
		if ( m_hPreviousActiveTimer.Get() )
		{
			CTeamRoundTimer *pTimer = dynamic_cast<CTeamRoundTimer*>( m_hPreviousActiveTimer.Get() );
			if ( pTimer && !pTimer->StartPaused() )
			{
				pTimer->ResumeTimer();
			}
		}
#endif
		SetInWaitingForPlayers( false );
		mp_waitingforplayers_cancel.SetValue( 0 );
	}

	if( m_bInWaitingForPlayers )
	{
		if ( IsInTournamentMode() == true )
			return;

		// only exit the waitingforplayers if the time is up, and we are not in a round
		// restart countdown already, and we are not waiting for a ready restart
		if( gpGlobals->curtime > m_flWaitingForPlayersTimeEnds && m_flRestartRoundTime < 0 && !IsWaitingForTeams() )
		{
			m_flRestartRoundTime.Set( gpGlobals->curtime );	// reset asap
			m_flRestartRoundStartTime.Set( -1.0f );

			if ( IsInArenaMode() == true )
			{
				if ( gpGlobals->curtime > m_flWaitingForPlayersTimeEnds )
				{
					SetInWaitingForPlayers( false );
					State_Transition( GR_STATE_PREROUND );
				}

				return;
			}

			// if "waiting for players" is ending and we're restarting...
			// keep the current round that we're already running around in as the first round after the restart
			CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
			if ( pMaster && pMaster->PlayingMiniRounds() && pMaster->GetCurrentRound() )
			{
				SetRoundToPlayNext( pMaster->GetRoundToUseAfterRestart() );
			}
		}
		else
		{
			if ( !m_hWaitingForPlayersTimer )
			{
				// Stop any timers, and bring up a new one
				HideActiveTimer();

#ifndef CSTRIKE_DLL
				variant_t sVariant;
				m_hWaitingForPlayersTimer = (CTeamRoundTimer*)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
				m_hWaitingForPlayersTimer->SetName( MAKE_STRING("zz_teamplay_waiting_timer") );
				m_hWaitingForPlayersTimer->KeyValue( "show_in_hud", "1" );
				sVariant.SetInt( m_flWaitingForPlayersTimeEnds - gpGlobals->curtime );
				m_hWaitingForPlayersTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
				m_hWaitingForPlayersTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
				m_hWaitingForPlayersTimer->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
#endif
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CheckRestartRound( void )
{
	if( mp_clan_readyrestart.GetBool() && IsInTournamentMode() == false )
	{
		m_bAwaitingReadyRestart = true;

		for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
		{
			SetTeamReadyState( false, i );
		}

		const char *pszReadyString = mp_clan_ready_signal.GetString();

		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#clan_ready_rules", pszReadyString );
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#clan_ready_rules", pszReadyString );

		// Don't let them put anything malicious in there
		if( pszReadyString == NULL || Q_strlen(pszReadyString) > 16 )
		{
			pszReadyString = "ready";
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_ready_restart" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}

		mp_clan_readyrestart.SetValue( 0 );

		// cancel any restart round in progress
		m_flRestartRoundTime.Set( -1.0f );
		m_flRestartRoundStartTime.Set( -1.0f );
	}

	// Restart the game if specified by the server
	int iRestartDelay = mp_restartround.GetInt();
	bool bRestartGameNow = mp_restartgame_immediate.GetBool();
	if ( iRestartDelay == 0 && !bRestartGameNow )
	{
		iRestartDelay = mp_restartgame.GetInt();
	}

	if ( iRestartDelay > 0 || bRestartGameNow )
	{
		int iDelayMax = 60;

#ifdef TF_DLL
		if ( TFGameRules() && ( TFGameRules()->IsMannVsMachineMode() || TFGameRules()->IsCompetitiveMode() || TFGameRules()->IsEmulatingMatch() || TFGameRules()->UsePlayerReadyStatusMode() ) )
		{
			iDelayMax = 150;
		}
#endif // #if defined(TF_CLIENT_DLL) || defined(TF_DLL)

		if ( iRestartDelay > iDelayMax )
		{
			iRestartDelay = iDelayMax;
		}

		if ( mp_restartgame.GetInt() > 0 || bRestartGameNow )
		{
			SetForceMapReset( true );
		}
		else
		{
			SetForceMapReset( false );
		}

		SetInStopWatch( false );

		// cannot enter between rounds anymore.
		SetAllowBetweenRounds( false );

		if ( bRestartGameNow )
		{
			iRestartDelay = 0;
		}

		m_flRestartRoundTime.Set( gpGlobals->curtime + iRestartDelay );
		m_flRestartRoundStartTime.Set( gpGlobals->curtime );

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_restart_seconds" );
		if ( event )
		{
			event->SetInt( "seconds", iRestartDelay );
			gameeventmanager->FireEvent( event );
		}

		if ( IsInTournamentMode() == false )
		{
			// let the players know
			const char *pFormat = NULL;

			if ( mp_restartgame.GetInt() > 0 )
			{
				if ( ShouldSwitchTeams() )
				{
					pFormat = ( iRestartDelay > 1 ) ? "#game_switch_in_secs" : "#game_switch_in_sec";
				}
				else if ( ShouldScrambleTeams() )
				{
					pFormat = ( iRestartDelay > 1 ) ? "#game_scramble_in_secs" : "#game_scramble_in_sec";

#ifdef TF_DLL
					IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_alert" );
					if ( event )
					{
						event->SetInt( "alert_type", HUD_ALERT_SCRAMBLE_TEAMS );
						gameeventmanager->FireEvent( event );
					}

					pFormat = NULL;
#endif
				}
			}
			else if ( mp_restartround.GetInt() > 0 )
			{
				pFormat = ( iRestartDelay > 1 ) ? "#round_restart_in_secs" : "#round_restart_in_sec";
			}

			if ( pFormat )
			{
				char strRestartDelay[64];
				Q_snprintf( strRestartDelay, sizeof( strRestartDelay ), "%d", iRestartDelay );
				UTIL_ClientPrintAll( HUD_PRINTCENTER, pFormat, strRestartDelay );
				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, pFormat, strRestartDelay );
			}
		}

		mp_restartround.SetValue( 0 );
		mp_restartgame.SetValue( 0 );
		mp_restartgame_immediate.SetValue( 0 );

		// cancel any ready restart in progress
		m_bAwaitingReadyRestart = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::CheckTimeLimit( bool bAllowEnd /*= true*/ )
{
	if ( IsInPreMatch() == true )
		return false;

	if ( ( mp_timelimit.GetInt() > 0 && CanChangelevelBecauseOfTimeLimit() ) || m_bChangelevelAfterStalemate )
	{
		// If there's less than 5 minutes to go, just switch now. This avoids the problem
		// of sudden death modes starting shortly after a new round starts.
		const int iMinTime = 5;
		bool bSwitchDueToTime = ( mp_timelimit.GetInt() > iMinTime && GetTimeLeft() < (iMinTime * 60) );

		if ( IsInTournamentMode() == true  )
		{
			if ( TournamentModeCanEndWithTimelimit() == false )
			{
				return false;
			}

#ifdef TF_DLL
			// if we're just checking for time limit, then it's not a win condition on its own for multi-series.
			if ( !bAllowEnd && GetTimeLeft() <= 0 )
			{
				if ( TFGameRules() )
				{
					ETFMatchGroup eMatchGroup = TFGameRules()->GetCurrentMatchGroupWithEmulation();
					if ( GetMatchGroupDescription( eMatchGroup ) && GetMatchGroupDescription( eMatchGroup )->BUsesMultiSeries() && !TFGameRules()->IsCommunityGameMode() )
					{
						return false;
					}
				}
			}
#endif

			bSwitchDueToTime = false;
		}

		if ( IsInArenaMode() == true )
		{
			bSwitchDueToTime = false;
		}

		if ( GetTimeLeft() <= 0 || m_bChangelevelAfterStalemate || bSwitchDueToTime )
		{
			if ( mp_timelimit_added_winlimit.GetInt() > 0 )
			{
				int nHighestScore = 0;
				for ( int i = LAST_SHARED_TEAM + 1; i < GetNumberOfTeams(); i++ )
				{
					CTeam *pTeam = GetGlobalTeam( i );
					if ( pTeam && pTeam->GetScore() > nHighestScore )
					{
						nHighestScore = pTeam->GetScore();
					}
				}

				int nAddedWinLimit = mp_timelimit_added_winlimit.GetInt();
				mp_winlimit.SetValue( nHighestScore + nAddedWinLimit );

				mp_timelimit.SetValue( 0 ); // Disable the time limit so it doesn't trigger again

				char strMessage[128];
				Q_snprintf( strMessage, sizeof( strMessage ), "Time limit reached! First team to %d wins the match!", mp_winlimit.GetInt() );

				UTIL_ClientPrintAll( HUD_PRINTCENTER, strMessage );
				UTIL_ClientPrintAll( HUD_PRINTTALK, strMessage );

				return false;
			}

			if ( bAllowEnd )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Time Limit" );
					gameeventmanager->FireEvent( event );
				}

				SendTeamScoresEvent();

				GoToIntermission();
			}
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::IsGameUnderTimeLimit( void )
{
	return ( mp_timelimit.GetInt() > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamplayRoundBasedRules::GetTimeLeft( void )
{
	float flTimeLimit = mp_timelimit.GetInt() * 60;
	float flMapChangeTime = m_flMapResetTime + flTimeLimit;

	// If the round timer is longer, let the round complete
	// TFTODO: Do we need to worry about the timelimit running our during a round?

	int iTime = (int)(flMapChangeTime - gpGlobals->curtime);
	if ( iTime < 0 )
	{
		iTime = 0;
	}

	return ( iTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::CheckNextLevelCvar( bool bAllowEnd /*= true*/ )
{
	ETFMatchGroup eMatchGroup = TFGameRules()->GetCurrentMatchGroupWithEmulation();
	bool bMultiSeries = GetMatchGroupDescription( eMatchGroup ) && GetMatchGroupDescription( eMatchGroup )->BUsesMultiSeries();
	if ( bMultiSeries )
	{
		// cannot force the game to be over when next level is set during multi-series.
		return false;
	}

	if ( m_bForceMapReset )
	{
		if ( nextlevel.GetString() && *nextlevel.GetString() )
		{
			if ( bAllowEnd )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
				if ( event )
				{
					event->SetString( "reason", "NextLevel CVAR" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
			}
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::CheckWinLimit( bool bAllowEnd /*= true*/, int nAddValueWhenChecking /*= 0*/ )
{
	// has one team won the specified number of rounds?
	int iWinLimit = mp_winlimit.GetInt();

	if ( iWinLimit > 0 )
	{
		for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
		{
			CTeam *pTeam = GetGlobalTeam(i);
			Assert( pTeam );

			if ( ( pTeam->GetScore() + nAddValueWhenChecking ) >= iWinLimit )
			{
				if ( bAllowEnd )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
					if ( event )
					{
						event->SetString( "reason", "Reached Win Limit" );
						gameeventmanager->FireEvent( event );
					}

					GoToIntermission();
				}
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::CheckMaxRounds( bool bAllowEnd /*= true*/, int nAddValueWhenChecking /*= 0*/ )
{
	if ( mp_maxrounds.GetInt() > 0 && IsInPreMatch() == false )
	{
		if ( ( m_nRoundsPlayed + nAddValueWhenChecking ) >= mp_maxrounds.GetInt() )
		{
			if ( bAllowEnd )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Round Limit" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
			}
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Transition( gamerules_roundstate_t newState )
{
	m_prevState = State_Get();

	State_Leave();
	State_Enter( newState );

}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter( gamerules_roundstate_t newState )
{
	m_iRoundState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	m_flLastRoundStateChangeTime = gpGlobals->curtime;

	if ( mp_showroundtransitions.GetInt() > 0 )
	{
		if ( m_pCurStateInfo )
			Msg( "Gamerules: entering state '%s'\n", m_pCurStateInfo->m_pStateName );
		else
			Msg( "Gamerules: entering state #%d\n", newState );
	}

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
	{
		(this->*m_pCurStateInfo->pfnEnterState)();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		if ( mp_showroundtransitions.GetInt() > 0 )
		{
			Msg( "Gamerules: leaving state '%s'\n", m_pCurStateInfo->m_pStateName );
		}
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnThink )
	{
		(this->*m_pCurStateInfo->pfnThink)();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGameRulesRoundStateInfo* CTeamplayRoundBasedRules::State_LookupInfo( gamerules_roundstate_t state )
{
	static CGameRulesRoundStateInfo playerStateInfos[] =
	{
		{ GR_STATE_INIT,		"GR_STATE_INIT",		&CTeamplayRoundBasedRules::State_Enter_INIT, NULL, &CTeamplayRoundBasedRules::State_Think_INIT },
		{ GR_STATE_PREGAME,		"GR_STATE_PREGAME",		&CTeamplayRoundBasedRules::State_Enter_PREGAME, NULL, &CTeamplayRoundBasedRules::State_Think_PREGAME },
		{ GR_STATE_STARTGAME,	"GR_STATE_STARTGAME",	&CTeamplayRoundBasedRules::State_Enter_STARTGAME, NULL, &CTeamplayRoundBasedRules::State_Think_STARTGAME },
		{ GR_STATE_PREROUND,	"GR_STATE_PREROUND",	&CTeamplayRoundBasedRules::State_Enter_PREROUND, &CTeamplayRoundBasedRules::State_Leave_PREROUND, &CTeamplayRoundBasedRules::State_Think_PREROUND },
		{ GR_STATE_RND_RUNNING,	"GR_STATE_RND_RUNNING",	&CTeamplayRoundBasedRules::State_Enter_RND_RUNNING, NULL,	&CTeamplayRoundBasedRules::State_Think_RND_RUNNING },
		{ GR_STATE_TEAM_WIN,	"GR_STATE_TEAM_WIN",	&CTeamplayRoundBasedRules::State_Enter_TEAM_WIN, NULL,	&CTeamplayRoundBasedRules::State_Think_TEAM_WIN },
		{ GR_STATE_RESTART,		"GR_STATE_RESTART",		&CTeamplayRoundBasedRules::State_Enter_RESTART,	NULL, &CTeamplayRoundBasedRules::State_Think_RESTART },
		{ GR_STATE_STALEMATE,	"GR_STATE_STALEMATE",	&CTeamplayRoundBasedRules::State_Enter_STALEMATE,	&CTeamplayRoundBasedRules::State_Leave_STALEMATE, &CTeamplayRoundBasedRules::State_Think_STALEMATE },
		{ GR_STATE_GAME_OVER,	"GR_STATE_GAME_OVER",	NULL, NULL, NULL },
		{ GR_STATE_BONUS,		"GR_STATE_BONUS",		&CTeamplayRoundBasedRules::State_Enter_BONUS, &CTeamplayRoundBasedRules::State_Leave_BONUS,	&CTeamplayRoundBasedRules::State_Think_BONUS },
		{ GR_STATE_BETWEEN_RNDS, "GR_STATE_BETWEEN_RNDS", &CTeamplayRoundBasedRules::State_Enter_BETWEEN_RNDS, &CTeamplayRoundBasedRules::State_Leave_BETWEEN_RNDS,	&CTeamplayRoundBasedRules::State_Think_BETWEEN_RNDS },
	};

	for ( int i=0; i < ARRAYSIZE( playerStateInfos ); i++ )
	{
		if ( playerStateInfos[i].m_iRoundState == state )
			return &playerStateInfos[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_INIT( void )
{
	InitTeams();
	ResetMapTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_INIT( void )
{
	State_Transition( GR_STATE_PREGAME );
}

//-----------------------------------------------------------------------------
// Purpose: The server is idle and waiting for enough players to start up again. 
//			When we find an active player go to GR_STATE_STARTGAME.
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_PREGAME( void )
{
	m_flNextPeriodicThink = gpGlobals->curtime + 0.1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_PREGAME( void )
{
	CheckRespawnWaves();

	// we'll just stay in pregame for the bugbait reports
	if ( IsLoadingBugBaitReport() || gpGlobals->eLoadType == MapLoad_Background )
		return;

	// Commentary stays in this mode too
	if ( IsInCommentaryMode() )
		return;

	if ( ( BHavePlayers() ) || ( IsInArenaMode() && ( m_flWaitingForPlayersTimeEnds == 0.f ) ) )
	{
		State_Transition( GR_STATE_STARTGAME );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Wait a bit and then spawn everyone into the preround
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_STARTGAME( void )
{
	// mark as the game starting for the first round
	m_bInitialSpawn = true;

	// if the gamerules wants to override and delay start, let it
	if ( StartGame_Start() )
	{
		CompleteStartGame();
	}
	else
	{
		m_flStateTransitionTime = -1;
	}
}

void CTeamplayRoundBasedRules::CompleteStartGame( void )
{
	m_flStateTransitionTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_STARTGAME()
{
	if ( m_flStateTransitionTime >= 0 && gpGlobals->curtime >= m_flStateTransitionTime )
	{
		if ( !IsInTraining() && !IsInItemTestingMode() )
		{
			static ConVarRef tf_bot_offline_practice( "tf_bot_offline_practice" );
			if ( mp_waitingforplayers_time.GetFloat() > 0 && tf_bot_offline_practice.GetInt() == 0 )
			{
				// go into waitingforplayers, reset at end of it
				SetInWaitingForPlayers( true );
			}
		}

		State_Transition( GR_STATE_PREROUND );

		return;
	}

	StartGame_Think();
}
	
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_PREROUND( void )
{
	BalanceTeams( false );

	m_flStartBalancingTeamsAt = gpGlobals->curtime + 60.0f;

	bool bDoRoundRespawn = true;
#ifdef TF_DLL
	if ( ( TFGameRules()->IsCompetitiveMode() || TFGameRules()->IsEmulatingMatch() ) && GetRoundsPlayed() == 0 && !m_bAllowBetweenRounds )
	{
		CTeamControlPointMaster* pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		if ( !pMaster || !pMaster->PlayingMiniRounds() || ( pMaster->GetCurrentRoundIndex() == 0 ) )
		{
			// we already did a round respawn in this case.
			bDoRoundRespawn = false;
		}
	}
#endif
	if ( bDoRoundRespawn )
	{
		RoundRespawn();
	}
	else if ( GetActiveRoundTimer() && GetActiveRoundTimer()->GetSetupTimeLength() > 0 )
	{
		// if we have setup time, then we need to activate it here, because BetweenRounds_End ends setup.
		SetSetup( true );
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_start" );
	if ( event )
	{
		event->SetBool( "full_reset", m_bForceMapReset );
		gameeventmanager->FireEvent( event );
	}

	if ( IsInArenaMode() )
	{
		if ( BHavePlayers() )
		{
#ifndef CSTRIKE_DLL
			variant_t sVariant;
			if ( !m_hStalemateTimer )
			{
				m_hStalemateTimer = (CTeamRoundTimer*)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
			}
			m_hStalemateTimer->KeyValue( "show_in_hud", "1" );

			sVariant.SetInt( tf_arena_preround_time.GetInt() );

			m_hStalemateTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
			m_hStalemateTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
			m_hStalemateTimer->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
#endif
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
			if ( event )
			{
				gameeventmanager->FireEvent( event );
			}
		}

		m_flStateTransitionTime = gpGlobals->curtime + tf_arena_preround_time.GetInt();
	}
#ifdef TF_DLL
	// Only allow at the very beginning of the game, or between waves in mvm
	else if ( TFGameRules() && TFGameRules()->UsePlayerReadyStatusMode() && m_bAllowBetweenRounds )
	{
		State_Transition( GR_STATE_BETWEEN_RNDS );
		SetAllowBetweenRounds( false );

		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			TFObjectiveResource()->SetMannVsMachineBetweenWaves( true );
		}

		StopWatchModeThink();
		return;
	}
#endif // TF_DLL
	else
	{
		float flTransitionTime = 3 * mp_enableroundwaittime.GetFloat();
#ifdef TF_DLL
		if ( TFGameRules() && ( TFGameRules()->IsCompetitiveMode() || TFGameRules()->IsEmulatingMatch() ) )
		{
			// TODO(mcoms): maybe needs some work?
			flTransitionTime = 3.0f;
			m_flCountdownTime = -1.f;
			// reset spawn points
			extern EHANDLE g_pLastSpawnPoints[TF_TEAM_COUNT];
			for ( int i = 0; i < TF_TEAM_COUNT; i++ )
			{
				g_pLastSpawnPoints[i].Term();
			}
			if ( !( GetActiveRoundTimer() && ( GetActiveRoundTimer()->GetSetupTimeLength() > 0 ) ) )
			{
				if ( ( TFGameRules()->GetRoundsPlayed() > 0 ) )
				{
					// we do a countdown after the first round, so we need some extra pre-round time
					if ( TFGameRules()->IsCompetitiveGame() )
					{
						flTransitionTime += TOURNAMENT_NOCANCEL_TIME;
						m_flCountdownTime = gpGlobals->curtime + TOURNAMENT_NOCANCEL_TIME;
					}
					else
					{
						flTransitionTime += TOURNAMENT_NOCANCEL_TIME - 5.0f;
						m_flCountdownTime = gpGlobals->curtime + TOURNAMENT_NOCANCEL_TIME - 5.0f;
					}
				}
			}
		}
		m_flStateTransitionTime = gpGlobals->curtime + flTransitionTime;
		if ( flTransitionTime > 0.0f )
		{
			// set speed at start of pre-round. previously we only did this if there was an active roundtimer and in competitive
			// but it is completely safe to refresh speed calculation at the start of pre-round. it happens anyway upon weapon switch.
			CTFPlayer *pPlayer;
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

				if ( !pPlayer )
					continue;

				if ( pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
					continue;

				pPlayer->TeamFortress_SetSpeed();
			}
		}
#else
		m_flStateTransitionTime = gpGlobals->curtime + flTransitionTime;
#endif // TF_DLL
	}

	StopWatchModeThink();

	PreRound_Start();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Leave_PREROUND( void )
{
	PreRound_End();

	// no longer the first round
	m_bInitialSpawn = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_PREROUND( void )
{
	if( gpGlobals->curtime > m_flStateTransitionTime )
	{
		if ( IsInArenaMode() == true )
		{
			if ( IsInWaitingForPlayers() == true )
			{
				if ( IsInTournamentMode() == true )
				{
					// check round restart
					CheckReadyRestart();
					State_Transition( GR_STATE_STALEMATE );
				}

				return;
			}

			State_Transition( GR_STATE_STALEMATE );	

			// hide the class composition panel
		}
		else
		{
			State_Transition( GR_STATE_RND_RUNNING );
		}
	}
#ifdef TF_DLL
	else
	{
		// otherwise it's most likely freeze time. prerounds are short enough anyway
		// if there isn't a freeze that this is likely safe to do.
		CTFPlayer *pPlayer;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( !pPlayer )
				continue;

			if ( pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
				continue;

			pPlayer->TeamFortress_SetSpeed();
		}
	}
#endif 

	CheckRespawnWaves();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_RND_RUNNING( void )
{
	SetupOnRoundRunning();

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_active" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	if( !IsInWaitingForPlayers() )
	{
		PlayStartRoundVoice();
	}

	m_bChangeLevelOnRoundEnd = false;
	m_bPrevRoundWasWaitingForPlayers = false;

	m_flNextBalanceTeamsTime = gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CheckReadyRestart( void )
{
	// check round restart
	if ( m_flRestartRoundTime > 0 && m_flRestartRoundTime <= gpGlobals->curtime && !g_pServerBenchmark->IsBenchmarkRunning() )
	{
		m_flRestartRoundTime.Set( -1.0f );
		m_flRestartRoundStartTime.Set( -1.0f );

#ifdef TF_DLL
		if ( TFGameRules() )
		{
			if ( TFGameRules()->IsMannVsMachineMode() )
			{
				if ( g_pPopulationManager && TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() )
				{
					g_pPopulationManager->StartCurrentWave();
					SetAllowBetweenRounds( true );
					return;
				}
			}
			else if ( TFGameRules()->IsCompetitiveMode() || TFGameRules()->IsEmulatingMatch() )
			{
				// otherwise, we handled this during m_flCompModeRespawnPlayersAtMatchStart
				if ( !TFGameRules()->IsPreRoundPushEnabled() )
				{
					TFGameRules()->StartCompetitiveMatch();
				}
				return;
			}
			else if ( mp_tournament.GetBool() )
			{
				if ( mp_tournament_prevent_team_switch_on_readyup.GetBool() )
				{
					TFGameRules()->SetSwitchTeams( false );
				}
				TFGameRules()->StartCompetitiveMatch();
				return;
			}
		}
#endif // TF_DLL

		// time to restart!
		State_Transition( GR_STATE_RESTART );
	}

	bool bProcessReadyRestart = IsWaitingForTeams();

#ifdef TF_DLL
	bProcessReadyRestart &= TFGameRules() && !TFGameRules()->UsePlayerReadyStatusMode();
#endif // TF_DLL

	// check ready restart
	if ( bProcessReadyRestart )
	{
		bool bTeamNotReady = false;
		for ( int i = LAST_SHARED_TEAM + 1; i < GetNumberOfTeams(); i++ )
		{
			if ( !IsTeamReady( i ) )
			{
				bTeamNotReady = true;
				break;
			}
		}

		if ( !bTeamNotReady )
		{
			mp_restartgame.SetValue( 5 );
			m_bAwaitingReadyRestart = false;
			SetAllowBetweenRounds( false );

			ShouldResetScores( true, true );
			ShouldResetRoundsPlayed( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_RND_RUNNING( void )
{
	//if we don't find any active players, return to GR_STATE_PREGAME
	if( !BHavePlayers() )
	{
#if defined( REPLAY_ENABLED )
		if ( g_pReplay )
		{
			// Write replay and stop recording if appropriate
			g_pReplay->SV_EndRecordingSession();
		}
#endif

		State_Transition( GR_STATE_PREGAME );
		return;
	}

	if ( m_flNextBalanceTeamsTime < gpGlobals->curtime )
	{
		BalanceTeams( true );
		m_flNextBalanceTeamsTime = gpGlobals->curtime + 1.0f;
	}

	CheckRespawnWaves();

	// check round restart
	CheckReadyRestart();

#ifdef TF_DLL
	if ( !TFGameRules()->IsMannVsMachineMode() && IsInPreMatch() && !TFGameRules()->IsCommunityGameMode() )
	{
		if ( TFGameRules()->UsePlayerReadyStatusMode() )
		{
			// if we just entered player ready status mode but we're stuck in round running, then let's fix that.
			if ( m_bAllowBetweenRounds )
			{
				m_bAwaitingReadyRestart = true;
				State_Transition( GR_STATE_PREROUND );
			}
		}
		// if we entered tournament mode but we're stuck without waiting for teams, then let's fix that.
		else if ( GetRoundRestartTime() <= 0 && mp_restartround.GetInt() <= 0 && mp_restartgame.GetInt() <= 0 && !mp_restartgame_immediate.GetBool() && !m_bAwaitingReadyRestart )
		{
			m_bAwaitingReadyRestart = true;
		}
	}
#endif

	// See if we're coming up to the server timelimit, in which case force a stalemate immediately.
	if ( mp_timelimit.GetInt() > 0 && IsInPreMatch() == false && GetTimeLeft() <= 0 )
	{
		if ( mp_timelimit_added_winlimit.GetInt() > 0 )
		{
			int nHighestScore = 0;
			for ( int i = LAST_SHARED_TEAM + 1; i < GetNumberOfTeams(); i++ )
			{
				CTeam *pTeam = GetGlobalTeam( i );
				if ( pTeam && pTeam->GetScore() > nHighestScore )
				{
					nHighestScore = pTeam->GetScore();
				}
			}

			int nAddedWinLimit = mp_timelimit_added_winlimit.GetInt();
			mp_winlimit.SetValue( nHighestScore + nAddedWinLimit );

			mp_timelimit.SetValue( 0 ); // Disable the time limit so it doesn't trigger again

			char strMessage[128];
			Q_snprintf( strMessage, sizeof( strMessage ), "Time limit reached! First team to %d wins the match!", mp_winlimit.GetInt() );

			UTIL_ClientPrintAll( HUD_PRINTCENTER, strMessage );
			UTIL_ClientPrintAll( HUD_PRINTTALK, strMessage );
		}
		else
		{
			const int iUserSetting = mp_match_end_at_timelimit.GetInt();
			if ( ( m_bAllowStalemateAtTimelimit || iUserSetting == 1 ) && iUserSetting != -1 )
			{
				int iDrawScoreCheck = -1;
				int iWinningTeam = 0;
				bool bTeamsAreDrawn = true;
				for ( int i = FIRST_GAME_TEAM; (i < GetNumberOfTeams()) && bTeamsAreDrawn; i++ )
				{
					int iTeamScore = GetGlobalTeam(i)->GetScore();

					if ( iTeamScore > iDrawScoreCheck )
					{
						iWinningTeam = i;
					}

					if ( iTeamScore != iDrawScoreCheck )
					{
						if ( iDrawScoreCheck == -1 )
						{
							iDrawScoreCheck = iTeamScore;
						}
						else
						{
							bTeamsAreDrawn = false;
						}
					}
				}

				if ( bTeamsAreDrawn )
				{
					if ( CanGoToStalemate() )
					{
						m_bChangelevelAfterStalemate = true;
						SetStalemate( STALEMATE_SERVER_TIMELIMIT, m_bForceMapReset );
					}
					else
					{
						SetOvertime( true );
					}
				}
				else
				{
					SetWinningTeam( iWinningTeam, WINREASON_TIMELIMIT, true, false, true );
				}
			}
		}
	}

	StopWatchModeThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_TEAM_WIN( void )
{
	// if we're forcing the map to reset it must be the end of a "full" round not a mini-round
	if ( m_bForceMapReset )
	{
		m_nRoundsPlayed++;
	}

	bool bGameOver = IsGameOver();

#ifdef TF_DLL
	if ( bGameOver && TFGameRules() && ( TFGameRules()->IsCompetitiveMode() || TFGameRules()->IsEmulatingMatch() ) && TFGameRules()->IsCommunityGameMode() )
	{
		extern ConVar tf_gamemode_community;
		extern ConVar tf_gamemode_misc;
		extern ConVar mp_tournament_readymode;
		// reset these so other things play nice
		mp_bonusroundtime.SetValue( mp_bonusroundtime.GetDefault() );
		tf_gamemode_community.SetValue( 0 );
		tf_gamemode_misc.SetValue( 0 );
		mp_tournament.SetValue( true );
		mp_tournament_readymode.SetValue( true );
		SetAllowBetweenRounds( true );
	}

	ETFMatchGroup eMatchGroup = TFGameRules()->GetCurrentMatchGroupWithEmulation();
	if ( GetMatchGroupDescription( eMatchGroup ) && GetMatchGroupDescription( eMatchGroup )->BUsesMultiSeries() && !TFGameRules()->IsCommunityGameMode() )
	{
		Msg( "[MULTI-SERIES DEBUG] Team Won\n" );

		// if we're at the end of a stopwatch round.
		if ( TFGameRules()->IsInStopWatch() && m_bForceMapReset && TFGameRules()->GetStopWatchTimer() )
		{
			// only reward the defending team for winning, since we already score who is the best attacker through stopwatch.
			// if an attacking team wins, then the only way for the other team to get a stopwatch point is to also win, but faster.
			// therefore, in that case, the differential in scoring is the stopwatch point.
			// if the attacking teams loses, then the other team can beat their # of points, and could potentially win.
			// in either case, winning just effectively adds a 2-0 differential, without really evaluating performance.
			// however, rewarding a successful defense does distinguish a team for being able to defend.
			// both teams being able to defend successfully marks both teams having some sort of skill preventing the other team from winning
			// even if one team does win the stopwatch point.
			if ( m_iWinningTeam != TEAM_UNASSIGNED )
			{
				CTFTeam* pTeam = GetGlobalTFTeam( m_iWinningTeam );
				if ( pTeam->GetRole() == TEAM_ROLE_DEFENDERS )
				{
					TFGameRules()->AddSeriesPoint( TFGameRules()->GetGCTeamForGameTeam( GetWinningTeam() ) );
					Msg( "[MULTI-SERIES DEBUG] Added series point to %d for winning (%d)\n", TFGameRules()->GetGCTeamForGameTeam( GetWinningTeam() ), m_iWinReason );
				}
			}

			// did we do the stopwatch back and forth?
			if ( !TFGameRules()->GetStopWatchTimer()->IsWatchingTimeStamps() )
			{
				Msg( "[MULTI-SERIES DEBUG] Detected Stopwatch end.\n" );
				int      iStopWatchWinner;
				CTFTeam* pAttacker = NULL;
				CTFTeam* pDefender = NULL;

				for ( int i = LAST_SHARED_TEAM + 1; i < GetNumberOfTeams(); i++ )
				{
					CTFTeam* pTeam = GetGlobalTFTeam( i );

					if ( pTeam )
					{
						if ( pTeam->GetRole() == TEAM_ROLE_DEFENDERS )
						{
							pDefender = pTeam;
						}

						if ( pTeam->GetRole() == TEAM_ROLE_ATTACKERS )
						{
							pAttacker = pTeam;
						}
					}
				}
				CTeamRoundTimer* pTimer = TFGameRules()->GetStopWatchTimer();
				if ( pTimer && pAttacker && pDefender )
				{
					Msg( "[MULTI-SERIES DEBUG] Calculating Stopwatch Score.\n" );
					if ( pAttacker->GetScore() > pDefender->GetScore() )
					{
						// getting more points is an absolute decider.
						iStopWatchWinner = pAttacker->GetTeamNumber();
						Msg( "[MULTI-SERIES DEBUG] %d captured more points (%d > %d).\n", iStopWatchWinner, pAttacker->GetScore(), pDefender->GetScore() );
					}
					else if ( pDefender->GetScore() > pAttacker->GetScore() )
					{
						iStopWatchWinner = pDefender->GetTeamNumber();
						Msg( "[MULTI-SERIES DEBUG] %d captured more points (%d > %d).\n", iStopWatchWinner, pDefender->GetScore(), pAttacker->GetScore() );
					}
					else
					{
						// teams are even.
						if ( pTimer->GetTimeRemaining() > 0.0f )
						{
							// attackers still have some time left, so they beat the time.
							iStopWatchWinner = pAttacker->GetTeamNumber();
							Msg( "[MULTI-SERIES DEBUG] %d beat the clock (%f left).\n", iStopWatchWinner, pTimer->GetTimeRemaining() );
						}
						else
						{
							iStopWatchWinner = pDefender->GetTeamNumber();
							Msg( "[MULTI-SERIES DEBUG] %d set the clock (%f total).\n", iStopWatchWinner, m_flStopWatchTotalTime );
						}
					}
					TFGameRules()->AddSeriesPoint( TFGameRules()->GetGCTeamForGameTeam( iStopWatchWinner ) );
				}
			}
		}
	}
#endif

	m_flStateTransitionTime = gpGlobals->curtime + GetBonusRoundTime( bGameOver );
	InternalHandleTeamWin( m_iWinningTeam );
	SendWinPanelInfo( bGameOver );

#ifdef TF_DLL
	// TODO(mcoms): mini matches?
	if ( TFGameRules() && ( TFGameRules()->IsCompetitiveMode() || TFGameRules()->IsEmulatingMatch() ) && bGameOver && !TFGameRules()->IsPlayingMultiSeriesIntermission() )
	{
		TFGameRules()->StopCompetitiveMatch( CMsgGC_Match_Result_Status_MATCH_SUCCEEDED );
	}
#endif // TF_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_TEAM_WIN( void )
{
	if ( gpGlobals->curtime > m_flStateTransitionTime )
	{
		bool bWinLimitReached = CheckWinLimit();
		bool bMaxRoundsReached = CheckMaxRounds();
		bool bTimeLimitReached = CheckTimeLimit();
		bool bNextLevelReached = CheckNextLevelCvar();

		bool bDone = ( bTimeLimitReached || bWinLimitReached || bMaxRoundsReached || bNextLevelReached );

#ifdef TF_DLL
		if ( TFGameRules() )
		{
			ETFMatchGroup eMatchGroup = TFGameRules()->GetCurrentMatchGroupWithEmulation();
			if ( GetMatchGroupDescription( eMatchGroup ) && GetMatchGroupDescription( eMatchGroup )->BUsesMultiSeries() && !TFGameRules()->IsCommunityGameMode() )
			{
				// In a multi-series match we override the traditional WinLimit/MaxRounds behavior.
				// A round win will trigger this state. If the series is complete (Win/MaxRounds limit hit),
				// we add a series point, tell the clients we're setting up the next series, and re-evaluate bDone.
				bool bSeriesComplete = ( bWinLimitReached || bMaxRoundsReached );

				if ( TFGameRules()->MatchmakingShouldUseStopwatchMode() && !TFGameRules()->GetStopWatchTimer() )
				{
					// if we're in a win state and we are no longer tracking the stopwatch, that means we did a full stopwatch back and forth
					bSeriesComplete = true;
				}

				if ( bSeriesComplete )
				{
					// if we are in stopwatch, we already gave a point elsewhere.
					if ( GetWinningTeam() != TEAM_UNASSIGNED && !TFGameRules()->MatchmakingShouldUseStopwatchMode() )
					{
						TFGameRules()->AddSeriesPoint( TFGameRules()->GetGCTeamForGameTeam( GetWinningTeam() ) );
						Msg("[MULTI-SERIES DEBUG] Added series point to %d for winning (%d)\n", TFGameRules()->GetGCTeamForGameTeam( GetWinningTeam() ), m_iWinReason );
					}
					// Are we done with the entire match?
					// The match is done if the time limit is reached, AND the series points are not tied.
					bool bIsTied = false;
					int nRedPoints = TFGameRules()->GetSeriesPoints( TF_GC_TEAM_DEFENDERS );
					int nBluePoints = TFGameRules()->GetSeriesPoints( TF_GC_TEAM_INVADERS );
					if ( nRedPoints == nBluePoints )
					{
						bIsTied = true;
					}
					if ( !bTimeLimitReached || bIsTied )
					{
						SetForceMapReset( true ); // Ensure the map resets for the next series
						
						// Setup intermission
						TFGameRules()->SetMultiSeriesIntermission( true );

						if ( mp_shuffleteams_auto.GetBool() )
						{
							int nSeriesDelta = abs( nRedPoints - nBluePoints );
							if ( nSeriesDelta >= mp_shuffleteams_auto_seriesdifference.GetInt() )
							{
								// Evaluate team disparity
								double nTeamScoreRed = 0.0;
								double nTeamScoreBlue = 0.0;
								
								for ( int i = 1; i <= MAX_PLAYERS; i++ )
								{
									CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
									if ( pPlayer )
									{
										if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
											nTeamScoreRed += TFAutoBalance()->GetPlayerAutoBalanceScore( pPlayer );
										else if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
											nTeamScoreBlue += TFAutoBalance()->GetPlayerAutoBalanceScore( pPlayer );
									}
								}

								if ( nTeamScoreRed > 0.0 && nTeamScoreBlue > 0.0 )
								{
									double flDisparity = 1.0;
									if ( nTeamScoreRed > nTeamScoreBlue )
									{
										flDisparity = nTeamScoreRed / nTeamScoreBlue;
									}
									else
									{
										flDisparity = nTeamScoreBlue / nTeamScoreRed;
									}

									if ( flDisparity >= mp_shuffleteams_auto_score_disparity.GetFloat() )
									{
										SetShuffleTeams( true );
									}
								}
							}
						}
					}
				}
				else
				{
					// Series is not complete yet (e.g. they only won 1 out of 2 rounds in a series).
					bDone = false;
				}
			}
		}
#endif // TF_DLL

		// check the win limit, max rounds, time limit and nextlevel cvar before starting the next round
		if ( !bDone )
		{
			PreviousRoundEnd();

			if ( ShouldGoToBonusRound() )
			{
				State_Transition( GR_STATE_BONUS );
			}
			else
			{
#if defined( REPLAY_ENABLED )
				if ( g_pReplay )
				{
					// Write replay and stop recording if appropriate
					g_pReplay->SV_EndRecordingSession();
				}
#endif

				State_Transition( GR_STATE_PREROUND );
			}
		}
		else if ( IsInTournamentMode() )
		{
			bool bShowScorboard = true;
#ifdef TF_DLL
			if ( TFGameRules() && ( TFGameRules()->IsCompetitiveMode() || TFGameRules()->IsEmulatingMatch() ) )
			{
				bShowScorboard = false;
			}
#endif // TF_DLL

			for ( int i = 1; i <= MAX_PLAYERS; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

				if ( !pPlayer )
					continue;

				if ( bShowScorboard )
				{
					pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
				}
			}

			RestartTournament();

			if ( IsInArenaMode() )
			{
#if defined( REPLAY_ENABLED )
				if ( g_pReplay )
				{
					// Write replay and stop recording if appropriate
					g_pReplay->SV_EndRecordingSession();
				}
#endif

				State_Transition( GR_STATE_PREROUND );
			}
#ifdef TF_DLL
			else if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && g_pPopulationManager )
			{
				// one of the convars mp_timelimit, mp_winlimit, mp_maxrounds, or nextlevel has been triggered
				for ( int i = 1; i <= MAX_PLAYERS; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
					if ( !pPlayer )
						continue;

					pPlayer->AddFlag( FL_FROZEN );
				}

				g_fGameOver = true;
				g_pPopulationManager->SetMapRestartTime( gpGlobals->curtime + 10.0f );
				State_Enter( GR_STATE_GAME_OVER );
				return;
			}
			else if ( TFGameRules() && TFGameRules()->UsePlayerReadyStatusMode() )
			{
				for ( int i = 1; i <= MAX_PLAYERS; i++ )
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
					if ( !pPlayer )
						continue;

					pPlayer->AddFlag( FL_FROZEN );
				}

				g_fGameOver = true;
				State_Enter( GR_STATE_GAME_OVER );
				
				const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );

				float flPostMatchPeriod = ( pMatchDesc || TFGameRules()->IsEmulatingMatch() ) ? GetPostMatchPeriod() : 10.0f;
				if ( TFGameRules()->IsPlayingMultiSeriesIntermission() )
				{
					flPostMatchPeriod = 8.0f;
				}

				bool bWillLeaveMap = false;
				static ConVarRef tf_match_emulation_restartmatch( "tf_match_emulation_restartmatch" );
				if ( pMatchDesc || TFGameRules()->IsEmulatingMatch() && !tf_match_emulation_restartmatch.GetBool() )
				{
					bWillLeaveMap = true;
				}

				// if we're running HLTV, then make sure we don't end the match before it's caught up.
				static ConVarRef tv_delaymapchange( "tv_delaymapchange" );
				if ( bWillLeaveMap && HLTVDirector() && HLTVDirector()->IsActive() && tv_delaymapchange.GetBool() )
				{
					flPostMatchPeriod = Max( flPostMatchPeriod, HLTVDirector()->GetDelay() );
				}

				m_flStateTransitionTime = gpGlobals->curtime + flPostMatchPeriod;

				if ( TFGameRules() && ( TFGameRules()->IsCompetitiveMode() || TFGameRules()->IsEmulatingMatch() ) )
				{
					TFGameRules()->MatchSummaryStart();
				}
				return;
			}
#endif // TF_DLL
			else
			{
#ifdef TF_DLL
				g_fGameOver = false;
				// we're ready for ready mode if we switch to it again.
				SetAllowBetweenRounds( true );
				// trick restart into doing a full map cleanup.
				SetInWaitingForPlayers( true );
				State_Transition( GR_STATE_RESTART );
				// restore our waiting for players state.
				SetInWaitingForPlayers( true );
#else
				State_Transition( GR_STATE_RND_RUNNING );
#endif
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_STALEMATE( void )
{
	m_flStalemateStartTime = gpGlobals->curtime;
	SetupOnStalemateStart();

	// Stop any timers, and bring up a new one
	HideActiveTimer();

	if ( m_hStalemateTimer )
	{
		UTIL_Remove( m_hStalemateTimer );
		m_hStalemateTimer = NULL;
	}

	int iTimeLimit = mp_stalemate_timelimit.GetInt();

	if ( IsInArenaMode() == true )
	{
		iTimeLimit = tf_arena_round_time.GetInt();
	}

	if ( iTimeLimit > 0 )
	{
#ifndef CSTRIKE_DLL
		variant_t sVariant;
		if ( !m_hStalemateTimer )
		{
			m_hStalemateTimer = (CTeamRoundTimer*)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
		}
		m_hStalemateTimer->KeyValue( "show_in_hud", "1" );
		sVariant.SetInt( iTimeLimit );
	
		m_hStalemateTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
		m_hStalemateTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
		m_hStalemateTimer->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
#endif
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Leave_STALEMATE( void )
{
	SetupOnStalemateEnd();

	if ( m_hStalemateTimer )
	{
		UTIL_Remove( m_hStalemateTimer );
	}

	if ( IsInArenaMode() == false )
	{
		RestoreActiveTimer();

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_BONUS( void )
{
	SetupOnBonusStart();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Leave_BONUS( void )
{
	SetupOnBonusEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_BONUS( void )
{
	BonusStateThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_BETWEEN_RNDS( void )
{
	BetweenRounds_Start();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Leave_BETWEEN_RNDS( void )
{
	BetweenRounds_End();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_BETWEEN_RNDS( void )
{
	BetweenRounds_Think();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::HideActiveTimer( void )
{
	// We can't handle this, because we won't be able to restore multiple timers
	Assert( m_hPreviousActiveTimer.Get() == NULL );

	m_hPreviousActiveTimer = NULL;

#ifndef CSTRIKE_DLL
	CBaseEntity *pEntity = NULL;
	variant_t sVariant;
	sVariant.SetInt( false );

	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "team_round_timer" )) != NULL)
	{
		CTeamRoundTimer *pTimer = assert_cast<CTeamRoundTimer*>(pEntity);
		if ( pTimer && pTimer->ShowInHud() )
		{
			Assert( !m_hPreviousActiveTimer );
			m_hPreviousActiveTimer = pTimer;
			pEntity->AcceptInput( "ShowInHUD", NULL, NULL, sVariant, 0 );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::RestoreActiveTimer( void )
{
	if ( m_hPreviousActiveTimer )
	{
		variant_t sVariant;
		sVariant.SetInt( true );
		m_hPreviousActiveTimer->AcceptInput( "ShowInHUD", NULL, NULL, sVariant, 0 );
		m_hPreviousActiveTimer = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_STALEMATE( void )
{
	//if we don't find any active players, return to GR_STATE_PREGAME
	if( !BHavePlayers() && IsInArenaMode() == false )
	{
#if defined( REPLAY_ENABLED )
		if ( g_pReplay )
		{
			// Write replay and stop recording if appropriate
			g_pReplay->SV_EndRecordingSession();
		}
#endif

		State_Transition( GR_STATE_PREGAME );
		return;
	}

	if ( IsInTournamentMode() == true && IsInWaitingForPlayers() == true )
	{
		CheckReadyRestart();
		CheckRespawnWaves();
		return;
	}

	int iDeadTeam = TEAM_UNASSIGNED;
	int iAliveTeam = TEAM_UNASSIGNED;

	// If a team is fully killed, the other team has won
	for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
	{
		CTeam *pTeam = GetGlobalTeam(i);
		Assert( pTeam );

		int iPlayers = pTeam->GetNumPlayers();
		if ( iPlayers )
		{
			bool bFoundLiveOne = false;
			for ( int player = 0; player < iPlayers; player++ )
			{
				if ( pTeam->GetPlayer(player) && pTeam->GetPlayer(player)->IsAlive() )
				{
					bFoundLiveOne = true;
					break;
				}
			}

			if ( bFoundLiveOne )
			{
				iAliveTeam = i;
			}
			else
			{
				iDeadTeam = i;
			}
		}
		else
		{
			iDeadTeam = i;
		}
	}

	if ( iDeadTeam && iAliveTeam )
	{
		// The live team has won. 
		bool bMasterHandled = false;
		if ( !m_bForceMapReset )
		{
			// We're not resetting the map, so give the winners control
			// of all the points that were in play this round.
			// Find the control point master.
			CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
			if ( pMaster )
			{
				variant_t sVariant;
				sVariant.SetInt( iAliveTeam );
				pMaster->AcceptInput( "SetWinnerAndForceCaps", NULL, NULL, sVariant, 0 );
				bMasterHandled = true;
			}
		}

		if ( !bMasterHandled )
		{
			SetWinningTeam( iAliveTeam, WINREASON_OPPONENTS_DEAD, m_bForceMapReset );
		}
	}
	else if ( ( iDeadTeam && iAliveTeam == TEAM_UNASSIGNED ) || 
			  ( m_hStalemateTimer && TimerMayExpire() && m_hStalemateTimer->GetTimeRemaining() <= 0 ) )
	{
		bool bFullReset = true;

		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

		if ( pMaster && pMaster->PlayingMiniRounds() )
		{
			// we don't need to do a full map reset for maps with mini-rounds
			bFullReset = false;
		}

		// Both teams are dead. Pure stalemate.
		SetWinningTeam( TEAM_UNASSIGNED, WINREASON_STALEMATE, bFullReset, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: manual restart
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Enter_RESTART( void )
{
	// send scores
	SendTeamScoresEvent();

	// send restart event
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_restart_round" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	m_bPrevRoundWasWaitingForPlayers = m_bInWaitingForPlayers;
	SetInWaitingForPlayers( false );

	ResetScores();

#ifdef TF_DLL
	if ( !TFGameRules() || !TFGameRules()->IsMatchPlayingOut() )
	{
		// reset the round time
		ResetMapTime();
	}
#endif

	State_Transition( GR_STATE_PREROUND );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::State_Think_RESTART( void )
{
	// should never get here, State_Enter_RESTART sets us into a different state
	Assert( 0 ); 
}

//-----------------------------------------------------------------------------
// Purpose: Sorts teams by score
//-----------------------------------------------------------------------------
int TeamScoreSort( CTeam* const *pTeam1, CTeam* const *pTeam2 )
{
	if ( !*pTeam1 )
		return -1;

	if ( !*pTeam2 )
		return -1;

	if ( (*pTeam1)->GetScore() > (*pTeam2)->GetScore() )
	{
		return 1;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Input for other entities to declare a round winner.
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetWinningTeam( int team, int iWinReason, bool bForceMapReset /* = true */, bool bSwitchTeams /* = false*/, bool bDontAddScore /* = false*/, bool bFinal /*= false*/ )
{
	// Commentary doesn't let anyone win
	if ( IsInCommentaryMode() )
		return;

	if ( ( team != TEAM_UNASSIGNED ) && ( team <= LAST_SHARED_TEAM || team >= GetNumberOfTeams() ) )
	{
		Assert( !"SetWinningTeam() called with invalid team." );
		return;
	}

	// are we already in this state?
	if ( State_Get() == GR_STATE_TEAM_WIN )
		return;

	SetForceMapReset( bForceMapReset );
	SetSwitchTeams( bSwitchTeams );

	m_iWinningTeam = team;
	m_iWinReason = iWinReason;

	// only reward the team if they have won the map and we're going to do a full reset or the time has run out and we're changing maps
	bool bRewardTeam = bForceMapReset || ( IsGameUnderTimeLimit() && ( GetTimeLeft() <= 0 ) );

	if ( bDontAddScore == true )
	{
		bRewardTeam = false;
	}

	m_bUseAddScoreAnim = false;
	if ( bRewardTeam && ( team != TEAM_UNASSIGNED ) && ShouldScorePerRound() )
	{
		GetGlobalTeam( team )->AddScore( TEAMPLAY_ROUND_WIN_SCORE );
		m_bUseAddScoreAnim = true;
	}

	// this was a sudden death win if we were in stalemate then a team won it
	bool bWasSuddenDeath = ( InStalemate() && m_iWinningTeam >= FIRST_GAME_TEAM );

	State_Transition( GR_STATE_TEAM_WIN );

	// this needs to be AFTER we add score above (for TF)
	PlayWinSong( team );

	m_flLastTeamWin = gpGlobals->curtime;

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_win" );
	if ( event )
	{
		event->SetInt( "team", team );
		event->SetInt( "winreason", iWinReason );
		event->SetBool( "full_round", bForceMapReset );
		event->SetFloat( "round_time", gpGlobals->curtime - m_flRoundStartTime );
		event->SetBool( "was_sudden_death", bWasSuddenDeath );
		// let derived classes add more fields to the event
		FillOutTeamplayRoundWinEvent( event );
		gameeventmanager->FireEvent( event );
	}

	// send team scores
	SendTeamScoresEvent();

	if ( team == TEAM_UNASSIGNED )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer )
				continue;

			pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_STALEMATE );
		}
	}

	// Auto scramble teams?
	if ( bForceMapReset && mp_scrambleteams_auto.GetBool() )
	{
		if ( IsInArenaMode() || IsInTournamentMode() || ShouldSkipAutoScramble() )
			return;

#ifndef DEBUG
		// Don't bother on a listen server - usually not desirable
		if ( !engine->IsDedicatedServer() )
			return;
#endif // DEBUG

		// Skip if we have a nextlevel set
		if ( !FStrEq( nextlevel.GetString(), "" ) )
			return;

		// Track the team scores
		if ( m_iWinningTeam != TEAM_UNASSIGNED )
		{
			// m_GameTeams differs from g_Teams by storing only "Real" teams
			if ( m_GameTeams.Count() == 0 )
			{
				int iTeamIndex = FIRST_GAME_TEAM;
				CTeam *pTeam;
				for ( pTeam = GetGlobalTeam(iTeamIndex); pTeam != NULL; pTeam = GetGlobalTeam(++iTeamIndex) )
				{
					m_GameTeams.Insert( iTeamIndex, 0 );
				}
			}

			// Safety net hack - we assume there are only two "Real" teams
			// driller:  need to make this work in all cases
			if ( m_GameTeams.Count() != 2 )
				return;
		}

		// Look for impending level change
		if ( ( ( mp_timelimit.GetInt() > 0 && CanChangelevelBecauseOfTimeLimit() ) || m_bChangelevelAfterStalemate ) && GetTimeLeft() <= 300 )
			return;

		if ( mp_winlimit.GetInt() || mp_maxrounds.GetInt() )
		{
			int nRoundsPlayed = GetRoundsPlayed();
			if ( ( mp_maxrounds.GetInt() - nRoundsPlayed ) == 1 )
			{
				return;
			}

			int nWinLimit = mp_winlimit.GetInt();
			for ( int iIndex = m_GameTeams.FirstInorder(); iIndex != m_GameTeams.InvalidIndex(); iIndex = m_GameTeams.NextInorder( iIndex ) )
			{
				int nTeamScore = GetGlobalTeam( m_GameTeams.Key( iIndex ) )->GetScore();
				if ( nWinLimit - nTeamScore == 1 )
				{
					return;
				}
			}
		}

		// Increment win counters
		int iWinningTeamIndex = m_GameTeams.Find( m_iWinningTeam );
		if ( iWinningTeamIndex != m_GameTeams.InvalidIndex() )
		{
			m_GameTeams[iWinningTeamIndex]++;
		}
		else
		{
			Assert( iWinningTeamIndex == m_GameTeams.InvalidIndex() );
			return;
		}

		// Did we hit our win delta?
		int nWinDelta = abs( m_GameTeams[1] - m_GameTeams[0] );
		if ( nWinDelta >= mp_scrambleteams_auto_windifference.GetInt() )
		{
			// Let the server know we're going to scramble on round restart
#ifdef TF_DLL
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_alert" );
			if ( event )
			{
				event->SetInt( "alert_type", HUD_ALERT_SCRAMBLE_TEAMS );
				gameeventmanager->FireEvent( event );
			}
#else
			const char *pszMessage = "#game_scramble_onrestart";
			if ( pszMessage )
			{
				UTIL_ClientPrintAll( HUD_PRINTCENTER, pszMessage );
				UTIL_ClientPrintAll( HUD_PRINTCONSOLE, pszMessage );
			}
#endif
			UTIL_LogPrintf( "World triggered \"ScrambleTeams_Auto\"\n" );

			SetScrambleTeams( true );
			ShouldResetScores( true, false );
			ShouldResetRoundsPlayed( false );
		}

		// If we switch teams after this win, swap scores
		if ( ShouldSwitchTeams() )
		{
			int nTempScore = m_GameTeams[0];
			m_GameTeams[0] = m_GameTeams[1];
			m_GameTeams[1] = nTempScore;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input for other entities to declare a stalemate
//			Most often a team_control_point_master saying that the
//			round timer expired 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetStalemate( int iReason, bool bForceMapReset /* = true */, bool bSwitchTeams /* = false */ )
{
	if ( IsInTournamentMode() == true && IsInPreMatch() == true )
		return;

	if ( !mp_stalemate_enable.GetBool() )
	{
		SetWinningTeam( TEAM_UNASSIGNED, WINREASON_STALEMATE, bForceMapReset, bSwitchTeams );
		return;
	}

	if ( InStalemate() )
		return;

	SetForceMapReset( bForceMapReset );

	m_iWinningTeam = TEAM_UNASSIGNED;

	PlaySuddenDeathSong();

	State_Transition( GR_STATE_STALEMATE );

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_stalemate" );
	if ( event )
	{
		event->SetInt( "reason", iReason );
		gameeventmanager->FireEvent( event );
	}
}

#ifdef GAME_DLL
void CC_CH_ForceRespawn( void )
{
	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	if ( pRules )
	{
		pRules->RespawnPlayers( true );
	}
}
static ConCommand mp_forcerespawnplayers("mp_forcerespawnplayers", CC_CH_ForceRespawn, "Force all players to respawn.", FCVAR_CHEAT );

static ConVar mp_tournament_allow_non_admin_restart( "mp_tournament_allow_non_admin_restart", "0", FCVAR_NONE, "Allow mp_tournament_restart command to be issued by players other than admin.");
void CC_CH_TournamentRestart( void )
{
	if ( mp_tournament_allow_non_admin_restart.GetBool() == false )
	{
		if ( !UTIL_IsCommandIssuedByServerAdmin() )
			return;
	}

#ifdef TF_DLL
	if ( TFGameRules() && ( TFGameRules()->IsMannVsMachineMode() ) )
		return;
#endif // TF_DLL

	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	if ( pRules )
	{
		pRules->FullRestartTournament();
	}
}
static ConCommand mp_tournament_restart("mp_tournament_restart", CC_CH_TournamentRestart, "Restart Tournament Mode on the current level."  );

void CC_CH_TournamentRefresh( void )
{
	if ( mp_tournament_allow_non_admin_restart.GetBool() == false )
	{
		if ( !UTIL_IsCommandIssuedByServerAdmin() )
			return;
	}

#ifdef TF_DLL
	if ( TFGameRules() && ( TFGameRules()->IsMannVsMachineMode() ) )
		return;
#endif // TF_DLL

	CTeamplayRoundBasedRules* pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	if ( pRules )
	{
		pRules->RestartTournament();
	}
}
static ConCommand mp_tournament_legacy_refresh( "mp_tournament_stop_and_refresh", CC_CH_TournamentRefresh, "Refresh Tournament Mode on the current level, keeping the round largely intact. Provided for legacy mp_tournament_restart behavior." );

void CTeamplayRoundBasedRules::RestartTournament( void )
{
	if ( IsInTournamentMode() == false )
		return;

	SetInWaitingForPlayers( true );
	m_bAwaitingReadyRestart = true;
	m_flStopWatchTotalTime = -1.0f;
	SetInStopWatch( false );

	// we might have had a stalemate during the last round
	// so reset this bool each time we restart the tournament
	m_bChangelevelAfterStalemate = false;

	ResetPlayerAndTeamReadyState();
}

void CTeamplayRoundBasedRules::FullRestartTournament( void )
{
	if ( IsInTournamentMode() == false )
		return;
	
	g_fGameOver = false;
	RestartTournament();
}

#endif


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bForceRespawn - respawn player even if dead or dying
//			bTeam - if true, only respawn the passed team
//			iTeam  - team to respawn
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::RespawnPlayers( bool bForceRespawn, bool bTeam /* = false */, int iTeam/* = TEAM_UNASSIGNED */ )
{
	if ( bTeam )
	{
		Assert( iTeam > LAST_SHARED_TEAM && iTeam < GetNumberOfTeams() );
	}	

	CBasePlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		// Check for team specific spawn
		if ( bTeam && pPlayer->GetTeamNumber() != iTeam )
			continue;

		// players that haven't chosen a team/class can never spawn
		if ( !pPlayer->IsReadyToPlay() )
		{
#ifdef TF_DLL
			CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
			// if the player has reset their class (or we're in a competitive match), we still need to wait the minimum time.
			const bool bNoSpawnBypass = TFGameRules()->IsCompetitiveGame() || pTFPlayer->HasResetClass();
			if ( bNoSpawnBypass && bTeam && !HasPassedMinRespawnTime( pPlayer ) )
				continue;
#endif

			// Let the player spawn immediately when they do pick a class
			if ( pPlayer->ShouldGainInstantSpawn() )
			{
				pPlayer->AllowInstantSpawn();
			}

			continue;
		}

		// If we aren't force respawning, don't respawn players that:
		// - are alive
		// - are still in the death anim stage of dying
		if ( !bForceRespawn )
		{
			if ( pPlayer->IsAlive() )
				continue; 

			if ( m_iRoundState != GR_STATE_PREROUND )
			{
				// If the player hasn't been dead the minimum respawn time, he
				// waits until the next wave.
				if ( bTeam && !HasPassedMinRespawnTime( pPlayer ) )
					continue;
				
				if ( !pPlayer->IsReadyToSpawn() )
				{
					// Let the player spawn immediately when they do pick a class
					if ( pPlayer->ShouldGainInstantSpawn() )
					{
						pPlayer->AllowInstantSpawn();
					}

					continue;
				}

			
			}
		}

		// Respawn this player
		pPlayer->ForceRespawn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::InitTeams( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::BHavePlayers( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer && pPlayer->IsReadyToPlay() )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::HandleTimeLimitChange( void )
{
	// check that we have an active timer in the HUD and use mp_timelimit if we don't
	if ( !MapHasActiveTimer() && ( mp_timelimit.GetInt() > 0 && GetTimeLeft() > 0  ) )
	{
		CreateTimeLimitTimer();
	}
	else
	{
		if ( m_hTimeLimitTimer )
		{
			UTIL_Remove( m_hTimeLimitTimer );
			m_hTimeLimitTimer = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::ResetPlayerAndTeamReadyState( void )
{
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		SetTeamReadyState( false, i );
	}

	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		m_bPlayerReady.Set( i, false );
	}

#ifdef GAME_DLL
	// Note <= MAX_PLAYERS vs < MAX_PLAYERS above
	for ( int i = 0; i <= MAX_PLAYERS; i++ )
	{
		m_bPlayerReadyBefore[i] = false;
	}
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::MapHasActiveTimer( void )
{
#ifndef CSTRIKE_DLL
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "team_round_timer" ) ) != NULL )
	{
		CTeamRoundTimer *pTimer = assert_cast<CTeamRoundTimer*>( pEntity );
		if ( pTimer && pTimer->ShowInHud() && ( Q_stricmp( STRING( pTimer->GetEntityName() ), "zz_teamplay_timelimit_timer" ) != 0 ) )
		{
			return true;
		}
	}
#endif
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CreateTimeLimitTimer( void )
{
	if ( IsInArenaMode () == true || IsInKothMode() == true )
		return;

	// this is the same check we use in State_Think_RND_RUNNING()
	// don't show the timelimit timer if we're not going to end the map when it runs out
	const int iUserSetting = mp_match_end_at_timelimit.GetInt();
	bool bAllowStalemate = ( m_bAllowStalemateAtTimelimit || iUserSetting == 1 ) && iUserSetting != -1;
	if ( !bAllowStalemate )
		return;

#ifndef CSTRIKE_DLL
	if ( !m_hTimeLimitTimer )
	{
		m_hTimeLimitTimer = (CTeamRoundTimer*)CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle );
		m_hTimeLimitTimer->SetName( MAKE_STRING( "zz_teamplay_timelimit_timer" ) );
	}

	variant_t sVariant;
	m_hTimeLimitTimer->KeyValue( "show_in_hud", "1" );
	sVariant.SetInt( GetTimeLeft() );
	m_hTimeLimitTimer->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );
	m_hTimeLimitTimer->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
	m_hTimeLimitTimer->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::RoundRespawn( void )
{
	if ( mp_showroundtransitions.GetInt() > 0 )
	{
		Msg( "Gamerules: round respawn\n");
	}

	m_flRoundStartTime = gpGlobals->curtime;

	if ( m_bForceMapReset || PrevRoundWasWaitingForPlayers() )
	{
		CleanUpMap();

		// clear out the previously played rounds
		m_iszPreviousRounds.RemoveAll();

		if ( mp_timelimit.GetInt() > 0 && GetTimeLeft() > 0  )
		{
			// check that we have an active timer in the HUD and use mp_timelimit if we don't
			if ( !MapHasActiveTimer() )
			{
				CreateTimeLimitTimer();
			}
		}

		m_iLastCapPointChanged = 0;
	}

	// reset our spawn times to the original values
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		if ( m_flOriginalTeamRespawnWaveTime[i] >= 0 )
		{
			m_TeamRespawnWaveTimes.Set( i, m_flOriginalTeamRespawnWaveTime[i] );
		}
	}

	if ( !IsInWaitingForPlayers() )
	{
		if ( m_bForceMapReset )
		{
			UTIL_LogPrintf( "World triggered \"Round_Start\"\n" );
		}
	}
	
	// Setup before respawning players, so we can mess with spawnpoints
	SetupOnRoundStart();

	// Do we need to switch the teams?
	m_bSwitchedTeamsThisRound = false;
	if ( ShouldSwitchTeams() )
	{
		m_bSwitchedTeamsThisRound = true;
		HandleSwitchTeams();
		SetSwitchTeams( false );
	}

	// Do we need to switch the teams?
	if ( ShouldScrambleTeams() )
	{
		HandleScrambleTeams();
		SetScrambleTeams( false );
	}

	if ( ShouldShuffleTeams() )
	{
		HandleTeamShuffle();
		SetShuffleTeams( false );
	}

#if defined( REPLAY_ENABLED )
	bool bShouldWaitToStartRecording = ShouldWaitToStartRecording();
	if ( g_pReplay && g_pReplay->SV_ShouldBeginRecording( bShouldWaitToStartRecording ) )
	{
		// Tell the replay manager that it should begin recording the new round as soon as possible
		g_pReplay->SV_GetContext()->GetSessionRecorder()->StartRecording();
	}
#endif

    // Free any edicts that were marked deleted. This should hopefully clear some out
    //  so the below function can use the now freed ones.
	engine->AllowImmediateEdictReuse();

	RespawnPlayers( true );

	// and then again, we free up edicts because player spawn can clean up edicts in some cases (upgrade mode)
	if ( m_bForceMapReset || PrevRoundWasWaitingForPlayers() )
	{
		engine->AllowImmediateEdictReuse();
	}

	// reset per-round scores for each player
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			pPlayer->ResetPerRoundStats();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Recreate all the map entities from the map data (preserving their indices),
//			then remove everything else except the players.
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CleanUpMap()
{
	if( mp_showcleanedupents.GetInt() )
	{
		Msg( "CleanUpMap\n===============\n" );
		Msg( "  Entities: %d (%d edicts)\n", gEntList.NumberOfEntities(), gEntList.NumberOfEdicts() );
	}

	// Get rid of all entities except players.
	CBaseEntity *pCur = gEntList.FirstEnt();
	while ( pCur )
	{
		if ( !RoundCleanupShouldIgnore( pCur ) )
		{
			if( mp_showcleanedupents.GetInt() & 1 )
			{
				Msg( "Removed Entity: %s\n", pCur->GetClassname() );
			}
			UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	// Clear out the event queue
	g_EventQueue.Clear();

	// Really remove the entities so we can have access to their slots below.
	gEntList.CleanupDeleteList();
	
	// Josh: Purge any template/script fixup name strings or whatever here!
	// That way we don't run out of memory running the same map forever.
	// We cannot free instantly due to string dependencies on events
	// after the entity's death.
	PurgeDeferredPooledStrings();

	engine->AllowImmediateEdictReuse();

	if ( mp_showcleanedupents.GetInt() & 2 )
	{
		Msg( "  Entities Left:\n" );
		pCur = gEntList.FirstEnt();
		while ( pCur )
		{
			Msg( "  %s (%d)\n", pCur->GetClassname(), pCur->entindex() );
			pCur = gEntList.NextEnt( pCur );
		}
	}

	// Now reload the map entities.
	class CTeamplayMapEntityFilter : public IMapEntityFilter
	{
	public:
		CTeamplayMapEntityFilter()
		{
			m_pRules = assert_cast<CTeamplayRoundBasedRules*>( GameRules() );
		}

		virtual bool ShouldCreateEntity( const char *pClassname )
		{
			// Don't recreate the preserved entities.
			if ( m_pRules->ShouldCreateEntity( pClassname ) )
				return true;

			// Increment our iterator since it's not going to call CreateNextEntity for this ent.
			if ( m_iIterator != g_MapEntityRefs.InvalidIndex() )
			{
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );
			}

			return false;
		}


		virtual CBaseEntity* CreateNextEntity( const char *pClassname )
		{
			if ( m_iIterator == g_MapEntityRefs.InvalidIndex() )
			{
				// This shouldn't be possible. When we loaded the map, it should have used 
				// CTeamplayMapEntityFilter, which should have built the g_MapEntityRefs list
				// with the same list of entities we're referring to here.
				Assert( false );
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );	// Seek to the next entity.

				if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
				{
					// Doh! The entity was delete and its slot was reused.
					// Just use any old edict slot. This case sucks because we lose the baseline.
					return CreateEntityByName( pClassname );
				}
				else
				{
					// Cool, the slot where this entity was is free again (most likely, the entity was 
					// freed above). Now create an entity with this specific index.
					return CreateEntityByName( pClassname, ref.m_iEdict );
				}
			}
		}

	public:
		int m_iIterator; // Iterator into g_MapEntityRefs.
		CTeamplayRoundBasedRules *m_pRules;
	};
	CTeamplayMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	// DO NOT CALL SPAWN ON info_node ENTITIES!

	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::ShouldCreateEntity( const char *pszClassName )
{
	return !FindInList( s_PreserveEnts, pszClassName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::RoundCleanupShouldIgnore( CBaseEntity *pEnt )
{
	return FindInList( s_PreserveEnts, pEnt->GetClassname() );
}

//-----------------------------------------------------------------------------
// Purpose: Sort function for sorting players by time spent connected ( user ID )
//-----------------------------------------------------------------------------
static int SwitchPlayersSort(  CBaseMultiplayerPlayer * const *p1, CBaseMultiplayerPlayer * const *p2 )
{
	// sort by score
	return ( (*p2)->GetTeamBalanceScore() - (*p1)->GetTeamBalanceScore() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::CheckRespawnWaves( void )
{
	for ( int team = LAST_SHARED_TEAM+1; team < GetNumberOfTeams(); team++ )
	{
		if ( GetNextRespawnWave(team) && GetNextRespawnWave(team) > gpGlobals->curtime )
			continue;

		RespawnTeam( team );

		// Set m_flNextRespawnWave to 0 when we don't have a respawn time to reduce networking
		float flNextRespawnLength = GetRespawnWaveMaxLength( team );
		if ( flNextRespawnLength )
		{
			m_flNextRespawnWave.Set( team, gpGlobals->curtime + flNextRespawnLength );
		}
		else
		{
			m_flNextRespawnWave.Set( team, 0.0f );
		}

	}
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::BalanceTeams( bool bRequireSwitcheesToBeDead )
{
	if ( mp_autoteambalance.GetBool() == false || ( IsInArenaMode() == true && tf_arena_use_queue.GetBool() == true ) )
	{
		return;
	}

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return;
#endif // _DEBUG || STAGING_ONLY

	if ( IsInTraining() || IsInItemTestingMode() )
	{
		return;
	}

	// we don't balance for a period of time at the start of the game
	if ( gpGlobals->curtime < m_flStartBalancingTeamsAt )
	{
		return;
	}

	// wrap with this bool, indicates it's a round running switch and not a between rounds insta-switch
	if ( bRequireSwitcheesToBeDead )
	{
#ifndef CSTRIKE_DLL
		// we don't balance if there is less than 60 seconds on the active timer
		CTeamRoundTimer *pActiveTimer = GetActiveRoundTimer();
		if ( pActiveTimer && pActiveTimer->GetTimeRemaining() < 60 )
		{
			return;
		}
#endif
	}

	int iHeaviestTeam = TEAM_UNASSIGNED, iLightestTeam = TEAM_UNASSIGNED;

	// Figure out if we're unbalanced
	if ( !AreTeamsUnbalanced( iHeaviestTeam, iLightestTeam ) )
	{
		m_flFoundUnbalancedTeamsTime = -1;
		m_bPrintedUnbalanceWarning = false;
		return;
	}

	if ( m_flFoundUnbalancedTeamsTime < 0 )
	{
		m_flFoundUnbalancedTeamsTime = gpGlobals->curtime;
	}

	// if teams have been unbalanced for X seconds, play a warning 
	if ( !m_bPrintedUnbalanceWarning && ( ( gpGlobals->curtime - m_flFoundUnbalancedTeamsTime ) > 1.0 ) )
	{
		// print unbalance warning
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_auto_team_balance_in", "5" );
		m_bPrintedUnbalanceWarning = true;
	}

	// teams are unblanced, figure out some players that need to be switched

	CTeam *pHeavyTeam = GetGlobalTeam( iHeaviestTeam );
	CTeam *pLightTeam = GetGlobalTeam( iLightestTeam );

	Assert( pHeavyTeam && pLightTeam );

	int iNumSwitchesRequired = ( pHeavyTeam->GetNumPlayers() - pLightTeam->GetNumPlayers() ) / 2;

	// sort the eligible players and switch the n best candidates
	CUtlVector<CBaseMultiplayerPlayer *> vecPlayers;

	CBaseMultiplayerPlayer *pPlayer;

	int iScore;

	int i;
	for ( i = 0; i < pHeavyTeam->GetNumPlayers(); i++ )
	{
		pPlayer = ToBaseMultiplayerPlayer( pHeavyTeam->GetPlayer(i) );

		if ( !pPlayer )
			continue;

		if ( !pPlayer->CanBeAutobalanced() )
			continue;

		// calculate a score for this player. higher is more likely to be switched
		iScore = pPlayer->CalculateTeamBalanceScore();

		pPlayer->SetTeamBalanceScore( iScore );

		vecPlayers.AddToTail( pPlayer );
	}

	// sort the vector
	vecPlayers.Sort( SwitchPlayersSort );

	int iNumEligibleSwitchees = iNumSwitchesRequired + 2;

	for ( int i=0; i<vecPlayers.Count() && iNumSwitchesRequired > 0 && i < iNumEligibleSwitchees; i++ )
	{
		pPlayer = vecPlayers.Element(i);

		Assert( pPlayer );

		if ( !pPlayer )
			continue;

		if ( bRequireSwitcheesToBeDead == false || !pPlayer->IsAlive() )
		{
			// We're trying to avoid picking a player that's recently
			// been auto-balanced by delaying their selection in the hope
			// that a better candidate comes along.
			if ( bRequireSwitcheesToBeDead )
			{
				int nPlayerTeamBalanceScore = pPlayer->CalculateTeamBalanceScore();

				// Do we already have someone in the queue?
				if ( m_nAutoBalanceQueuePlayerIndex > 0 )
				{
					// Is this player's score worse?
					if ( nPlayerTeamBalanceScore < m_nAutoBalanceQueuePlayerScore )
					{
						m_nAutoBalanceQueuePlayerIndex = pPlayer->entindex();
						m_nAutoBalanceQueuePlayerScore = nPlayerTeamBalanceScore;
					}
				}
				// Has this person been switched recently?
				else if ( nPlayerTeamBalanceScore < -10000 ) 
				{
					// Put them in the queue
					m_nAutoBalanceQueuePlayerIndex = pPlayer->entindex();
					m_nAutoBalanceQueuePlayerScore = nPlayerTeamBalanceScore;
					m_flAutoBalanceQueueTimeEnd = gpGlobals->curtime + 3.0f;

					continue;
				}

				// If this is the player in the queue...
				if ( m_nAutoBalanceQueuePlayerIndex == pPlayer->entindex() )
				{
					// Pass until their timer is up
					if ( m_flAutoBalanceQueueTimeEnd > gpGlobals->curtime )
						continue;
				}
			}
				
			pPlayer->ChangeTeam( iLightestTeam );
			pPlayer->SetLastForcedChangeTeamTimeToNow();

			m_nAutoBalanceQueuePlayerScore = -1;
			m_nAutoBalanceQueuePlayerIndex = -1;

			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_teambalanced_player" );
			if ( event )
			{
				event->SetInt( "player", pPlayer->entindex() );
				event->SetInt( "team", iLightestTeam );
				gameeventmanager->FireEvent( event );
			}

			// tell people that we've switched this player
			UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_player_was_team_balanced", pPlayer->GetPlayerName() );

			iNumSwitchesRequired--;
		}
	}
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::ResetScores( void )
{
	if ( m_bResetTeamScores )
	{
		for ( int i = 0; i < GetNumberOfTeams(); i++ )
		{
			GetGlobalTeam( i )->ResetScores();
		}
	}

	if ( m_bResetPlayerScores )
	{
		CBasePlayer *pPlayer;

		for( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );

			if (pPlayer == NULL)
				continue;

			if (FNullEnt( pPlayer->edict() ))
				continue;

			pPlayer->ResetScores();
		}

#ifdef TF_DLL
		IGameEvent *event = gameeventmanager->CreateEvent( "scorestats_accumulated_reset" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
#endif // TF_DLL
	}

	if ( m_bResetRoundsPlayed )
	{
		m_nRoundsPlayed = 0;
	}

	// assume we always want to reset the scores 
	// unless someone tells us not to for the next reset 
	m_bResetTeamScores = true;
#ifdef TF_DLL
	m_bResetPlayerScores = !TFGameRules() || !TFGameRules()->IsMatchPlayingOut();
#else
	m_bResetPlayerScores = true;
#endif
	m_bResetRoundsPlayed = true;
	//m_flStopWatchTime = -1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::ResetMapTime( void )
{
	m_flMapResetTime = gpGlobals->curtime;

	// send an event with the time remaining until map change
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_map_time_remaining" );
	if ( event )
	{
		event->SetInt( "seconds", GetTimeLeft() );
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::PlayStartRoundVoice( void )
{
	for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
	{
		BroadcastSound( i, UTIL_VarArgs("Game.TeamRoundStart%d", i ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::PlayWinSong( int team )
{
	if ( team == TEAM_UNASSIGNED )
	{
		PlayStalemateSong();
	}
	else
	{
		BroadcastSound( TEAM_UNASSIGNED, UTIL_VarArgs("Game.TeamWin%d", team ) );

		for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
		{
			if ( i == team )
			{
				BroadcastSound( i, WinSongName( i ) );
			}
			else
			{
				const char *pchLoseSong = LoseSongName( i );
				if ( pchLoseSong )
				{
					BroadcastSound( i, pchLoseSong );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::PlaySuddenDeathSong( void )
{
	BroadcastSound( TEAM_UNASSIGNED, "Game.SuddenDeath" );

	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		BroadcastSound( i, "Game.SuddenDeath" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::PlayStalemateSong( void )
{
	BroadcastSound( TEAM_UNASSIGNED, GetStalemateSong( TEAM_UNASSIGNED ) );

	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		BroadcastSound( i, GetStalemateSong( i ) );
	}
}

bool CTeamplayRoundBasedRules::PlayThrottledAlert( int iTeam, const char *sound, float fDelayBeforeNext )
{
	if ( m_flNewThrottledAlertTime <= gpGlobals->curtime )
	{
		BroadcastSound( iTeam, sound );
		m_flNewThrottledAlertTime = gpGlobals->curtime + fDelayBeforeNext;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::BroadcastSound( int iTeam, const char *sound, int iAdditionalSoundFlags /* = 0 */, CBasePlayer *pPlayer /* = NULL */ )
{
	//send it to everyone
	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_broadcast_audio" );
	if ( event )
	{
		event->SetInt( "team", iTeam );
		event->SetString( "sound", sound );
		event->SetInt( "additional_flags", iAdditionalSoundFlags );
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::AddPlayedRound( string_t strName )
{
	if ( strName != NULL_STRING )
	{
		m_iszPreviousRounds.AddToHead( strName );

		// we only need to store the last two rounds that we've played
		if ( m_iszPreviousRounds.Count() > 2 )
		{
			// remove all but two of the entries (should only ever have to remove 1 when we're at 3)
			for ( int i = m_iszPreviousRounds.Count() - 1 ; i > 1 ; i-- )
			{
				m_iszPreviousRounds.Remove( i );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::IsPreviouslyPlayedRound( string_t strName )
{
	return ( m_iszPreviousRounds.Find( strName ) != m_iszPreviousRounds.InvalidIndex() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
string_t CTeamplayRoundBasedRules::GetLastPlayedRound( void )
{
	return ( m_iszPreviousRounds.Count() ? m_iszPreviousRounds[0] : NULL_STRING );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::PlayerThink(CBasePlayer* pPlayer)
{
	if ( IsGamePaused() && pPlayer->ShouldBePausedDuringPause() )
	{
		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->m_nButtons = 0;
		pPlayer->m_afButtonReleased = 0;
	}
	BaseClass::PlayerThink( pPlayer );
}


//=========================================================
// ClientCommand
// the user has typed a command which is unrecognized by everything else;
// this check to see if the gamerules knows anything about the command
//=========================================================
bool CTeamplayRoundBasedRules::ClientCommand( CBaseEntity* pEdict, const CCommand& args )
{
	if ( BaseClass::ClientCommand( pEdict, args ) )
		return true;

	CBasePlayer* pPlayer = ToBasePlayer( pEdict );

	const char* pcmd = args[0];

	if ( FStrEq( pcmd, "pause_request_server" ) )
	{
		int iPauseType;
		if ( args.ArgC() < 2 )
		{
			iPauseType = 2; // toggle pause
		}
		else
		{
			iPauseType = atoi( args[1] );
		}
		
		bool bWantsPause;
		if ( iPauseType == 2 )
		{
			bWantsPause = !m_bGamePaused;
		}
		else
		{
			bWantsPause = iPauseType == 0;
		}

		CSteamID steamID;
		if ( !pPlayer->GetSteamID( &steamID ) )
		{
			return true;
		}

		if ( bWantsPause )
		{
			Pause( steamID );
		}
		else
		{
			Unpause( steamID );
		}
		
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::Pause( CSteamID SteamID )
{
	if ( !CanPlayerPause( SteamID ) )
	{
		return;
	}

	m_pausingPlayerId = SteamID;
	m_flPauseCurTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::Unpause( CSteamID SteamID )
{
	if ( !CanPlayerUnpause( SteamID ) )
	{
		return;
	}

	m_unpausingPlayerId = SteamID;
	m_flUnpauseCurTime = gpGlobals->curtime + mp_unpause_countdown.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::CanPlayerPause( CSteamID SteamID )
{
	// cannot pause if we're already paused.
	if ( IsGamePaused() )
	{
		return false;
	}
	
	if ( !IsPausingEnabled() )
	{
		return false;
	}

	// limited pauses
	if ( mp_pause_setting.GetInt() == 1 )
	{
		int iRemainingIndex = m_nPausesRemaining.Find( SteamID );
		if ( iRemainingIndex == m_nPausesRemaining.InvalidIndex() )
		{
			iRemainingIndex = m_nPausesRemaining.Insert( SteamID, mp_pause_count.GetInt() );
		}

		int iNumPausesRemaining = m_nPausesRemaining[iRemainingIndex];
		if ( iNumPausesRemaining <= 0 )
		{
			return false;
		}

		int iTimeIndex = m_nLastPauseTime.Find( SteamID );
		if ( iTimeIndex == m_nLastPauseTime.InvalidIndex() )
		{
			// hasn't paused before
			m_nLastPauseTime.Insert( SteamID, gpGlobals->tickcount );
		}
		else
		{
			// has paused before, check cooldown.
			const int iTicksPast = gpGlobals->tickcount - m_nLastPauseTime[iTimeIndex];
			if ( TICKS_TO_TIME( iTicksPast ) <= mp_pause_cooldown_time.GetFloat() )
			{
				return false;
			}
			// refresh pause time
			m_nLastPauseTime[iTimeIndex] = gpGlobals->tickcount;
		}

		// decrement pause
		m_nPausesRemaining[iRemainingIndex] = iNumPausesRemaining - 1;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::CanPlayerUnpause( CSteamID SteamID )
{
	// cannot unpause if we're not paused.
	if ( !IsGamePaused() )
	{
		return false;
	}

	// somehow, we're paused but pauses are disabled. just let someone unpause.
	if ( !IsPausingEnabled() )
	{
		return true;
	}

	// unpause already pending
	if ( m_flUnpauseCurTime > 0.0f )
	{
		return false;
	}

	// cannot unpause immediately. prevents accidental toggles and such.
	if ( m_flPauseTime <= mp_pause_cooldown.GetFloat() )
	{
		return false;
	}

	bool bSameTeam = false;
	int  iPausingTeam = 0;
	// get at the players pausing and unpausing.
	if ( SteamID.IsValid() && m_pausingPlayerId.IsValid() )
	{
		CBasePlayer* pPlayer = UTIL_PlayerBySteamID( SteamID );
		if ( pPlayer )
		{
			CBasePlayer* pPausingPlayer = UTIL_PlayerBySteamID( m_pausingPlayerId );
			if ( pPausingPlayer )
			{
				iPausingTeam = pPausingPlayer->GetTeamNumber();
				bSameTeam = pPlayer->GetTeamNumber() == iPausingTeam;
			}
		}
	}
	if ( !bSameTeam )
	{
		if ( iPausingTeam >= FIRST_GAME_TEAM )
		{
			// cannot unpause if the other team is not full, until resume time elapses.
			int iExpectedPlayers = GetTeamSize( iPausingTeam );
			int iCurrentPlayers = GetGlobalTeam( iPausingTeam )->GetNumPlayers();
			if ( iCurrentPlayers < iExpectedPlayers && m_flPauseTime <= mp_pause_same_team_resume_time_disconnected.GetFloat() )
			{
				return false;
			}
		}

		// general resume time check.
		if ( m_flPauseTime <= mp_pause_same_team_resume_time.GetFloat() )
		{
			return false;
		}
	}

	return true;
}
#endif // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::IsGamePaused()
{
	return m_bGamePaused;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::IsPausingEnabled()
{
	if ( !m_bPauseEnabled )
	{
		return false;
	}

	if ( !mp_pause_setting.GetBool() )
	{
		return false;
	}

	if ( mp_tournament_required_for_pause.GetBool() && !IsInTournamentMode() )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamRoundTimer *CTeamplayRoundBasedRules::GetActiveRoundTimer( void )
{
#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	int iTimerEntIndex = ObjectiveResource()->GetTimerToShowInHUD();
	CTeamRoundTimer *pTimer = NULL;

#ifdef GAME_DLL
	pTimer = ( dynamic_cast< CTeamRoundTimer * >( UTIL_EntityByIndex( iTimerEntIndex ) ) );
#else
	pTimer = ( dynamic_cast< CTeamRoundTimer * >( ClientEntityList().GetEnt( iTimerEntIndex ) ) );
#endif

	return pTimer;
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: How long are the respawn waves for this team currently?
//-----------------------------------------------------------------------------
float CTeamplayRoundBasedRules::GetRespawnWaveMaxLength( int iTeam, bool bScaleWithNumPlayers /* = true */ )
{
	if ( iTeam >= MAX_TEAMS )
		return 0;

	if ( State_Get() != GR_STATE_RND_RUNNING )
		return 0;

	if ( GetRespawnTimeMode() )
		return 0.0f;

	//Let's just turn off respawn times while players are messing around waiting for the tournament to start
	if ( IsInTournamentMode() == true && IsInPreMatch() == true )
		return 0.0f;

#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	static ConVarRef tf_tc2_mode( "tf_tc2_mode" );
	if ( tf_tc2_mode.GetBool() && bScaleWithNumPlayers )
	{
		return 1.0f;
	}
#endif

	float flTime = ( ( m_TeamRespawnWaveTimes[iTeam] >= 0 ) ? m_TeamRespawnWaveTimes[iTeam] : mp_respawnwavetime.GetFloat() );

	// For long respawn times, scale the time as the number of players drops
	if ( bScaleWithNumPlayers && flTime > 5 )
	{
		flTime = MAX( 5, flTime * GetRespawnTimeScalar(iTeam) );
	}

	return flTime;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we are running tournament mode
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::IsInTournamentMode( void )
{
	return mp_tournament.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we are running highlander mode
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::IsInHighlanderMode( void )
{
#if defined( TF_CLIENT_DLL ) || defined( TF_DLL )
	// can't use highlander mode and the queue system
	if ( IsInArenaMode() == true && tf_arena_use_queue.GetBool() == true )
		return false;

	return mp_highlander.GetBool();
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we are running highlander mode
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::IsInSixesMode(void)
{
#if defined( TF_CLIENT_DLL ) || defined( TF_DLL )
	// can't use sixes mode and the queue system
	if (IsInArenaMode() == true && tf_arena_use_queue.GetBool() == true)
		return false;

	if (!IsInTournamentMode())
		return false;

	if (!TFGameRules()->IsCompetitiveGame())
		return false;

	return mp_sixes.GetBool();
#else
	return false;
#endif
}

bool CTeamplayRoundBasedRules::IsAllTalkActive()
{
	bool bAllTalk = BaseClass::IsAllTalkActive();
	if (bAllTalk)
	{
		return true;
	}

	static ConVar* sv_alltalk_betweenrounds = cvar->FindVar("sv_alltalk_betweenrounds");
	if (!sv_alltalk_betweenrounds || !sv_alltalk_betweenrounds->GetBool())
	{
		return false;
	}

	if ( State_Get() == GR_STATE_STARTGAME || State_Get() == GR_STATE_PREROUND || ( State_Get() == GR_STATE_RND_RUNNING && !TFGameRules()->IsInWaitingForPlayers() ) )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamplayRoundBasedRules::GetBonusRoundTime( bool bGameOver /* = false*/ )
{
	return Max( 5, mp_bonusroundtime.GetInt() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamplayRoundBasedRules::GetPostMatchPeriod( void )
{
	return mp_tournament_post_match_period.GetInt();

}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::Update( float frametime )
{
	BaseClass::Update( frametime );

	int nTime = 0;

	if ( m_flRestartRoundTime > gpGlobals->curtime )
	{
		nTime = Ceil2Int( m_flRestartRoundTime - gpGlobals->curtime );
	}
	else if ( m_flCountdownTime > gpGlobals->curtime )
	{
		nTime = Ceil2Int( m_flCountdownTime - gpGlobals->curtime );
	}

	if ( nTime != m_nLastEventFiredTime )
	{
		m_nLastEventFiredTime = nTime;

		IGameEvent * event = gameeventmanager->CreateEvent( "restart_timer_time" );
		if ( event )
		{
			event->SetInt( "time", nTime );
			gameeventmanager->FireEventClientSide( event );
		}
	}
}
#endif

int CTeamplayRoundBasedRules::GetTeamSize( int iTeam )
{
	if ( IsInSixesMode() )
	{
		return 6;
	}

	if ( IsInHighlanderMode() )
	{
		return TF_LAST_NORMAL_CLASS - 1;
	}

	// means no team size restriction
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we should even bother to do balancing stuff
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::ShouldBalanceTeams( void )
{
	if ( IsInTournamentMode() )
		return false;

	if ( IsInTraining() || IsInItemTestingMode() )
		return false;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return false;
#endif // _DEBUG || STAGING_ONLY

	if ( mp_teams_unbalance_limit.GetInt() <= 0 )
		return false;

#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		return false;
#endif // TF_DLL

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the passed team change would cause unbalanced teams
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::WouldChangeUnbalanceTeams( int iNewTeam, int iCurrentTeam  )
{
	// players are allowed to change to their own team
	if ( iNewTeam == iCurrentTeam )
		return false;

	// if mp_teams_unbalance_limit is 0, don't check
	if ( !ShouldBalanceTeams() )
		return false;

#if defined( _DEBUG ) || defined( STAGING_ONLY )
	if ( mp_developer.GetBool() )
		return false;
#endif // _DEBUG || STAGING_ONLY

	// if they are joining a non-playing team, allow
	if ( iNewTeam < FIRST_GAME_TEAM )
		return false;

#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		return false;
#endif // TF_DLL

	CTeam *pNewTeam = GetGlobalTeam( iNewTeam );

	if ( !pNewTeam )
	{
		Assert( 0 );
		return true;
	}

	// add one because we're joining this team
	int iNewTeamPlayers = pNewTeam->GetNumPlayers() + 1;

	// for each game team
	int i = FIRST_GAME_TEAM;

	CTeam *pTeam;

	for ( pTeam = GetGlobalTeam(i); pTeam != NULL; pTeam = GetGlobalTeam(++i) )
	{
		if ( pTeam == pNewTeam )
			continue;

		int iNumPlayers = pTeam->GetNumPlayers();

		if ( i == iCurrentTeam )
		{
			iNumPlayers = MAX( 0, iNumPlayers-1 );
		}

		if ( ( iNewTeamPlayers - iNumPlayers ) > mp_teams_unbalance_limit.GetInt() )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamplayRoundBasedRules::AreTeamsUnbalanced( int &iHeaviestTeam, int &iLightestTeam )
{
	if ( !IsInArenaMode() || ( IsInArenaMode() && !tf_arena_use_queue.GetBool() ) )
	{
		if ( !ShouldBalanceTeams() )
			return false;
	}

#ifndef CLIENT_DLL
	if ( IsInCommentaryMode() )
		return false;
#endif

#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		return false;
#endif // TF_DLL

	int iMostPlayers = 0;
	int iLeastPlayers = MAX_PLAYERS + 1;

	int i = FIRST_GAME_TEAM;

	for ( CTeam *pTeam = GetGlobalTeam(i); pTeam != NULL; pTeam = GetGlobalTeam(++i) )
	{
		int iNumPlayers = pTeam->GetNumPlayers();

		if ( iNumPlayers < iLeastPlayers )
		{
			iLeastPlayers = iNumPlayers;
			iLightestTeam = i;
		}

		if ( iNumPlayers > iMostPlayers )
		{
			iMostPlayers = iNumPlayers;
			iHeaviestTeam = i;	
		}
	}

	if ( IsInArenaMode() && tf_arena_use_queue.GetBool() )
	{
		if ( iMostPlayers == 0 && iMostPlayers == iLeastPlayers )
			return true;

		if ( iMostPlayers != iLeastPlayers )
			return true;
		
		return false;
	}

	if ( ( iMostPlayers - iLeastPlayers ) > mp_teams_unbalance_limit.GetInt() )
	{
		return true;
	}

	return false;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::SetRoundState( int iRoundState )
{
	m_iRoundState = iRoundState;
	m_flLastRoundStateChangeTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::OnPreDataChanged( DataUpdateType_t updateType )
{
	m_bOldInWaitingForPlayers = m_bInWaitingForPlayers;
	m_bOldInOvertime = m_bInOvertime;
	m_bOldInSetup = m_bInSetup;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED || 
		 m_bOldInWaitingForPlayers != m_bInWaitingForPlayers ||
		 m_bOldInOvertime != m_bInOvertime ||
		 m_bOldInSetup != m_bInSetup )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_update_timer" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( State_Get() == GR_STATE_STALEMATE )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_stalemate" );
			if ( event )
			{
				event->SetInt( "reason", STALEMATE_JOIN_MID );
				gameeventmanager->FireEventClientSide( event );
			}
		}
	}

	if ( m_bInOvertime && ( m_bOldInOvertime != m_bInOvertime ) )
	{
		HandleOvertimeBegin();
	}
}
#endif // CLIENT_DLL

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRoundBasedRules::ResetTeamsRoundWinTracking( void )
{
	if ( m_GameTeams.Count() != 2 )
		return;

	m_GameTeams[0] = 0;
	m_GameTeams[1] = 0;
}
#endif // GAME_DLL
