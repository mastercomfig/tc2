//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "hltvdirector.h"
#include "team_control_point.h"


CBaseEntity* GetCapturePointByIndex( int iCaptureIndex )
{
	CTeamControlPoint *pTeamControlPoint = (CTeamControlPoint *)gEntList.FindEntityByClassname( NULL, "team_control_point" );

	while ( pTeamControlPoint )
	{
		if ( pTeamControlPoint->GetPointIndex() == iCaptureIndex )
		{
			return pTeamControlPoint;
		}

		pTeamControlPoint = (CTeamControlPoint *)gEntList.FindEntityByClassname( pTeamControlPoint, "team_control_point" );
	}

	return NULL;
}

class CTFHLTVDirector : public CHLTVDirector
{
public:
	DECLARE_CLASS( CTFHLTVDirector, CHLTVDirector );

	const char** GetModEvents();
	void SetHLTVServer( IHLTVServer *hltv );
	void CreateShotFromEvent( CHLTVGameEvent *event );

	virtual char	*GetFixedCameraEntityName( void ) { return "info_observer_point"; }
};

void CTFHLTVDirector::SetHLTVServer( IHLTVServer *hltv )
{
	BaseClass::SetHLTVServer( hltv );

	if ( m_pHLTVServer )
	{
		// mod specific events the director uses to find interesting shots
		ListenForGameEvent( "teamplay_point_captured" );
		ListenForGameEvent( "teamplay_capture_blocked" );
		ListenForGameEvent( "teamplay_point_startcapture" );
		ListenForGameEvent( "teamplay_flag_event" );
		ListenForGameEvent( "ctf_flag_captured" );
	}
}

void CTFHLTVDirector::CreateShotFromEvent( CHLTVGameEvent *event )
{
	// show event at least for 2 more seconds after it occured
	const char *name = event->m_Event->GetName();

	int thera = RandomFloat()>0.5?20:-20;

	if ( !Q_strcmp( "teamplay_point_startcapture", name ) || 
		 !Q_strcmp( "teamplay_point_captured", name ) ||
		 !Q_strcmp( "teamplay_capture_blocked", name ) )
	{
		CBaseEntity *pCapturePoint = GetCapturePointByIndex( event->m_Event->GetInt( "cp" ) );

		int iCameraIndex = -1;
		float flClosest = 99999.9f;
		
		if ( pCapturePoint )
		{
			// Does it have an associated viewpoint?
			for ( int i = 0; i<m_nNumFixedCameras; i++ )
			{
				CBaseEntity *pCamera = m_pFixedCameras[ i ];
		
				if ( pCamera )
				{
					byte pvs[MAX_MAP_CLUSTERS/8];
					int clusterIndex = engine->GetClusterForOrigin( pCamera->GetAbsOrigin() );
					engine->GetPVSForCluster( clusterIndex, sizeof(pvs), pvs );
					bool bCameraInPVS = engine->CheckOriginInPVS( pCapturePoint->GetAbsOrigin(), pvs, sizeof( pvs ) );

					if ( bCameraInPVS == true )
					{
						float flDistance = (pCapturePoint->GetAbsOrigin() - pCamera->GetAbsOrigin()).Length();
						if ( flDistance <= flClosest )
						{
							iCameraIndex = i;
							flClosest = flDistance;
						}
					}
				}
			}
		}

		CBasePlayer *pPlayer = NULL;

		if ( !Q_strcmp( "teamplay_point_captured", name ) )
		{
			const char *pszCappers = event->m_Event->GetString("cappers");
			int nLength = Q_strlen(pszCappers);

			if ( nLength > 0 )
			{
				int iRandomCapper = pszCappers[ RandomInt(0,nLength-1) ];
				pPlayer = UTIL_PlayerByIndex( iRandomCapper );
			}
		}
		else if ( !Q_strcmp( "teamplay_capture_blocked", name ) )
		{
			int iBlocker = event->m_Event->GetInt("blocker");
			pPlayer = UTIL_PlayerByIndex( iBlocker );
		}

		if ( pPlayer )
		{
			if ( iCameraIndex >= 0 && RandomFloat() > 0.66f )
			{
				StartFixedCameraShot( iCameraIndex, pPlayer->entindex() );
			}
			else if ( pCapturePoint )
			{
				StartChaseCameraShot( pPlayer->entindex(), pCapturePoint->entindex(), 96, 20, thera, false );
			}
			else
			{
				StartChaseCameraShot( pPlayer->entindex(), 0, 96, 20, 0, false );
			}
		}
		else if ( iCameraIndex >= 0 && pCapturePoint )
		{
			// no player known for this event
			StartFixedCameraShot( iCameraIndex, pCapturePoint->entindex() );
		}

		// shot 2 seconds after event
		m_nNextShotTick = MIN( m_nNextShotTick, (event->m_Tick+TIME_TO_TICKS(1.0)) );
	}
	else if ( !Q_strcmp( "object_destroyed", name ) )
	{
		CBasePlayer *attacker = UTIL_PlayerByUserId( event->m_Event->GetInt("attacker") );
		if ( attacker )
		{
			int iObjectIndex = event->m_Event->GetInt("index");
			StartChaseCameraShot( attacker->entindex(), iObjectIndex, 96, 20, thera, false );
		}
	}
	else if ( !Q_strcmp( "ctf_flag_captured", name ) )
	{
		CBasePlayer *capper = UTIL_PlayerByUserId( event->m_Event->GetInt("capper") );
		if ( capper )
		{
			StartChaseCameraShot( capper->entindex(), 0, 96, 20, 0, false );
		}
	}
	else if ( !Q_strcmp( "teamplay_flag_event", name ) )
	{
		StartChaseCameraShot( event->m_Event->GetInt("player"), 0, 96, 20, 0, false );
	}
	else
	{

		// let baseclass create a shot
		BaseClass::CreateShotFromEvent( event );
	}
}

const char** CTFHLTVDirector::GetModEvents()
{
	// game events relayed to spectator clients
	static const char *s_modevents[] =
	{
		"game_newmap",
		"hltv_status",
		"hltv_chat",
		"player_connect",
		"player_disconnect",
		"player_changeclass",
		"player_changename",
		"player_team",
		"player_info",
		"player_death",
		"player_chat",
		"player_spawn",
		"player_hurt",
		"round_start",
		"round_end",
		"server_cvar",
		"server_spawn",
		"client_disconnect",
				
		// additional TF events:
		"controlpoint_starttouch",
		"controlpoint_endtouch",
		"ctf_flag_captured",
		"teamplay_broadcast_audio",
		"teamplay_capture_blocked",
		"teamplay_flag_event",
		"teamplay_game_over",
		"teamplay_point_captured",
		"teamplay_round_stalemate",
		"teamplay_round_start",
		"teamplay_round_win",
		"teamplay_timer_time_added",
		"teamplay_update_timer",
		"teamplay_win_panel",
		"teamplay_setup_finished",
		"teamplay_alert",
		"teamplay_teambalanced_player",
		"teamplay_point_startcapture",
		"teamplay_round_active", // i don't think we need this one?
		"training_complete",
		"tf_game_over",
		"object_destroyed",
		"object_detonated",
		"base_player_teleported",
		"user_data_downloaded", // should we do this one?
		"achievement_event",
		"training_complete",
		"flagstatus_update",
		"player_stats_updated",
		"playing_commentary",
		"achievement_earned",
		"tournament_stateupdate",
		"player_teleported",
		"player_healonhit",
		"arena_match_maxstreak",
		"arena_round_start",
		"arena_win_panel",
		"pve_win_panel",
		"arrow_impact",
		"show_annotation",
		"hide_annotation",
		"post_inventory_application",
		"deploy_buff_banner",
		"teams_changed",
		"rocket_jump",
		"rocket_jump_landed",
		"sticky_jump",
		"sticky_jump_landed",
		"rocketpack_launch",
		"rocketpack_landed",
		//"raid_spawn_mob",
		//"raid_spawn_squad",
		"num_cappers_changed",
		"scorestats_accumulated_update",
		"scorestats_accumulated_reset",
		"player_healed",
		"building_healed",
		"item_pickup",
		"duel_status",
		"fish_notice",
		"fish_notice__arm",
		"slap_notice",
		"throwable_hit",
		// TODO: eyeball and merasmus
		"npc_hurt",
		"player_highfive_start",
		"player_highfive_cancel",
		"player_highfive_success",
		"player_upgraded",
		"player_buyback",
		"player_used_powerup_bottle",
		"mvm_mission_update",
		"recalculate_holidays",
		"mvm_begin_wave",
		"mvm_wave_complete",
		"mvm_reset_stats",
		"damage_resisted",
		"revive_player_notify",
		"revive_player_stopped",
		"upgrades_file_changed",
		"rd_rules_state_changed",
		"rd_robot_killed",
		"rd_robot_impact",
		"competitive_victory",
		"competitive_stats_update",
		"minigame_win",
		"sentry_on_go_active",
		"player_score_changed",
		"special_score",
		"competitive_state_changed",
		"team_leader_killed",
		"recalculate_truce",
		"player_abandoned_match",
		"cl_drawline",
		"restart_timer_time",
		"winlimit_changed",
		"stop_watch_changed",
		"show_match_summary",
		"hide_match_summary",
		"rematch_failed_to_create",
		"player_next_map_vote_change",
		"vote_maps_changed",
		// optional events for mods (don't do anything on their own)
#if 0
		"game_init",
		"teamplay_round_selected",
		"teamplay_ready_restart",
		"teamplay_round_restart_seconds",
		"teamplay_team_ready",
		"teamplay_map_time_remaining",
		"teamplay_point_locked",
		"teamplay_point_unlocked",
		"teamplay_capture_broken",
		"player_chargedeployed",
		"player_builtobject",
		"player_upgradedobject",
		// TODO: missed an object_ event here i think
		"player_ignited",
		"player_extinguished",
		"player_invulned",
		"player_stealsandvich",
		"player_stunned",
		"scout_grand_slam",
		"player_sapped_object",
		"player_buff",
		"medic_death",
		"medic_defended",
		"player_destroyed_pipebomb",
		"object_deflected",
		"nav_blocked",
		"player_regenerate",
		"update_status_item",
		"stats_resetround",
		"christmas_gift_grab",
		"player_killed_achievement_zone",
		"doomsday_rocket_open",
		"remove_nemesis_relationships",
		"mvm_creditbonus_wave",
		"mvm_creditbonus_all",
		"mvm_creditbonus_all_advanced",
		"mvm_quick_sentry_upgrade",
		"mvm_tank_destroyed_by_players",
		"mvm_kill_robot_delivering_bomb",
		"mvm_pickup_currency",
		"mvm_bomb_carrier_killed",
		"mvm_sentrybuster_detonate",
		"mvm_scout_marked_for_death",
		"mvm_medic_powerup_shared",
		"mvm_mission_complete",
		"mvm_bomb_reset_by_player",
		"mvm_bomb_alarm_triggered",
		"mvm_bomb_deploy_reset_by_player",
		"mvm_wave_failed",
		"revive_player_complete",
		"player_turned_to_ghost",
		"medigun_shield_blocked_damage",
		"mvm_adv_wave_complete_no_gates",
		"mvm_sniper_headshot_currency",
		"mvm_mannhattan_pit",
		"flag_carried_in_detection_zone",
		"mvm_adv_wave_killed_stun_radio",
		"player_directhit_stun",
		"mvm_sentrybuster_killed",
		"teamplay_pre_round_time_left",
		"parachute_deploy",
		"parachute_holster",
		"kill_refills_meter",
		"rps_taunt_event",
		"conga_kill",
		"player_initial_spawn",
		"questlog_opened",
		"rd_player_score_points",
		"demoman_det_stickies",
		"killed_capping_player",
		"environmental_death",
		"projectile_direct_hit",
		"pass_get",
		"pass_score",
		"pass_free",
		"pass_pass_caught",
		"pass_ball_stolen",
		"pass_ball_blocked",
		"damage_prevented",
		"halloween_boss_killed",
		"escaped_loot_island",
		"tagged_player_as_it",
		"merasmus_stunned",
		"merasmus_prop_found",
		"halloween_skeleton_killed",
		"skeleton_killed_quest",
		"skeleton_king_killed_quest",
		"escape_hell",
		"cross_spectral_bridge",
		"minigame_won",
		"respawn_ghost",
		"kill_in_hell",
		"halloween_duck_collected",
		"halloween_soul_collected",
		"deadringer_cheat_death",
		"crossbow_heal",
		"damage_mitigated",
		"payload_pushed",
		"player_domination",
		"player_rocketpack_pushed",
		"projectile_removed",
		"gas_doused_player_ignited",
		"capper_killed",
		"killed_ball_carrier",
#endif
			
		NULL
	};

	return s_modevents;
}

static CTFHLTVDirector s_HLTVDirector;	// singleton

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CHLTVDirector, IHLTVDirector, INTERFACEVERSION_HLTVDIRECTOR, s_HLTVDirector );

CHLTVDirector* HLTVDirector()
{
	return &s_HLTVDirector;
}

IGameSystem* HLTVDirectorSystem()
{
	return &s_HLTVDirector;
}
