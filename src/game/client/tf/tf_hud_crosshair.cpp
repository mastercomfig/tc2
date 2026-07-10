//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "clientmode.h"
#include "c_tf_player.h"
#include "tf_hud_crosshair.h"
#include "hud_crosshair.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "tf_logic_halloween_2014.h"
#include "tf_gamerules.h"
#include "tf_weapon_invis.h"
#include "mathlib/mathlib.h"

ConVar cl_crosshair_red( "cl_crosshair_red", "200", FCVAR_ARCHIVE );
ConVar cl_crosshair_green( "cl_crosshair_green", "200", FCVAR_ARCHIVE );
ConVar cl_crosshair_blue( "cl_crosshair_blue", "200", FCVAR_ARCHIVE );
ConVar cl_crosshairalpha( "cl_crosshairalpha", "255", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );

ConVar cl_crosshair_file( "cl_crosshair_file", "", FCVAR_ARCHIVE );
ConVar cl_hitmarker_file( "cl_hitmarker_file", "crosshair4", FCVAR_ARCHIVE );

ConVar cl_crosshair_scale( "cl_crosshair_scale", "32.0", FCVAR_ARCHIVE );

ConVar cl_crosshair_gap( "cl_crosshair_gap", "4", FCVAR_ARCHIVE );

ConVar cl_hitmarker( "cl_hitmarker", "1", FCVAR_ARCHIVE );

using namespace vgui;

// Everything else is expecting to find "CHudCrosshair"
DECLARE_NAMED_HUDELEMENT( CHudTFCrosshair, CHudCrosshair );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTFCrosshair::CHudTFCrosshair( const char *pName ) :
	CHudCrosshair ( pName )
{
	m_flTimeToHideUntil = -1.f;
	m_iDamaged = 0;
	m_flDamageOffTime = 0.0f;

	ListenForGameEvent( "restart_timer_time" );
	ListenForGameEvent("player_hurt");
	ListenForGameEvent("npc_hurt");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudTFCrosshair::ShouldDraw( void )
{
	// turn off for the minigames
	if ( CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() )
		return false;

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		return false;

	// turn off if the local player is a ghost
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
			return false;

		if ( pPlayer->IsTaunting() )
			return false;
	}

	if ( m_flTimeToHideUntil > gpGlobals->curtime )
		return false;

	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTFCrosshair::LevelShutdown( void )
{
	m_Crosshair.Shutdown();
	m_DmgCrosshair.Shutdown();
	m_StickbombViewCrosshair.Shutdown();

	m_flDamageOffTime = 0.0f;
	m_flTimeToHideUntil = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTFCrosshair::Init()
{
	m_Crosshair.Init();
	m_DmgCrosshair.Init();
	m_StickbombViewCrosshair.Init();

	m_flDamageOffTime = 0.0f;

	m_flTimeToHideUntil = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTFCrosshair::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( "restart_timer_time", event->GetName() ) )
	{
		if ( TFGameRules() && ( TFGameRules()->IsCompetitiveMode() || TFGameRules()->IsEmulatingMatch() ) )
		{
			int nTime = event->GetInt( "time" );
			if ( ( nTime <= 10 ) && ( nTime > 0 ) )
			{
				m_flTimeToHideUntil = gpGlobals->curtime + nTime;
				return;
			}
		}
	}
	else if ( FStrEq(event->GetName(), "player_hurt") )
	{
		const int iDamage = event->GetInt("damageamount");
		const int iHealth = event->GetInt("health");

		const int iAttacker = engine->GetPlayerForUserID(event->GetInt("attacker"));
		C_TFPlayer* pAttacker = ToTFPlayer(UTIL_PlayerByIndex(iAttacker));

		const int iVictim = engine->GetPlayerForUserID(event->GetInt("userid"));
		C_TFPlayer* pVictim = ToTFPlayer(UTIL_PlayerByIndex(iVictim));

		HandleDamageEvent(pAttacker, pVictim, iDamage, iHealth);
	}
	else if ( FStrEq(event->GetName(), "npc_hurt") )
	{
		const int iDamage = event->GetInt("damageamount");
		const int iHealth = event->GetInt("health");

		const int iAttacker = engine->GetPlayerForUserID(event->GetInt("attacker_player"));
		C_TFPlayer* pAttacker = ToTFPlayer(UTIL_PlayerByIndex(iAttacker));

		C_BaseCombatCharacter* pVictim = (C_BaseCombatCharacter*)ClientEntityList().GetClientEntity(event->GetInt("entindex"));

		HandleDamageEvent(pAttacker, pVictim, iDamage, iHealth);
	}

	m_flTimeToHideUntil = -1.f;
}

void CHudTFCrosshair::HandleDamageEvent(C_TFPlayer* pAttacker, C_BaseCombatCharacter* pVictim,
	int iDamage, int iHealth)
{
	if (iDamage <= 0) // zero value (invuln?)
		return;

	CTFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pLocalPlayer)
		return;

	if (!pAttacker || !pVictim)
		return;

	if ((pAttacker == pLocalPlayer) ||
		(pLocalPlayer->IsPlayerClass(TF_CLASS_MEDIC) && (pLocalPlayer->MedicGetHealTarget() == pAttacker)))
	{
		bool bDeadRingerSpy = false;
		C_TFPlayer* pVictimPlayer = ToTFPlayer(pVictim);
		if (pVictimPlayer)
		{
			// Player hurt self
			if (pAttacker == pVictimPlayer)
				return;

			// Don't show damage on stealthed and/or disguised enemy spies
			if (pVictimPlayer->IsPlayerClass(TF_CLASS_SPY) && pVictimPlayer->GetTeamNumber() != pLocalPlayer->GetTeamNumber())
			{
				CTFWeaponInvis* pWpn = (CTFWeaponInvis*)pVictimPlayer->Weapon_OwnsThisID(TF_WEAPON_INVIS);
				if (pWpn && pWpn->HasFeignDeath())
				{
					if (pVictimPlayer->m_Shared.IsFeignDeathReady())
					{
						bDeadRingerSpy = true;
					}
				}

				if (!bDeadRingerSpy)
				{
					if (pVictimPlayer->m_Shared.GetDisguiseTeam() == pLocalPlayer->GetTeamNumber() || pVictimPlayer->m_Shared.IsStealthed())
						return;
				}
			}
		}

		const bool bLastHit = ( iHealth <= 0 ) || bDeadRingerSpy;
		m_iDamaged = iDamage;
		m_bKill = bLastHit;
		const float flNewDmgTime = gpGlobals->curtime + ( bLastHit ? 0.2f : 0.1f );
		if ( flNewDmgTime > m_flDamageOffTime )
		{
			m_flDamageOffTime = flNewDmgTime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTFCrosshair::Paint()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	m_Crosshair.Update( cl_crosshair_file.GetString() );
	m_DmgCrosshair.Update( cl_hitmarker_file.GetString() );
	// TODO(mcoms): stickybombcrosshair
	//m_StickbombViewCrosshair.Update( "stickybombview" );

	// This is somewhat cut'n'paste from CHudCrosshair::Paint(). Would be nice to unify them some more.
	float x, y;
	bool bBehindCamera;
	GetDrawPosition( &x, &y, &bBehindCamera );

	if ( bBehindCamera )
		return;

	float flWeaponScale = 1.f;
	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->GetWeaponCrosshairScale( flWeaponScale );
	}

	float flPlayerScale = cl_crosshair_scale.GetFloat() / 32.0f;  // the player can change the scale in the options/multiplayer tab
	Color clr( cl_crosshair_red.GetInt(), cl_crosshair_green.GetInt(), cl_crosshair_blue.GetInt(), cl_crosshairalpha.GetInt() );

	const float flCrosshairScale = flWeaponScale * flPlayerScale;

	if ( cl_hitmarker.GetBool() && m_iDamaged > 0 )
	{
		const float flShowTime = m_bKill ? 0.2f : 0.1f;
		const float flDamageStartTime = m_flDamageOffTime - flShowTime;
		const int nAlpha = RoundFloatToNearestInt( RemapValClamped( m_flDamageOffTime - gpGlobals->curtime, 0.0f, 0.05f, 255.0f, 60.0f ) );
		Color dmgClr( 255, 40, 20, nAlpha );
		float flScaleFactor = m_bKill ? 1.5f : 1.0f;
		float flDmgLerp = RemapValClamped( m_iDamaged, 10.0f, 150.0f, 0.0f, 0.5f );
		if ( m_bKill )
		{
			// TODO(mcoms): overkill
			flDmgLerp += RemapValClamped( m_iDamaged / 3.0f, 10.0f, 150.0f, 0.0f, 0.5f );
		}
		flScaleFactor += flDmgLerp;
		
		const float flHitmarkerScale = flCrosshairScale * flScaleFactor * RemapValClamped( gpGlobals->curtime - flDamageStartTime, 0.0f, 0.05f, 0.75f, 1.0f );

		m_DmgCrosshair.Draw( x, y, flHitmarkerScale, dmgClr );

		if ( m_flDamageOffTime <= gpGlobals->curtime )
		{
			m_iDamaged = 0;
		}
	}

	if ( !m_Crosshair.HasCrosshair() )
	{
		return BaseClass::Paint();
	}

	m_Crosshair.Draw( x, y, flCrosshairScale, clr, cl_crosshair_gap.GetInt() );
}

CCrosshairElement::CCrosshairElement()
{
	m_iTextureID = -1;
	m_szTextureName[0] = '\0';
	m_pMaterial = NULL;
	m_pFrameVar = NULL;
	m_nNumFrames = 0;
	m_nFrame = -1;
}

CCrosshairElement::~CCrosshairElement()
{
	if ( vgui::surface() && m_iTextureID != -1 )
	{
		vgui::surface()->DestroyTextureID( m_iTextureID );
		m_iTextureID = -1;
	}
}

void CCrosshairElement::Init()
{
	if ( m_iTextureID == -1 )
	{
		m_iTextureID = vgui::surface()->CreateNewTextureID();
	}
}

void CCrosshairElement::Shutdown()
{
	m_szTextureName[0] = '\0';
	
	if ( m_pMaterial )
	{
		delete m_pMaterial;
		m_pMaterial = NULL;
	}

	if ( m_pFrameVar )
	{
		delete m_pFrameVar;
		m_pFrameVar = NULL;
	}
}

void CCrosshairElement::Update( const char* szTextureName )
{
	if ( FStrEq( m_szTextureName, szTextureName ) )
	{
		return;
	}

	if ( m_szTextureName[0] == '\0' && szTextureName == NULL )
	{
		return;
	}

	if ( szTextureName )
	{
		Q_strncpy( m_szTextureName, szTextureName, sizeof(m_szTextureName) );
	}
	else
	{
		m_szTextureName[0] = '\0';
		if ( m_pMaterial )
		{
			delete m_pMaterial;
			m_pMaterial = NULL;
		}
		return;
	}

	char buf[256];
	Q_snprintf( buf, sizeof(buf), "vgui/crosshairs/%s", szTextureName );

	if ( m_iTextureID != -1 )
	{
		vgui::surface()->DrawSetTextureFile( m_iTextureID, buf, true, false );
	}

	if ( m_pMaterial )
	{
		delete m_pMaterial;
	}

	m_pMaterial = vgui::surface()->DrawGetTextureMatInfoFactory( m_iTextureID );

	if ( !m_pMaterial )
		return;

	m_pFrameVar = m_pMaterial->FindVarFactory( "$frame", NULL );
	if ( m_pFrameVar )
	{
		m_nNumFrames = m_pMaterial->GetNumAnimationFrames() - 1;
		m_nFrame = -1;
	}
}

void CCrosshairElement::Draw( int x, int y, float flScale, Color color, int iGap )
{
	if ( !m_pMaterial )
	{
		return;
	}

	if ( m_pFrameVar )
	{
		int nFrame = clamp( iGap, 0, m_nNumFrames );
		if ( nFrame != m_nFrame )
		{
			m_nFrame = nFrame;
			m_pFrameVar->SetIntValue( nFrame );
		}
	}

	if ( flScale < 1 / 64.0f )
		return;

	if ( color.a() == 0)
		return;
	
	int iTextureW = 32;
	int iTextureH = 32;
	float flWidth = flScale * (float)iTextureW;
	float flHeight = flScale * (float)iTextureH;
	int iWidth = RoundFloatToNearestInt( flWidth );
	int iHeight = RoundFloatToNearestInt( flHeight );
	int iX = RoundFloatToNearestInt( x );
	int iY = RoundFloatToNearestInt( y );

	vgui::ISurface* pSurf = vgui::surface();

	pSurf->DrawSetColor( color );
	pSurf->DrawSetTexture( m_iTextureID );
	pSurf->DrawTexturedRect( iX - iWidth, iY - iHeight, iX + iWidth, iY + iHeight );
	pSurf->DrawSetTexture(0);
}
