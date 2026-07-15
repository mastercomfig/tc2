//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "soundenvelope.h"
#include "tf_weapon_slap.h"
#include "particle_parse.h"

#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif

#define SLAP_PARTICLE_LEVEL_1	"flaming_slap"
#define SLAP_SOUND_LEVEL_1		"Weapon_Slap.FireSmall"

#define SLAP_PARTICLE_LEVEL_2	"flaming_slap_2"
#define SLAP_SOUND_LEVEL_2		"Weapon_Slap.FireMedium"

#define SLAP_PARTICLE_LEVEL_3	"flaming_slap_3"
#define SLAP_SOUND_LEVEL_3		"Weapon_Slap.FireLarge"

#ifdef CLIENT_DLL
void RecvProxy_UpdateSlapKills( const CRecvProxyData *pData, void *pStruct, void *pOut );
#endif

//=============================================================================
//
// Weapon Slap tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFSlap, DT_TFWeaponSlap )

BEGIN_NETWORK_TABLE( CTFSlap, DT_TFWeaponSlap )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bFirstHit ) ),
	RecvPropInt( RECVINFO( m_nNumKills ), 0, RecvProxy_UpdateSlapKills ),
	RecvPropFloat( RECVINFO( m_flHeat ) ),
#else
	SendPropBool( SENDINFO( m_bFirstHit ) ),
	SendPropInt( SENDINFO( m_nNumKills ) ),
	SendPropFloat( SENDINFO( m_flHeat ), 0, SPROP_NOSCALE ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFSlap )
	DEFINE_FIELD(  m_bFirstHit, FIELD_BOOLEAN ),
END_PREDICTION_DATA()
#endif // CLIENT_DLL

LINK_ENTITY_TO_CLASS( tf_weapon_slap, CTFSlap );
PRECACHE_WEAPON_REGISTER( tf_weapon_slap );

#ifdef CLIENT_DLL
void RecvProxy_UpdateSlapKills( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CTFSlap *pSlap = ( CTFSlap* )pStruct;
	pSlap->SetNumKills( pData->m_Value.m_Int );
	pSlap->UpdateFireEffect();
}
#endif

//=============================================================================
//
// Weapon Slap functions.
//
// -----------------------------------------------------------------------------
CTFSlap::CTFSlap()
{
	m_bFirstHit = true;
	m_nNumKills = 0;
	m_flHeat = 0.0f;
	m_flHeatDecayTimer = 0.0f;

#ifdef CLIENT_DLL
	m_pFlameEffect = NULL;
	m_pFlameEffectSound = NULL;
	m_hEffectOwner = NULL;
#endif // CLIENT_DLL
}

void CTFSlap::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "Weapon_Slap.OpenHand" );
	PrecacheScriptSound( "Weapon_Slap.BackHand" );
	PrecacheScriptSound( "Weapon_Slap.OpenHandHitWorld" );
	PrecacheScriptSound( "Weapon_Slap.BackHandHitWorld" );

	//PrecacheScriptSound( SLAP_SOUND_LEVEL_1 );
	//PrecacheScriptSound( SLAP_SOUND_LEVEL_2 );
	//PrecacheScriptSound( SLAP_SOUND_LEVEL_3 );

	PrecacheParticleSystem( SLAP_PARTICLE_LEVEL_1 );
	PrecacheParticleSystem( SLAP_PARTICLE_LEVEL_2 );
	PrecacheParticleSystem( SLAP_PARTICLE_LEVEL_3 );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFSlap::PrimaryAttack()
{
	if ( !CanAttack() )
	{
		m_flNextPrimaryAttack = MAX(m_flNextPrimaryAttack, gpGlobals->curtime);
		return;
	}

	Slap();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFSlap::SecondaryAttack()
{
	if ( !CanAttack() )
		return;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFSlap::Deploy()
{
	m_nNumKills = 0;
	m_flHeat = 0.0f;
	m_flHeatDecayTimer = 0.0f;

	return BaseClass::Deploy();
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFSlap::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_nNumKills = 0;
	m_flHeat = 0.0f;
	m_flHeatDecayTimer = 0.0f;

#ifdef CLIENT_DLL
	UpdateFireEffect();
#endif // CLIENT_DLL

	return BaseClass::Holster( pSwitchingTo );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFSlap::Smack( void )
{
	BaseClass::Smack();

	if ( m_bFirstHit )
	{
		m_bFirstHit = false;
		// set the 2nd smack time to do 2nd hit without animation
		m_flSmackTime = GetSmackTime( TF_WEAPON_SECONDARY_MODE );
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFSlap::Slap( void )
{
	// Get the current player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// Swing the weapon.
	m_bFirstHit = true;
	Swing( pPlayer );

	m_flNextSecondaryAttack = m_flNextPrimaryAttack;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CTFSlap::PlaySwingSound( void )
{
//	if ( IsCurrentAttackACrit() )
//	{
//		WeaponSound( ( m_nNumKills > 0 ) ? SPECIAL2: BURST );
//	}
//	else
//	{
//		WeaponSound( ( m_nNumKills > 0 ) ? SPECIAL1 : MELEE_MISS );
//	}

	if ( IsCurrentAttackACrit() )
	{
		WeaponSound( BURST );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFSlap::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
}

void CTFSlap::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

#ifdef GAME_DLL
	if ( m_flHeat > 0.0f && gpGlobals->curtime > m_flHeatDecayTimer )
	{
		m_flHeat = Max( 0.0f, m_flHeat.Get() - 0.25f );
		m_flHeatDecayTimer = gpGlobals->curtime + 1.0f;
	}
#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSlap::OnEntityHit( CBaseEntity *pEntity, CTakeDamageInfo *info )
{
	BaseClass::OnEntityHit( pEntity, info );

	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	CTFPlayer *pVictim = ToTFPlayer( pEntity );
	if ( !pVictim || !pVictim->IsAlive() || pVictim->InSameTeam( pOwner ) )
		return;

	if ( IsSuperSlap() )
	{
		// Ignite target for 4 seconds
		pVictim->m_Shared.Burn( pOwner, this, 4.f );

		// Mark target for death for 5 seconds
		pVictim->m_Shared.AddCond( TF_COND_MARKEDFORDEATH, 5.0f );

		// Give owner speed boost for 3 seconds
		pOwner->m_Shared.AddCond( TF_COND_SPEED_BOOST, 3.0f );

		// Apply moderate custom knockback
		Vector vecDir = pVictim->GetAbsOrigin() - pOwner->GetAbsOrigin();
		vecDir.z = 0.0f;
		if ( vecDir.LengthSqr() > 0.0f )
		{
			VectorNormalize( vecDir );
			vecDir.z = 0.3f;
			VectorNormalize( vecDir );
			pVictim->ApplyGenericPushbackImpulse( vecDir * 100.0f, pOwner );
		}

		m_flHeat = 0.0f;
	}
	else
	{
		float flAdd = pVictim->m_Shared.InCond( TF_COND_BURNING ) ? 0.5f : 0.34f;
		m_flHeat = Min( 1.0f, m_flHeat.Get() + flAdd );

		m_flHeatDecayTimer = gpGlobals->curtime + 3.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSlap::OnPlayerKill( CTFPlayer *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::OnPlayerKill( pVictim, info );

	m_nNumKills++;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *CTFSlap::GetShootSound( int iIndex ) const
{
	if ( iIndex == MELEE_HIT )
	{
		return m_bFirstHit ? "Weapon_Slap.OpenHand" : "Weapon_Slap.BackHand";
	}
	else if ( iIndex == MELEE_HIT_WORLD )
	{
		return m_bFirstHit ? "Weapon_Slap.OpenHandHitWorld" : "Weapon_Slap.BackHandHitWorld";
	}

	return BaseClass::GetShootSound( iIndex );
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSlap::UpdateVisibility( void )
{
	BaseClass::UpdateVisibility();
	UpdateFireEffect();
}

void CTFSlap::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	UpdateFireEffect();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSlap::StartFlameEffects( void )
{
	C_TFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	const char *pszParticleEffect = NULL;
	const char *pszSoundEffect = NULL;
	int iHeatLevel = 0;

#if 0
	if ( m_flHeat >= 1.0f )
	{
		pszParticleEffect = SLAP_PARTICLE_LEVEL_3;
		pszSoundEffect = SLAP_SOUND_LEVEL_3;
		iHeatLevel = 3;
	}
	else if ( m_flHeat >= 0.75f )
	{
		pszParticleEffect = SLAP_PARTICLE_LEVEL_2;
		pszSoundEffect = SLAP_SOUND_LEVEL_2;
		iHeatLevel = 2;
	}
	else
	{
		pszParticleEffect = SLAP_PARTICLE_LEVEL_1;
		pszSoundEffect = SLAP_SOUND_LEVEL_1;
		iHeatLevel = 1;
	}
#else
	if ( m_flHeat >= 1.0f )
	{
		pszParticleEffect = SLAP_PARTICLE_LEVEL_2;
		pszSoundEffect = SLAP_SOUND_LEVEL_2;
		iHeatLevel = 2;
	}
#endif

	if ( iHeatLevel == m_iHeatLevel )
		return;

	m_iHeatLevel = iHeatLevel;
	StopFlameEffects();

	if ( m_iHeatLevel < 1 )
		return;

	m_hEffectOwner = NULL;
	bool bIsVM = false;
	if ( ( pOwner == C_TFPlayer::GetLocalTFPlayer() ) && ( ::input->CAM_IsThirdPerson() == false ) )
	{
		m_hEffectOwner = pOwner->GetViewModel();
		bIsVM = true;
	}
	else
	{
		m_hEffectOwner = pOwner;
	}

	if ( m_hEffectOwner )
	{
		int iAttachment = m_hEffectOwner->LookupAttachment( "effect_hand_R" );
		m_pFlameEffect = m_hEffectOwner->ParticleProp()->Create( pszParticleEffect, PATTACH_POINT_FOLLOW, iAttachment, Vector( 0, 0, 0 ) );
		if ( bIsVM )
		{
			m_pFlameEffect->SetIsViewModelEffect( true );
			ClientLeafSystem()->SetRenderGroup( m_pFlameEffect->RenderHandle(), RENDER_GROUP_VIEW_MODEL_TRANSLUCENT );
		}

#if 0
		// Create the looping flame sound
		CLocalPlayerFilter filter;
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pFlameEffectSound = controller.SoundCreate( filter, m_hEffectOwner->entindex(), pszSoundEffect );
		controller.Play( m_pFlameEffectSound, 1.0, 100 );
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSlap::StopFlameEffects( void )
{
	if ( m_pFlameEffect && m_hEffectOwner )
	{
		m_hEffectOwner->ParticleProp()->StopEmission( m_pFlameEffect );
		m_pFlameEffect = NULL;
		m_hEffectOwner = NULL;
	}

	if ( m_pFlameEffectSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pFlameEffectSound );
		m_pFlameEffectSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSlap::UpdateFireEffect( void )
{
	if ( m_flHeat >= 0.5f )
	{
		StartFlameEffects();
	}
	else
	{
		if ( m_iHeatLevel != 0 )
		{
			m_iHeatLevel = 0;
			StopFlameEffects();
		}
	}
}
#endif // CLIENT_DLL
