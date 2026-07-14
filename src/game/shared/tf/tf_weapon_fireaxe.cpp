//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_fireaxe.h"

#include "tf_gamerules.h"

//=============================================================================
//
// Weapon FireAxe tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFFireAxe, DT_TFWeaponFireAxe )

BEGIN_NETWORK_TABLE( CTFFireAxe, DT_TFWeaponFireAxe )
#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flKillSpeedBoostTimer ) ),
	RecvPropFloat( RECVINFO( m_flBattery ) ),
#else
	SendPropFloat( SENDINFO( m_flKillSpeedBoostTimer ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flBattery ), 0, SPROP_NOSCALE ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFireAxe )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flKillSpeedBoostTimer, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_fireaxe, CTFFireAxe );
PRECACHE_WEAPON_REGISTER( tf_weapon_fireaxe );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFireAxe::CTFFireAxe()
{
	m_flKillSpeedBoostTimer = -1.0f;
	m_flBattery = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFFireAxe::GetSpeedMod( void )
{
	float flSpeed = 1.0f;

	if ( TFGameRules()->IsBetaActive() )
	{
		float flMoveSpeedAttr = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT( flMoveSpeedAttr, mult_player_movespeed );
		if ( flMoveSpeedAttr == 1.0f )
		{
			return 1.0f;
		}

		// Speed boost (+20% active speed, i.e., 1.2x) is only active while we have battery charge!
		float flTargetSpeed = ( m_flBattery > 0.0f ) ? 1.20f : 1.0f;

		if ( flMoveSpeedAttr != 0.0f )
		{
			flSpeed = flTargetSpeed / flMoveSpeedAttr;
		}
		else
		{
			flSpeed = flTargetSpeed;
		}
	}

	return flSpeed;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFireAxe::AddBatteryCharge( float flCharge )
{
#ifdef GAME_DLL
	float flPrev = m_flBattery;
	m_flBattery = Min( 100.0f, m_flBattery.Get() + flCharge );

	if ( flPrev <= 0.0f && m_flBattery > 0.0f )
	{
		CTFPlayer *pOwner = GetTFPlayerOwner();
		if ( pOwner && pOwner->GetActiveWeapon() == this )
		{
			pOwner->TeamFortress_SetSpeed();
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFireAxe::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

#ifdef GAME_DLL
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( pOwner && TFGameRules()->IsBetaActive() )
	{
		if ( m_flBattery > 0.0f )
		{
			float flPrevBattery = m_flBattery;
			m_flBattery = Max( 0.0f, m_flBattery.Get() - gpGlobals->frametime * 25.0f ); // 25% drain per second (lasts 4 seconds)

			if ( m_flBattery <= 0.0f && flPrevBattery > 0.0f )
			{
				pOwner->TeamFortress_SetSpeed(); // recalculate speed (remove speed boost)
			}
		}
	}
#endif
}

bool CTFFireAxe::IsPowerjack() const
{
	float flMoveSpeedAttr = 1.0f;
	CALL_ATTRIB_HOOK_FLOAT( flMoveSpeedAttr, mult_player_movespeed );
	if ( flMoveSpeedAttr == 1.0f )
	{
		return false;
	}
	return true;
}

bool CTFFireAxe::ShouldUpdateMeter() const
{
	return IsPowerjack() && TFGameRules()->IsBetaActive();
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFFireAxe::GetInitialAfterburnDuration() const 
{ 
	int iAddBurningDamageType = 0;
	CALL_ATTRIB_HOOK_INT( iAddBurningDamageType, set_dmgtype_ignite );
	if ( iAddBurningDamageType )
	{
		return TF_AFTERBURN_BASE_DURATION;
	}

	return BaseClass::GetInitialAfterburnDuration();
}
#endif

