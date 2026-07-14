//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_FIREAXE_H
#define TF_WEAPON_FIREAXE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFFireAxe C_TFFireAxe
#endif

//=============================================================================
//
// BrandingIron class.
//
class CTFFireAxe : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFFireAxe, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFFireAxe();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_FIREAXE; }

	virtual float		GetSpeedMod( void ) OVERRIDE;
	virtual void		ItemPostFrame( void ) OVERRIDE;

	bool				IsPowerjack( void ) const;

	float		GetProgress() { return m_flBattery / 100.0f; }
	bool        ShouldUpdateMeter() const;
	const char	*GetEffectLabelText( void ) { return "#TF_BATTERY"; }
	bool        EffectMeterShouldFlash( void ) { return false; }
	void				AddBatteryCharge( float flCharge );

#ifdef GAME_DLL
	virtual float GetInitialAfterburnDuration() const OVERRIDE;
	void SetKillSpeedBoostTimer( float flTime ) { m_flKillSpeedBoostTimer = flTime; }
#endif
	float GetKillSpeedBoostTimer( void ) const { return m_flKillSpeedBoostTimer; }

private:

	CTFFireAxe( const CTFFireAxe & ) {}

	CNetworkVar( float, m_flKillSpeedBoostTimer );
	CNetworkVar( float, m_flBattery );
};

#endif // TF_WEAPON_FIREAXE_H
