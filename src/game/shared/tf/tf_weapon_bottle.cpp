//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_bottle.h"
#include "decals.h"
#include "tf_weapon_grenade_pipebomb.h"

#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "prediction.h"
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_fx.h"
#include "tf_gamestats.h"
#endif

//=============================================================================
//
// Weapon Breakable Melee tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBreakableMelee, DT_TFWeaponBreakableMelee )

BEGIN_NETWORK_TABLE( CTFBreakableMelee, DT_TFWeaponBreakableMelee )
#if defined( CLIENT_DLL )
	RecvPropBool( RECVINFO( m_bBroken ), 0, CTFBreakableMelee::RecvProxy_Broken )
#else
	SendPropBool( SENDINFO( m_bBroken ) )
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBreakableMelee )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bBroken, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_INSENDTABLE )
#endif // CLIENT_DLL
END_PREDICTION_DATA()

//=============================================================================
//
// Weapon Bottle tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBottle, DT_TFWeaponBottle )

BEGIN_NETWORK_TABLE( CTFBottle, DT_TFWeaponBottle )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBottle )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bottle, CTFBottle );
PRECACHE_WEAPON_REGISTER( tf_weapon_bottle );

//=============================================================================
//
// Weapon Breakable Sign tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFBreakableSign, DT_TFWeaponBreakableSign )

BEGIN_NETWORK_TABLE( CTFBreakableSign, DT_TFWeaponBreakableSign )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBreakableSign )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_breakable_sign, CTFBreakableSign );
PRECACHE_WEAPON_REGISTER( tf_weapon_breakable_sign );

//=============================================================================
//
// Weapon Stickbomb tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFStickBomb, DT_TFWeaponStickBomb )

BEGIN_NETWORK_TABLE( CTFStickBomb, DT_TFWeaponStickBomb )
#if defined( CLIENT_DLL )
	RecvPropInt( RECVINFO( m_iDetonated ) )
#else
	SendPropInt( SENDINFO( m_iDetonated ), 1, SPROP_UNSIGNED )
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFStickBomb )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_iDetonated, FIELD_INTEGER, FTYPEDESC_INSENDTABLE )
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_stickbomb, CTFStickBomb );
PRECACHE_WEAPON_REGISTER( tf_weapon_stickbomb );

#define TF_WEAPON_STICKBOMB_NORMAL_MODEL	"models/workshop/weapons/c_models/c_caber/c_caber.mdl"
#define TF_WEAPON_STICKBOMB_BROKEN_MODEL	"models/workshop/weapons/c_models/c_caber/c_caber_exploded.mdl"

//=============================================================================

#define TF_BREAKABLE_MELEE_BREAK_BODYGROUP 0
// Absolute body number of broken/not-broken since the server can't figure them out from the studiohdr.  Would only
// matter if we had other body groups going on anyway
#define TF_BREAKABLE_MELEE_BODY_NOTBROKEN 0
#define TF_BREAKABLE_MELEE_BODY_BROKEN 1

//=============================================================================
//
// Weapon Breakable Melee functions.
//

CTFBreakableMelee::CTFBreakableMelee()
{
	m_bBroken = false;
}

void CTFBreakableMelee::WeaponReset( void )
{
	BaseClass::WeaponReset();

	if ( !GetOwner() || !GetOwner()->IsAlive() )
	{
		m_bBroken = false;
	}
}

bool CTFBreakableMelee::DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt )
{
	bool bRet = BaseClass::DefaultDeploy( szViewModel, szWeaponModel, iActivity, szAnimExt );

	if ( bRet )
	{
		SwitchBodyGroups();
	}

	return bRet;
}

void CTFBreakableMelee::SwitchBodyGroups( void )
{
	int iState = 0;

	if ( m_bBroken == true )
	{
		iState = 1;
	}

#ifdef CLIENT_DLL
	// We'll successfully predict m_nBody along with m_bBroken, but this can be called outside prediction, in which case
	// we want to use the networked m_nBody value -- but still fixup our viewmodel which is clientside only.
	if ( prediction->InPrediction() )
		{ SetBodygroup( TF_BREAKABLE_MELEE_BREAK_BODYGROUP, iState ); }

	CTFPlayer *pTFPlayer = ToTFPlayer( GetOwner() );
	if ( pTFPlayer && pTFPlayer->GetActiveWeapon() == this )
	{
		C_BaseAnimating *pViewWpn = GetAppropriateWorldOrViewModel();
		if ( pViewWpn != this )
		{
			pViewWpn->SetBodygroup( TF_BREAKABLE_MELEE_BREAK_BODYGROUP, iState );
		}
	}
#else // CLIENT_DLL
	m_nBody = iState ? TF_BREAKABLE_MELEE_BODY_BROKEN : TF_BREAKABLE_MELEE_BODY_NOTBROKEN;
#endif // CLIENT_DLL
}

bool CTFBreakableMelee::UpdateBodygroups( CBaseCombatCharacter* pOwner, int iState )
{
	SwitchBodyGroups();

	return BaseClass::UpdateBodygroups( pOwner, iState );
}

void CTFBreakableMelee::Smack( void )
{
	BaseClass::Smack();

	if ( ConnectedHit() && IsCurrentAttackACrit() )
	{
		SetBroken( true );
	}
}

void CTFBreakableMelee::SetBroken( bool bBroken )
{ 
	m_bBroken = bBroken;
	SwitchBodyGroups();
}

#ifdef CLIENT_DLL
/* static */ void CTFBreakableMelee::RecvProxy_Broken( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TFBreakableMelee* pWeapon = ( C_TFBreakableMelee*) pStruct;

	if ( !!pData->m_Value.m_Int != pWeapon->m_bBroken )
	{
		pWeapon->m_bBroken = !!pData->m_Value.m_Int;
		pWeapon->SwitchBodyGroups();
	}
}
#endif // CLIENT_DLL

CTFStickBomb::CTFStickBomb()
: CTFBreakableMelee()
{
	m_iDetonated = 0;
}

void CTFStickBomb::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( TF_WEAPON_STICKBOMB_NORMAL_MODEL );
	PrecacheModel( TF_WEAPON_STICKBOMB_BROKEN_MODEL );
}

void CTFStickBomb::PrimaryAttack()
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
	{
		m_flNextPrimaryAttack = MAX(m_flNextPrimaryAttack, gpGlobals->curtime);
		return;
	}

	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_bConnected = false;

	// Note: For beta caber, we do NOT call pPlayer->EndClassSpecialSkill() here!
	// It will be deferred to Smack() if we actually hit an enemy.
	if ( !TFGameRules() || !TFGameRules()->IsBetaActive() )
	{
		pPlayer->EndClassSpecialSkill();
	}

	// Swing the weapon.
	Swing( pPlayer );

	m_bCurrentAttackIsDuringDemoCharge = pPlayer->m_Shared.GetNextMeleeCrit() != MELEE_NOCRIT;

	if ( pPlayer->m_Shared.GetNextMeleeCrit() == MELEE_MINICRIT )
	{
		m_bMiniCrit = true;
	}
	else
	{
		m_bMiniCrit = false;
	}

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );

	if ( pPlayer->m_Shared.IsStealthed() && ShouldRemoveInvisibilityOnPrimaryAttack() )
	{
		pPlayer->RemoveInvisibility();
	}
#endif

	pPlayer->m_Shared.OnAttack();
}

void CTFStickBomb::Smack( void )
{
	CTFWeaponBaseMelee::Smack();

	// Stick bombs detonate once, on impact.
	if ( m_iDetonated == 0 )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( GetOwner() );
		if ( !pTFPlayer )
			return;

		bool bHitEnemy = false;
		bool bIsBeta = TFGameRules() && TFGameRules()->IsBetaActive();
		bool bConnectedHit = ConnectedHit();

		if ( !bConnectedHit && !bIsBeta )
			return;

		trace_t trace;
		DoSwingTrace( trace );
		Vector explosion = trace.endpos;

		bool bDirectHitEnemy = false;
		if ( trace.m_pEnt && trace.m_pEnt->IsAlive() && trace.m_pEnt != pTFPlayer )
		{
			if ( trace.m_pEnt->GetTeamNumber() != pTFPlayer->GetTeamNumber() && trace.m_pEnt->GetTeamNumber() >= TF_TEAM_RED )
			{
#ifdef GAME_DLL
				if ( trace.m_pEnt->m_takedamage != DAMAGE_NO )
				{
					bDirectHitEnemy = true;
				}
#else
				bDirectHitEnemy = true;
#endif
			}
		}

		if ( bDirectHitEnemy )
		{
			explosion = pTFPlayer->WorldSpaceCenter();
		}

		float flRadius = bIsBeta ? 146.0f : 100.0f;
		CALL_ATTRIB_HOOK_FLOAT( flRadius, mult_explosion_radius );

		CBaseEntity *pEntity = NULL;
		for ( CEntitySphereQuery sphere( explosion, flRadius ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
		{
			if ( !pEntity || !pEntity->IsAlive() || pEntity == pTFPlayer )
				continue;
#ifdef GAME_DLL
			if ( pEntity->m_takedamage == DAMAGE_NO )
				continue;
#endif
			if ( pEntity->GetTeamNumber() != pTFPlayer->GetTeamNumber() && pEntity->GetTeamNumber() >= TF_TEAM_RED )
			{
				Vector vecPos;
				pEntity->CollisionProp()->CalcNearestPoint( explosion, &vecPos );
				if ( (explosion - vecPos).LengthSqr() <= flRadius * flRadius )
				{
					bHitEnemy = true;
					break;
				}
			}
		}

		if ( bHitEnemy || !bIsBeta )
		{
			m_iDetonated = 1;
			m_bBroken = true;
			SwitchBodyGroups();
			// End charge here if beta active (since we skipped it in PrimaryAttack)
			if ( bIsBeta )
			{
				pTFPlayer->EndClassSpecialSkill();
			}
		}

#ifdef GAME_DLL
		{
			Vector vecForward; 
			AngleVectors( pTFPlayer->EyeAngles(), &vecForward );
			Vector vecSwingStart = pTFPlayer->WorldSpaceCenter();

			CPVSFilter filter( explosion );
			
			// Halloween Spell
			int iHalloweenSpell = 0;
			int iCustomParticleIndex = INVALID_STRING_INDEX;
			if ( TF_IsHolidayActive( kHoliday_HalloweenOrFullMoon ) )
			{
				CALL_ATTRIB_HOOK_INT_ON_OTHER( this, iHalloweenSpell, halloween_pumpkin_explosions );
				if ( iHalloweenSpell > 0 )
				{
					iCustomParticleIndex = GetParticleSystemIndex( "halloween_explosion" );
				}
			}

			TE_TFExplosion( filter, 0.0f, explosion, Vector(0,0,1), TF_WEAPON_GRENADELAUNCHER, pTFPlayer->entindex(), -1, SPECIAL1, iCustomParticleIndex );

			// TODO(mcoms): use DMG_MELEE? (Fixed the Ullapool Caber's explosion not being counted as melee damage (for kill_refills_meter))
			int dmgType = bIsBeta ? (DMG_BLAST | DMG_PREVENT_PHYSICS_FORCE | DMG_HALF_FALLOFF) : DMG_BLAST | DMG_HALF_FALLOFF;
			const bool bIsCrit = IsCurrentAttackACrit();
			if (bIsCrit)
				dmgType |= DMG_CRITICAL;

			float flDamage = 75.0f;
			CALL_ATTRIB_HOOK_FLOAT( flDamage, mult_dmg );
			if ( !bIsCrit && m_bMiniCrit )
			{
				flDamage *= 1.35f;
			}

			CTakeDamageInfo info( pTFPlayer, pTFPlayer, this, vec3_origin, explosion, flDamage, dmgType, TF_DMG_CUSTOM_STICKBOMB_EXPLOSION, &explosion );

			CTFRadiusDamageInfo radiusinfo( &info, explosion, flRadius, bIsBeta ? pTFPlayer : NULL );

			TFGameRules()->RadiusDamage( radiusinfo );

			if ( bIsBeta )
			{
				// Always handle self damage manually for beta caber so we can restore physics force to the user
				Vector vecPos;
				pTFPlayer->CollisionProp()->CalcNearestPoint( explosion, &vecPos );
				float flDist = (explosion - vecPos).Length();
				float flDamageToSelf = info.GetDamage();
				float flFalloff = 0.5f;

				if ( flDist > 0 && flRadius > 0 )
				{
					flDamageToSelf -= flDamageToSelf * flFalloff * (flDist / flRadius);
				}

				if ( flDamageToSelf > 0 )
				{
					if ( bHitEnemy )
					{
						flDamageToSelf *= 25.0f;
					}
					else
					{
						flDamageToSelf *= 0.75f;
					}
					int selfDmgType = (dmgType & ~(DMG_CRITICAL)) & ~(DMG_PREVENT_PHYSICS_FORCE);
					CTakeDamageInfo selfInfo( pTFPlayer, pTFPlayer, this, flDamageToSelf, selfDmgType, TF_DMG_CUSTOM_STICKBOMB_EXPLOSION );
					selfInfo.SetDamagePosition( explosion );
					pTFPlayer->TakeDamage( selfInfo );
				}

				if ( bHitEnemy )
				{
					// at position
					Vector vel1 = Vector(RandomFloat(-10, 10), RandomFloat(-10, 10), 100);
					float timer1 = RandomFloat(0.6f, 0.8f);
					CreateGrenade(pTFPlayer, vecSwingStart, vel1, timer1, 0.25f, bIsCrit);
					// at swing direction
					Vector vel2 = Vector(RandomFloat(-10, 10), RandomFloat(-10, 10), 100);
					vel2 += vecForward * 50.0f;
					float timer2 = RandomFloat(0.6f, 0.8f);
					CreateGrenade(pTFPlayer, vecSwingStart, vel2, timer2, 0.25f, bIsCrit);
					// at velocity
					Vector vel3 = Vector(RandomFloat(-10, 10), RandomFloat(-10, 10), 100);
					vel3 += pTFPlayer->GetAbsVelocity();
					float timer3 = RandomFloat(0.6f, 0.8f);
					CreateGrenade(pTFPlayer, vecSwingStart, vel3, timer3, 0.25f, bIsCrit);
					// random
					Vector vel4 = Vector(RandomFloat(-200, 200), RandomFloat(-200, 200), 100);
					float timer4 = RandomFloat(0.6f, 0.8f);
					CreateGrenade(pTFPlayer, vecSwingStart, vel4, timer4, 0.25f, bIsCrit);
				}
			}

			if ( !pTFPlayer->IsAlive() && bIsBeta )
			{
				float flNormalDuration = TFGameRules()->GetNextRespawnWave( pTFPlayer->GetTeamNumber(), pTFPlayer ) - gpGlobals->curtime;
				float flNewDuration = MAX( 2.0f, flNormalDuration - 5.0f );
				pTFPlayer->SetRespawnOverride( flNewDuration, NULL_STRING );
			}
		}
#endif
	}
}

#ifdef GAME_DLL
void CTFStickBomb::CreateGrenade(CTFPlayer* pPlayer, const Vector& pos, const Vector& vel, float flTimer, float flDmgMult, bool bIsCrit)
{
	Vector angImpulse = AngularImpulse(600, random->RandomInt(-1200, 1200), 0);
	CTFGrenadePipebombProjectile* pProjectile = CTFGrenadePipebombProjectile::Create(pos, QAngle(180, 0, 0), vel, angImpulse, pPlayer, GetTFWpnData(), -1, flDmgMult);
	if (pProjectile)
	{
		pProjectile->SetLauncher(this);
		pProjectile->SetCritical(bIsCrit);
		if (!bIsCrit && m_bMiniCrit)
		{
			pProjectile->IncrementDeflected(); // hack for minicrits
		}
		pProjectile->SetModel(BaseClass::GetWorldModel());
		pProjectile->SetDetonateTimerLength(flTimer);
		UTIL_SetSize(pProjectile, TF_GRENADE_PROJECTILE_MINS, TF_GRENADE_PROJECTILE_MAXS);
	}
}
#endif

void CTFStickBomb::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iDetonated = 0;

	SwitchBodyGroups();
}

void CTFStickBomb::WeaponRegenerate( void )
{
	BaseClass::WeaponRegenerate();

	m_iDetonated = 0;

	SetContextThink( &CTFStickBomb::SwitchBodyGroups, gpGlobals->curtime + 0.01f, "SwitchBodyGroups" );
}

void CTFStickBomb::SwitchBodyGroups( void )
{
#ifdef CLIENT_DLL
	if ( !GetViewmodelAttachment() )
		return;

	if ( m_iDetonated == 1 )
	{
		GetViewmodelAttachment()->SetModel( TF_WEAPON_STICKBOMB_BROKEN_MODEL );
	}
	else
	{
		GetViewmodelAttachment()->SetModel( TF_WEAPON_STICKBOMB_NORMAL_MODEL );
	}
#endif
}

const char *CTFStickBomb::GetWorldModel( void ) const
{
	if ( m_iDetonated == 1 )
	{
		return TF_WEAPON_STICKBOMB_BROKEN_MODEL;
	}
	else
	{
		return BaseClass::GetWorldModel();
	}
}

#ifdef CLIENT_DLL

int CTFStickBomb::GetWorldModelIndex( void )
{
	if ( !modelinfo )
		return BaseClass::GetWorldModelIndex();

	if ( m_iDetonated == 1 )
	{
		m_iWorldModelIndex = modelinfo->GetModelIndex( TF_WEAPON_STICKBOMB_BROKEN_MODEL );
		return m_iWorldModelIndex;
	}
	else
	{
		m_iWorldModelIndex = modelinfo->GetModelIndex( TF_WEAPON_STICKBOMB_NORMAL_MODEL );
		return m_iWorldModelIndex;
	}
}

void CTFStickBomb::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	
	SwitchBodyGroups();
}

#endif
