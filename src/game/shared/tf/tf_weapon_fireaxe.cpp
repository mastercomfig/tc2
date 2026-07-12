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
#else
	SendPropFloat( SENDINFO( m_flKillSpeedBoostTimer ), 0, SPROP_NOSCALE ),
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

		float flTargetSpeed = flMoveSpeedAttr - 0.05f;
		if ( m_flKillSpeedBoostTimer >= 0.0f && m_flKillSpeedBoostTimer < gpGlobals->curtime )
		{
			flTargetSpeed += 0.15f;
		}

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

