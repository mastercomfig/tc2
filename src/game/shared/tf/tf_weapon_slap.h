//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SLAP_H
#define TF_WEAPON_SLAP_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFSlap C_TFSlap
#endif

//=============================================================================
//
// Fists weapon class.
//
class CTFSlap : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFSlap, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFSlap();

	virtual int GetWeaponID( void ) const OVERRIDE { return TF_WEAPON_SLAP; }
	virtual void Precache() OVERRIDE;
	virtual void PrimaryAttack() OVERRIDE;
	virtual void SecondaryAttack() OVERRIDE;
	virtual bool Deploy() OVERRIDE;
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo ) OVERRIDE;
	virtual void Smack( void ) OVERRIDE;
	virtual void SendPlayerAnimEvent( CTFPlayer *pPlayer ) OVERRIDE;
	virtual bool HideWhileStunned( void ) OVERRIDE { return false; }
	virtual void PlaySwingSound( void ) OVERRIDE;
	virtual char const *GetShootSound( int iIndex ) const OVERRIDE;

	void Slap( void );
	int GetNumKills( void ) { return m_nNumKills; }
	bool IsSuperSlap( void ) const { return m_flHeat >= 1.0f; }

	virtual float GetProgress() { return m_flHeat; }
	virtual bool ShouldUpdateMeter() const { return false; }
	virtual const char *GetEffectLabelText( void ) { return "#TF_HEAT"; }

	virtual void ItemPostFrame( void ) OVERRIDE;

#ifdef GAME_DLL
	virtual void OnEntityHit( CBaseEntity *pEntity, CTakeDamageInfo *info ) OVERRIDE;
	virtual void OnPlayerKill( CTFPlayer *pVictim, const CTakeDamageInfo &info ) OVERRIDE;
#else
	virtual void OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual void UpdateVisibility( void ) OVERRIDE;
	void SetNumKills( int nNumKills ) { m_nNumKills = nNumKills; }
	void UpdateFireEffect( void );
#endif // GAME_DLL

private:
	CNetworkVar( bool, m_bFirstHit );
	CNetworkVar( int, m_nNumKills );
	CNetworkVar( float, m_flHeat );

	float m_flHeatDecayTimer;

	int m_iHeatLevel = 0;

#ifdef CLIENT_DLL
	void StartFlameEffects( void );
	void StopFlameEffects( void );
	CNewParticleEffect *m_pFlameEffect;
	CSoundPatch *m_pFlameEffectSound;
	EHANDLE m_hEffectOwner;
#endif // CLIENT_DLL
};

#endif // TF_WEAPON_SLAP_H
