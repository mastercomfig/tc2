//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_pipebomblauncher.h"
#include "tf_fx_shared.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "in_buttons.h"
#include "datacache/imdlcache.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "prediction.h"
#include "c_tf_gamestats.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

#define TF_PIPEBOMB_HIGHLIGHT 1
#define TF_PIPEBOMB_DETONATE  2

#define TF_WEAPON_PIPEBOMBD_MODEL		"models/weapons/w_models/w_stickybomb_d.mdl"

#define TF_WEAPON_PIPEBOMB_LAUNCHER_CHARGE_SOUND	"Weapon_StickyBombLauncher.ChargeUp"

ConVar tf_scotres_inner_radius( "tf_scotres_inner_radius", "0.16", FCVAR_REPLICATED | FCVAR_NOTIFY, "Inner screen-space radius for Scottish Resistance reticle cluster targeting." );
ConVar tf_scotres_outer_radius( "tf_scotres_outer_radius", "0.22", FCVAR_REPLICATED | FCVAR_NOTIFY, "Outer screen-space radius for Scottish Resistance reticle loose/individual targeting." );
ConVar tf_scotres_chain_step( "tf_scotres_chain_step", "0.8", FCVAR_REPLICATED | FCVAR_NOTIFY, "Scottish Resistance cluster chaining maximum step distance factor (relative to bomb damage radius)." );
ConVar tf_scotres_chain_budget( "tf_scotres_chain_budget", "2.5", FCVAR_REPLICATED | FCVAR_NOTIFY, "Scottish Resistance cluster chaining maximum total path distance factor (relative to anchor bomb damage radius)." );
ConVar tf_scotres_aim_score( "tf_scotres_aim_score", "500000", FCVAR_REPLICATED | FCVAR_NOTIFY, "Aiming closeness score for selecting Scottish Resistance anchor bomb." );

//=============================================================================
//
// Weapon Pipebomb Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFPipebombLauncher, DT_WeaponPipebombLauncher )

BEGIN_NETWORK_TABLE_NOBASE( CTFPipebombLauncher, DT_PipebombLauncherLocalData )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iPipebombCount ) ),
	RecvPropFloat( RECVINFO( m_flChargeBeginTime ) ),
#else
	SendPropInt( SENDINFO( m_iPipebombCount ), 5, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flChargeBeginTime ) ),
#endif
END_NETWORK_TABLE()


BEGIN_NETWORK_TABLE( CTFPipebombLauncher, DT_WeaponPipebombLauncher )
#ifdef CLIENT_DLL
	RecvPropDataTable( "PipebombLauncherLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_PipebombLauncherLocalData ) ),
#else
	SendPropDataTable( "PipebombLauncherLocalData", 0, &REFERENCE_SEND_TABLE( DT_PipebombLauncherLocalData ), SendProxy_SendLocalWeaponDataTable ),
#endif	
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFPipebombLauncher )
	DEFINE_FIELD(  m_flChargeBeginTime, FIELD_FLOAT )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_pipebomblauncher, CTFPipebombLauncher );
PRECACHE_WEAPON_REGISTER( tf_weapon_pipebomblauncher );

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFPipebombLauncher )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon Pipebomb Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFPipebombLauncher::CTFPipebombLauncher()
{
	m_bReloadsSingly = true;
	m_flLastDenySoundTime = 0.0f;
	m_bNoAutoRelease = false;
	m_bWantsToShoot = false;
	m_iPipebombCount = 0;
#ifdef CLIENT_DLL
	m_flNextBombCheckTime = 0;
	m_bBombThinking = false;
	m_flDetonateFlashTime = 0.0f;
	m_flDetonateFlashRadius = 0.0f;
	m_bDetonateFlashColor = false;
	m_bTargetingInner = false;
	m_bTargetingOuter = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFPipebombLauncher::~CTFPipebombLauncher()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::Spawn( void )
{
	m_iAltFireHint = HINT_ALTFIRE_PIPEBOMBLAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFPipebombLauncher::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef CLIENT_DLL
	if ( m_flChargeBeginTime > 0.f )
	{
		StopSound( TF_WEAPON_PIPEBOMB_LAUNCHER_CHARGE_SOUND );
	}
#endif
	m_flChargeBeginTime = 0;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFPipebombLauncher::Deploy( void )
{
	m_flChargeBeginTime = 0;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::WeaponReset( void )
{
	BaseClass::WeaponReset();

#ifndef CLIENT_DLL
	DetonateRemotePipebombs( true );
#else
	if (m_flChargeBeginTime > 0.f)
	{
		StopSound( TF_WEAPON_PIPEBOMB_LAUNCHER_CHARGE_SOUND );
	}
#endif

	m_flChargeBeginTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_flChargeBeginTime > 0 )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( !pPlayer )
			return;

		// If we're not holding down the attack button, launch our grenade
		if ( m_iClip1 > 0  && !(pPlayer->m_nButtons & IN_ATTACK) && (pPlayer->m_afButtonReleased & IN_ATTACK) )
		{
			LaunchGrenade();
		}
		else if ( !m_bNoAutoRelease )
		{
			float flTotalChargeTime = gpGlobals->curtime - m_flChargeBeginTime;
			if ( flTotalChargeTime >= GetChargeForceReleaseTime() )
			{
				ForceLaunchGrenade();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::PrimaryAttack( void )
{
	// Check for ammunition.
	if ( m_iClip1 <= 0 && m_iClip1 != -1 )
		return;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if ( !CanAttack() )
	{
		m_flNextPrimaryAttack = MAX(m_flNextPrimaryAttack, gpGlobals->curtime);
		m_flChargeBeginTime = 0;
		return;
	}

	if ( m_flChargeBeginTime <= 0 )
	{
		// Set the weapon mode.
		m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

		// save that we had the attack button down
		m_flChargeBeginTime = gpGlobals->curtime;

		SendWeaponAnim( ACT_VM_PULLBACK );

#ifdef CLIENT_DLL
		EmitSound( TF_WEAPON_PIPEBOMB_LAUNCHER_CHARGE_SOUND );
#endif // CLIENT_DLL
	}
	else
	{
		float flTotalChargeTime = gpGlobals->curtime - m_flChargeBeginTime;

		if ( flTotalChargeTime >= GetChargeMaxTime() )
		{
			LaunchGrenade();
		}
	}

#ifdef CLIENT_DLL
	if ( GetDetonateMode() == TF_DETONATE_MODE_DOT && !m_bBombThinking )
	{
		m_bBombThinking = true;
		SetContextThink( &CTFPipebombLauncher::BombHighlightThink, gpGlobals->curtime + 0.1f, "BOMB_HIGHLIGHT_THINK" );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void CTFPipebombLauncher::BombHighlightThink( void )
{
	ModifyPipebombsInView( TF_PIPEBOMB_HIGHLIGHT );
	if ( GetOwner() )
	{
		SetContextThink( &CTFPipebombLauncher::BombHighlightThink, gpGlobals->curtime + 0.1f, "BOMB_HIGHLIGHT_THINK" );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::WeaponIdle( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( m_flChargeBeginTime > 0 && m_iClip1 > 0 && (pPlayer->m_afButtonReleased & IN_ATTACK) )
	{
		if ( m_iClip1 > 0 )
		{
			m_bWantsToShoot = true;
		}
	}

	if ( m_bWantsToShoot )
	{
		LaunchGrenade();
	}
	else
	{
		BaseClass::WeaponIdle();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::LaunchGrenade( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	m_bWantsToShoot = false;

	CalcIsAttackCritical();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	CTFGrenadePipebombProjectile *pProjectile = static_cast<CTFGrenadePipebombProjectile*>( FireProjectile( pPlayer ) );
	if ( pProjectile )
	{
		// Save the charge time to scale the detonation timer.
		pProjectile->SetChargeTime( gpGlobals->curtime - m_flChargeBeginTime );

#ifdef GAME_DLL
		if ( GetDetonateMode() == TF_DETONATE_MODE_AIR )
		{
			pProjectile->m_bWallShatter = true;
		}
		else if ( GetDetonateMode() == TF_DETONATE_MODE_DOT )
		{
			pProjectile->m_bDefensiveBomb = true;
			pProjectile->SetModel( TF_WEAPON_PIPEBOMBD_MODEL );
		}

		float flChargeDmg = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT( flChargeDmg, stickybomb_charge_damage_increase );
		if ( flChargeDmg != 1.0f )
		{
			float flDamage = pProjectile->GetDamage();
			flDamage += flDamage * ( flChargeDmg - 1.0f ) * GetCurrentCharge();
			pProjectile->SetDamage( flDamage );
		}
#endif	// GAME_DLL
	}

#ifdef CLIENT_DLL
	C_CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
	StopSound( TF_WEAPON_PIPEBOMB_LAUNCHER_CHARGE_SOUND );
#else
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set next attack times.

	float flFireDelay = ApplyFireDelay( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay );

	m_flNextPrimaryAttack = gpGlobals->curtime + flFireDelay;
	m_flLastDenySoundTime = gpGlobals->curtime;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	// Check the reload mode and behave appropriately.
	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}

	m_flChargeBeginTime = 0;

	if ( TFGameRules()->GameModeUsesUpgrades() )
	{
		PlayUpgradedShootSound( "Weapon_Upgrade.DamageBonus" );
	}
}

float CTFPipebombLauncher::GetProjectileSpeed( void )
{
	float flForwardSpeed = RemapValClamped( ( gpGlobals->curtime - m_flChargeBeginTime ),
		0.0f,
		GetChargeMaxTime(),
		TF_PIPEBOMB_MIN_CHARGE_VEL,
		TF_PIPEBOMB_MAX_CHARGE_VEL );

	return flForwardSpeed;
}

void CTFPipebombLauncher::AddPipeBomb( CTFGrenadePipebombProjectile *pBomb )
{
	PipebombHandle hHandle;
	hHandle = pBomb;
	m_Pipebombs.AddToTail( hHandle );
}

//-----------------------------------------------------------------------------
// Purpose: Add pipebombs to our list as they're fired
//-----------------------------------------------------------------------------
CBaseEntity *CTFPipebombLauncher::FireProjectile( CTFPlayer *pPlayer )
{
	CBaseEntity *pProjectile = BaseClass::FireProjectile( pPlayer );
	if ( pProjectile )
	{
#ifdef GAME_DLL
		// If we've gone over the max pipebomb count, detonate the oldest
		int nMaxPipebombs = TF_WEAPON_PIPEBOMB_COUNT;
		CALL_ATTRIB_HOOK_INT( nMaxPipebombs, add_max_pipebombs );
		if ( m_Pipebombs.Count() >= nMaxPipebombs )
		{
			CTFGrenadePipebombProjectile *pTemp = m_Pipebombs[0];
			if ( pTemp )
			{
				pTemp->SetDetonateTimerLength( 0.0f, true ); // explode NOW
			}

			m_Pipebombs.Remove(0);
		}

		CTFGrenadePipebombProjectile *pPipebomb = (CTFGrenadePipebombProjectile*)pProjectile;

		PipebombHandle hHandle;
		hHandle = pPipebomb;
		m_Pipebombs.AddToTail( hHandle );

		m_iPipebombCount = m_Pipebombs.Count();
#endif
	}

	return pProjectile;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs if secondary fire is down.
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::ItemBusyFrame( void )
{
#ifdef GAME_DLL
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner && pOwner->m_nButtons & IN_ATTACK2 )
	{
		// We need to do this to catch the case of player trying to detonate
		// pipebombs while in the middle of reloading.
		SecondaryAttack();
	}
#endif

	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Detonate active pipebombs
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::SecondaryAttack( void )
{
	const bool bDoSkill = DetonateAction();
	CTFPlayer* pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer )
	{
		// only update shared skill cooldown if it's not coming up already
		if ( pPlayer->m_Shared.GetNextClassSpecialTime() <= gpGlobals->curtime )
		{
			pPlayer->m_Shared.SetNextClassSpecialTime( gpGlobals->curtime + ( bDoSkill ? 0.25f : 0.1f ) );
		}
	}
}

bool CTFPipebombLauncher::DetonateAction()
{
	if ( !CanAttack( TF_CAN_ATTACK_FLAG_PIPEBOMBLAUNCHER_SECONDARY ) )
		return false;

	if ( m_iPipebombCount > 0 )
	{
		// Get a valid player.
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if ( !pPlayer )
		{
			return false;
		}

		if ( pPlayer->m_Shared.GetNextClassSpecialTime() > gpGlobals->curtime )
		{
			if ( m_flLastDenySoundTime <= gpGlobals->curtime )
			{
				// Deny!
				m_flLastDenySoundTime = gpGlobals->curtime + 1;
				WeaponSound( SPECIAL2 );
			}
			return false;
		}

		//If one or more pipebombs failed to detonate then play a sound.
		if ( DetonateRemotePipebombs( false ) == true )
		{
			if ( m_flLastDenySoundTime <= gpGlobals->curtime )
			{
				// Deny!
				m_flLastDenySoundTime = gpGlobals->curtime + 1;
				WeaponSound( SPECIAL2 );
			}
			return false;
		}
		else
		{
			// Play a detonate sound.
			WeaponSound( SPECIAL3 );

#ifdef GAME_DLL
			IGameEvent *pDetEvent = gameeventmanager->CreateEvent( "demoman_det_stickies" );

			if ( pDetEvent )
			{
				pDetEvent->SetInt( "player", pPlayer->entindex() );

				// Send the event
				gameeventmanager->FireEvent( pDetEvent );
			}
#endif
			return true;
		}
	}
	else
	{
#ifdef CLIENT_DLL
		m_flDetonateFlashTime = gpGlobals->curtime;
		m_flDetonateFlashRadius = -1.0f; // Failure flash!
		m_bDetonateFlashColor = false;
#endif
	}
	return false;
}

//=============================================================================
//
// Server specific functions.
//
#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::UpdateOnRemove(void)
{
	// If we just died, we want to fizzle our pipebombs.
	// If the player switched classes, our pipebombs have already been removed.
	DetonateRemotePipebombs( true );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::ApplyPostHitEffects( const CTakeDamageInfo &inputInfo, CTFPlayer *pPlayer )
{
	if ( !GetTFPlayerOwner() )
		return;

	if ( pPlayer->m_Shared.GetWeaponKnockbackID() == -1 )
	{
		pPlayer->m_Shared.SetWeaponKnockbackID( GetTFPlayerOwner()->GetUserID() );
	}
}

#endif


//-----------------------------------------------------------------------------
// Purpose: If a pipebomb has been removed, remove it from our list
//-----------------------------------------------------------------------------
void CTFPipebombLauncher::DeathNotice( CBaseEntity *pVictim )
{
	Assert( dynamic_cast<CTFGrenadePipebombProjectile*>(pVictim) );

	PipebombHandle hHandle;
	hHandle = (CTFGrenadePipebombProjectile*)pVictim;
	m_Pipebombs.FindAndRemove( hHandle );

	m_iPipebombCount = m_Pipebombs.Count();
}


//-----------------------------------------------------------------------------
// Purpose: Remove *with* explosions
//-----------------------------------------------------------------------------
bool CTFPipebombLauncher::DetonateRemotePipebombs( bool bFizzle )
{
	if ( GetDetonateMode() == TF_DETONATE_MODE_DOT && !bFizzle )
	{
		return ModifyPipebombsInView( TF_PIPEBOMB_DETONATE );
	}

	bool bFailedToDetonate = false;

	int count = m_Pipebombs.Count();

	for ( int i = 0; i < count; i++ )
	{
		CTFGrenadePipebombProjectile *pTemp = m_Pipebombs[i];
		if ( pTemp )
		{
			//This guy will die soon enough.
			if ( pTemp->IsEffectActive( EF_NODRAW ) )
				continue;
#ifdef GAME_DLL
			{
				CDisablePredictionFiltering disabler;
				if (bFizzle)
				{
					pTemp->Fizzle();
				}
			}
#endif

			if ( bFizzle == false )
			{
				if ( ( gpGlobals->curtime - pTemp->m_flCreationTime ) < pTemp->GetLiveTime() )
				{
					if ( pTemp->GetLiveTime() <= 0.5f )
					{
						pTemp->SetDetonateOnPulse( true );
					}
					bFailedToDetonate = true;
					continue;
				}
			}
#ifdef GAME_DLL
			{
				CDisablePredictionFiltering disabler;
				if (CanDestroyStickies())
				{
					pTemp->DetonateStickies();
				}
				pTemp->Detonate();
			}
#endif
		}
	}

	return bFailedToDetonate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPipebombLauncher::ModifyPipebombsInView( int iEffect )
{
	CTFPlayer* pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return true;

	// Dot product from the view angle to determine which bombs to detonate.
	Vector vecPlayerForward;
	AngleVectors( pPlayer->Weapon_EyeAngles(), &vecPlayerForward, NULL, NULL );
	vecPlayerForward.NormalizeInPlace();

	// Determine the dynamic dot product thresholds based on FOV
	const float flInnerScreenRadius = tf_scotres_inner_radius.GetFloat(); 
	const float flOuterScreenRadius = tf_scotres_outer_radius.GetFloat(); 

	float flFOV = pPlayer->GetFOV(); 
	float flTanHalfVert = 0.75f * tan( DEG2RAD( flFOV ) * 0.5f );
	float flInnerThreshold = cos( atan( flInnerScreenRadius * flTanHalfVert ) );
	float flOuterThreshold = cos( atan( flOuterScreenRadius * flTanHalfVert ) );

	int count = m_Pipebombs.Count();

#ifdef CLIENT_DLL
	if ( iEffect == TF_PIPEBOMB_HIGHLIGHT )
	{
		m_bTargetingInner = false;
		m_bTargetingOuter = false;
	}
#endif

	bool bDetonatedInner = false;
	bool bDetonatedOuter = false;

	// Find the anchoring bomb
	CTFGrenadePipebombProjectile *pAnchorBomb = NULL;
	float flBestDist = FLT_MAX; // Start with an infinitely large distance
	for ( int i = 0; i < count; ++i )
	{
		CTFGrenadePipebombProjectile *pTemp = m_Pipebombs[i];
		if ( !pTemp || pTemp->IsEffectActive( EF_NODRAW ) )
			continue;

		Vector vecToTarget = pTemp->WorldSpaceCenter() - pPlayer->EyePosition();
		float flDistToBomb = vecToTarget.NormalizeInPlace();
		
		float flDot = DotProduct( vecToTarget, vecPlayerForward );

		// must be inside the inner targeting circle
		if ( flDot > flInnerThreshold )
		{
			bool bArmed = ( ( gpGlobals->curtime - pTemp->m_flCreationTime ) > pTemp->GetLiveTime() );
			if ( bArmed )
			{
				// Prioritize bombs we are looking directly at, but still factor in distance slightly.
				// The score is lower the better.
				float flScore = ( 1.0f - flDot ) / ( 1.0f - flInnerThreshold ) * tf_scotres_aim_score.GetFloat() + flDistToBomb;
				
				if ( flScore < flBestDist )
				{
					flBestDist = flScore;
					pAnchorBomb = pTemp;
				}
			}
		}
	}

	// If we have an anchor bomb, find all connected bombs in the same cluster using Dijkstra's algorithm.
	// This propagates detonation to adjacent bombs but enforces a path-distance budget to prevent bridging to separate traps.
	CUtlVector<CTFGrenadePipebombProjectile*> clusterBombs;
	int iAnchorIndex = -1;
	if ( pAnchorBomb )
	{
		int iClusteringCount = ( count > 128 ) ? 128 : count;
		for ( int i = 0; i < iClusteringCount; ++i )
		{
			if ( m_Pipebombs[i] == pAnchorBomb )
			{
				iAnchorIndex = i;
				break;
			}
		}

		if ( iAnchorIndex != -1 )
		{
			float flMinPathDist[128];
			bool bVisited[128];
			for ( int i = 0; i < iClusteringCount; ++i )
			{
				flMinPathDist[i] = FLT_MAX;
				bVisited[i] = false;
			}

			flMinPathDist[iAnchorIndex] = 0.0f;
			float flMaxBudget = pAnchorBomb->GetDamageRadius() * tf_scotres_chain_budget.GetFloat();

			for ( int step = 0; step < iClusteringCount; ++step )
			{
				// Find the unvisited node with the smallest path distance
				int iCurrent = -1;
				float flBestPathDist = FLT_MAX;
				for ( int i = 0; i < iClusteringCount; ++i )
				{
					if ( !bVisited[i] && flMinPathDist[i] < flBestPathDist )
					{
						flBestPathDist = flMinPathDist[i];
						iCurrent = i;
					}
				}

				// If no node is reachable or we exceeded the budget, we are done
				if ( iCurrent == -1 || flBestPathDist > flMaxBudget )
					break;

				bVisited[iCurrent] = true;
				CTFGrenadePipebombProjectile *pCurrent = m_Pipebombs[iCurrent];
				if ( pCurrent && !pCurrent->IsEffectActive( EF_NODRAW ) )
				{
					clusterBombs.AddToTail( pCurrent );

					// Update distances to neighbors
					for ( int i = 0; i < iClusteringCount; ++i )
					{
						if ( bVisited[i] )
							continue;

						CTFGrenadePipebombProjectile *pTemp = m_Pipebombs[i];
						if ( !pTemp || pTemp->IsEffectActive( EF_NODRAW ) )
							continue;

						float flStepDist = pCurrent->GetAbsOrigin().DistTo( pTemp->GetAbsOrigin() );
						
						// Chaining check: adjacent bombs must be close enough (relative to bomb damage radius)
						const float flMaxStep = pCurrent->GetDamageRadius() * tf_scotres_chain_step.GetFloat();
						if ( flStepDist <= flMaxStep )
						{
							float flNewDist = flBestPathDist + flStepDist;
							if ( flNewDist < flMinPathDist[i] )
							{
								flMinPathDist[i] = flNewDist;
							}
						}
					}
				}
			}
		}
	}

	// detonate all valid bombs
	bool bFailedToDetonate = true;
	for ( int i = 0; i < count; ++i )
	{
		CTFGrenadePipebombProjectile *pTemp = m_Pipebombs[i];
		if ( !pTemp )
			continue;

		if ( pTemp->IsEffectActive( EF_NODRAW ) )
		{
			if ( iEffect == TF_PIPEBOMB_HIGHLIGHT )
			{
#ifdef CLIENT_DLL
				pTemp->SetHighlight( false );
#endif
			}
			continue;
		}

		float flDistToPlayer = pPlayer->GetAbsOrigin().DistTo( pTemp->GetAbsOrigin() );
		bool bShouldDetonate = false;
		bool bArmed = ( ( gpGlobals->curtime - pTemp->m_flCreationTime ) > pTemp->GetLiveTime() );

		// 1. Detonate around the anchor bomb (entire connected cluster)
		if ( pAnchorBomb )
		{
			if ( clusterBombs.Find( pTemp ) != clusterBombs.InvalidIndex() )
			{
				bShouldDetonate = true;
			}
		}

		// Calculate dot product to see if inside targeting circles
		Vector vecToTarget = pTemp->WorldSpaceCenter() - pPlayer->EyePosition();
		vecToTarget.NormalizeInPlace();
		float flDot = DotProduct( vecToTarget, vecPlayerForward );

		// 2. Detonate any bomb inside the outer screen radius but outside the inner screen radius
		if ( bArmed && flDot > flOuterThreshold && flDot <= flInnerThreshold )
		{
			bShouldDetonate = true;
		}

		// 3. Sticky jumping should detonate under player feet, but only if they are in the air
		if ( !( pPlayer->GetFlags() & FL_ONGROUND ) && flDistToPlayer < pTemp->GetDamageRadius() )
		{
			bShouldDetonate = true;
		}

		// Execution
		if ( bShouldDetonate && bArmed )
		{
			switch ( iEffect )
			{
			case TF_PIPEBOMB_HIGHLIGHT:
#ifdef CLIENT_DLL
				pTemp->SetHighlight( true );
				if ( pAnchorBomb && clusterBombs.Find( pTemp ) != clusterBombs.InvalidIndex() )
				{
					m_bTargetingInner = true;
				}
				if ( flDot > flOuterThreshold )
				{
					m_bTargetingOuter = true;
				}
#endif
				break;
			case TF_PIPEBOMB_DETONATE:
				bFailedToDetonate = false;

				// Track which region succeeded in detonating a bomb for the visual indicator
				if ( pAnchorBomb && clusterBombs.Find( pTemp ) != clusterBombs.InvalidIndex() )
				{
					bDetonatedInner = true;
				}
				if ( flDot > flOuterThreshold )
				{
					bDetonatedOuter = true;
				}

#ifdef GAME_DLL
				if ( CanDestroyStickies() )
				{
					pTemp->DetonateStickies();
				}
#endif
				pTemp->Detonate();
				break;
			}
		}
		else if ( iEffect == TF_PIPEBOMB_HIGHLIGHT )
		{
#ifdef CLIENT_DLL
			pTemp->SetHighlight( false );
#endif
		}
	}

#ifdef CLIENT_DLL
	if ( iEffect == TF_PIPEBOMB_DETONATE )
	{
		m_flDetonateFlashTime = gpGlobals->curtime;
		if ( !bFailedToDetonate )
		{
			if ( bDetonatedInner )
			{
				m_flDetonateFlashRadius = flInnerScreenRadius;
				m_bDetonateFlashColor = true;
			}
			else if ( bDetonatedOuter )
			{
				m_flDetonateFlashRadius = flOuterScreenRadius;
				m_bDetonateFlashColor = false;
			}
		}
		else
		{
			// Failure! Flash both boundaries in red to show targeting limits
			m_flDetonateFlashRadius = -1.0f;
			m_bDetonateFlashColor = false;
		}
	}
#endif

	return bFailedToDetonate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPipebombLauncher::Reload( void )
{
	if ( m_flChargeBeginTime > 0 )
		return false;

	return BaseClass::Reload();
}

#ifdef CLIENT_DLL
ConVar cl_scotres_ring_draw_inner( "cl_scotres_ring_draw_inner", "2", FCVAR_ARCHIVE, "Draw the Scottish Resistance inner reticle ring (0 = flash on det, 1 = always, 2 = only when targeting)" );
ConVar cl_scotres_ring_draw_outer( "cl_scotres_ring_draw_outer", "2", FCVAR_ARCHIVE, "Draw the Scottish Resistance outer reticle ring (0 = flash on det, 1 = always, 2 = only when targeting)" );
ConVar cl_scotres_ring_thickness( "cl_scotres_ring_thickness", "2", FCVAR_ARCHIVE, "Thickness of the Scottish Resistance reticle rings in pixels" );
ConVar cl_scotres_ring_inner_r( "cl_scotres_ring_inner_r", "0", FCVAR_ARCHIVE, "Red component of the Scottish Resistance inner ring color" );
ConVar cl_scotres_ring_inner_g( "cl_scotres_ring_inner_g", "255", FCVAR_ARCHIVE, "Green component of the Scottish Resistance inner ring color" );
ConVar cl_scotres_ring_inner_b( "cl_scotres_ring_inner_b", "255", FCVAR_ARCHIVE, "Blue component of the Scottish Resistance inner ring color" );
ConVar cl_scotres_ring_inner_a( "cl_scotres_ring_inner_a", "128", FCVAR_ARCHIVE, "Alpha component of the Scottish Resistance inner ring color" );

ConVar cl_scotres_ring_outer_r( "cl_scotres_ring_outer_r", "255", FCVAR_ARCHIVE, "Red component of the Scottish Resistance outer ring color" );
ConVar cl_scotres_ring_outer_g( "cl_scotres_ring_outer_g", "128", FCVAR_ARCHIVE, "Green component of the Scottish Resistance outer ring color" );
ConVar cl_scotres_ring_outer_b( "cl_scotres_ring_outer_b", "0", FCVAR_ARCHIVE, "Blue component of the Scottish Resistance outer ring color" );
ConVar cl_scotres_ring_outer_a( "cl_scotres_ring_outer_a", "128", FCVAR_ARCHIVE, "Alpha component of the Scottish Resistance outer ring color" );

ConVar cl_scotres_ring_fail_r( "cl_scotres_ring_fail_r", "220", FCVAR_ARCHIVE, "Red component of the Scottish Resistance failure rings color" );
ConVar cl_scotres_ring_fail_g( "cl_scotres_ring_fail_g", "40", FCVAR_ARCHIVE, "Green component of the Scottish Resistance failure rings color" );
ConVar cl_scotres_ring_fail_b( "cl_scotres_ring_fail_b", "40", FCVAR_ARCHIVE, "Blue component of the Scottish Resistance failure rings color" );
ConVar cl_scotres_ring_fail_a( "cl_scotres_ring_fail_a", "96", FCVAR_ARCHIVE, "Alpha component of the Scottish Resistance failure rings color" );

void CTFPipebombLauncher::DrawCrosshair( void )
{
	// Draw the base crosshair first
	BaseClass::DrawCrosshair();

	if ( GetDetonateMode() != TF_DETONATE_MODE_DOT )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Compute screen space center
	int xCenter = ScreenWidth() / 2;
	int yCenter = ScreenHeight() / 2;

	bool bInFlash = ( m_flDetonateFlashTime > 0.0f && gpGlobals->curtime < m_flDetonateFlashTime + 0.3f );
	bool bFlashingFailure = bInFlash && ( m_flDetonateFlashRadius < 0.0f );

	// Draw both rings
	for ( int ring = 0; ring < 2; ++ring )
	{
		bool bIsInner = ( ring == 0 );
		int nDrawMode = bIsInner ? cl_scotres_ring_draw_inner.GetInt() : cl_scotres_ring_draw_outer.GetInt();
		bool bIsTargeting = bIsInner ? m_bTargetingInner : m_bTargetingOuter;
		bool bAlwaysDraw = ( nDrawMode == 1 ) || ( nDrawMode == 2 && bIsTargeting );
		bool bFlashingSuccess = bInFlash && ( m_flDetonateFlashRadius > 0.0f && ( m_bDetonateFlashColor == bIsInner ) );

		if ( bAlwaysDraw || bFlashingFailure || bFlashingSuccess )
		{
			float flAge = gpGlobals->curtime - m_flDetonateFlashTime;
			float flFlashScale = 1.0f - ( flAge / 0.3f );
			if ( flFlashScale < 0.0f ) flFlashScale = 0.0f;
			if ( flFlashScale > 1.0f ) flFlashScale = 1.0f;

			Color clrRing;
			if ( bFlashingFailure )
			{
				// Failure: draw fail color
				clrRing = Color( 
					cl_scotres_ring_fail_r.GetInt(), 
					cl_scotres_ring_fail_g.GetInt(), 
					cl_scotres_ring_fail_b.GetInt(), 
					(int)( flFlashScale * cl_scotres_ring_fail_a.GetInt() ) 
				);
			}
			else if ( bFlashingSuccess )
			{
				// Success flash: flash bright success color
				int nMaxAlpha = bIsInner ? cl_scotres_ring_inner_a.GetInt() : cl_scotres_ring_outer_a.GetInt();
				int nTargetAlpha = (int)( flFlashScale * 255.0f );
				if ( nTargetAlpha < nMaxAlpha && bAlwaysDraw )
				{
					nTargetAlpha = nMaxAlpha;
				}
				else if ( !bAlwaysDraw )
				{
					nTargetAlpha = (int)( flFlashScale * nMaxAlpha );
				}

				clrRing = Color(
					bIsInner ? cl_scotres_ring_inner_r.GetInt() : cl_scotres_ring_outer_r.GetInt(),
					bIsInner ? cl_scotres_ring_inner_g.GetInt() : cl_scotres_ring_outer_g.GetInt(),
					bIsInner ? cl_scotres_ring_inner_b.GetInt() : cl_scotres_ring_outer_b.GetInt(),
					nTargetAlpha
				);
			}
			else
			{
				// Idle persistent draw
				clrRing = Color(
					bIsInner ? cl_scotres_ring_inner_r.GetInt() : cl_scotres_ring_outer_r.GetInt(),
					bIsInner ? cl_scotres_ring_inner_g.GetInt() : cl_scotres_ring_outer_g.GetInt(),
					bIsInner ? cl_scotres_ring_inner_b.GetInt() : cl_scotres_ring_outer_b.GetInt(),
					bIsInner ? cl_scotres_ring_inner_a.GetInt() : cl_scotres_ring_outer_a.GetInt()
				);
			}

			float flRadius = bIsInner ? tf_scotres_inner_radius.GetFloat() : tf_scotres_outer_radius.GetFloat();
			float flPixelRadius = flRadius * ( ScreenHeight() * 0.5f );

			// Draw the circle
			vgui::surface()->DrawSetColor( clrRing );
			int nThickness = MAX( 1, cl_scotres_ring_thickness.GetInt() );
			for ( int i = 0; i < nThickness; ++i )
			{
				vgui::surface()->DrawOutlinedCircle( xCenter, yCenter, (int)flPixelRadius + i, 64 );
			}
		}
	}
}
#endif
