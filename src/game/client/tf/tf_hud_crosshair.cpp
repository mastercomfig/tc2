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

ConVar cl_crosshair_file( "cl_crosshair_file", "", FCVAR_ARCHIVE );

ConVar cl_crosshair_scale( "cl_crosshair_scale", "32.0", FCVAR_ARCHIVE );

ConVar cl_crosshair_gap( "cl_crosshair_gap", "0", FCVAR_ARCHIVE );

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
	m_szPreviousCrosshair[0] = '\0';
	m_iCrosshairTextureID = -1;
	m_flTimeToHideUntil = -1.f;
	m_pFrameVar = NULL;
	m_nPrevFrame = -1;
	m_iDmgCrosshairTextureID = -1;
	m_iDamaged = 0;
	m_flDamageOffTime = 0.0f;
	m_pDmgCrosshairMaterial = NULL;

	ListenForGameEvent( "restart_timer_time" );
	ListenForGameEvent("player_hurt");
	ListenForGameEvent("npc_hurt");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTFCrosshair::~CHudTFCrosshair( void )
{
	if ( vgui::surface() && m_iCrosshairTextureID != -1 )
	{
		vgui::surface()->DestroyTextureID( m_iCrosshairTextureID );
		m_iCrosshairTextureID = -1;
	}
	if (vgui::surface() && m_iDmgCrosshairTextureID != -1)
	{
		vgui::surface()->DestroyTextureID(m_iDmgCrosshairTextureID);
		m_iDmgCrosshairTextureID = -1;
	}
	if ( m_pFrameVar )
	{
		delete m_pFrameVar;
		m_pFrameVar = NULL;
	}
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
	m_szPreviousCrosshair[0] = '\0';

	if ( m_pCrosshairMaterial )
	{
		delete m_pCrosshairMaterial;
		m_pCrosshairMaterial = NULL;
	}

	if (m_pDmgCrosshairMaterial)
	{
		delete m_pDmgCrosshairMaterial;
		m_pDmgCrosshairMaterial = NULL;
	}

	m_flDamageOffTime = 0.0f;
	
	m_flTimeToHideUntil = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTFCrosshair::Init()
{
	if ( m_iCrosshairTextureID == -1 )
	{
		m_iCrosshairTextureID = vgui::surface()->CreateNewTextureID();
	}

	if (m_iDmgCrosshairTextureID == -1)
	{
		m_iDmgCrosshairTextureID = vgui::surface()->CreateNewTextureID();
	}

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

		bool bLastHit = (iHealth <= 0) || bDeadRingerSpy;
		m_iDamaged = iDamage;
		m_bKill = bLastHit;
		float flNewDmgTime = gpGlobals->curtime + (bLastHit ? 0.2f : 0.1f);
		if (flNewDmgTime > m_flDamageOffTime)
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

	if( !pPlayer )
		return;

	if ( !m_pDmgCrosshairMaterial )
	{
		if (m_iDmgCrosshairTextureID != -1)
		{
			vgui::surface()->DrawSetTextureFile( m_iDmgCrosshairTextureID, "vgui/crosshairs/crosshair4", true, false);
		}

		m_pDmgCrosshairMaterial = vgui::surface()->DrawGetTextureMatInfoFactory( m_iDmgCrosshairTextureID );

		if ( !m_pDmgCrosshairMaterial )
			return;
	}

	const char *crosshairfile = cl_crosshair_file.GetString();
	if ( ( crosshairfile == NULL ) || ( Q_stricmp( m_szPreviousCrosshair, crosshairfile ) != 0 ) )
	{
		char buf[256];
		Q_snprintf( buf, sizeof(buf), "vgui/crosshairs/%s", crosshairfile );

		if ( m_iCrosshairTextureID != -1 )
		{
			vgui::surface()->DrawSetTextureFile( m_iCrosshairTextureID, buf, true, false );
		}

		if ( m_pCrosshairMaterial )
		{
			delete m_pCrosshairMaterial;
		}

		m_pCrosshairMaterial = vgui::surface()->DrawGetTextureMatInfoFactory( m_iCrosshairTextureID );

		if (!m_pCrosshairMaterial)
			return;

		// save the name to compare with the cvar in the future
		Q_strncpy( m_szPreviousCrosshair, crosshairfile, sizeof(m_szPreviousCrosshair) );

		m_pFrameVar = m_pCrosshairMaterial->FindVarFactory( "$frame", NULL );
		if ( m_pFrameVar )
		{
			m_nNumFrames = m_pCrosshairMaterial->GetNumAnimationFrames() - 1;
			m_nPrevFrame = -1;
		}
	}

	// This is somewhat cut'n'paste from CHudCrosshair::Paint(). Would be nice to unify them some more.
	float x, y;
	bool bBehindCamera;
	GetDrawPosition ( &x, &y, &bBehindCamera );

	if( bBehindCamera )
		return;

	float flWeaponScale = 1.f;
	int iTextureW = 32;
	int iTextureH = 32;
	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->GetWeaponCrosshairScale( flWeaponScale );
	}

	float flPlayerScale = 1.0f;
#ifdef TF_CLIENT_DLL
	Color clr( cl_crosshair_red.GetInt(), cl_crosshair_green.GetInt(), cl_crosshair_blue.GetInt(), 255 );
	flPlayerScale = cl_crosshair_scale.GetFloat() / 32.0f;  // the player can change the scale in the options/multiplayer tab
	
	if ( m_pFrameVar )
	{
		int nFrame = clamp(cl_crosshair_gap.GetInt(), 0, m_nNumFrames);
		if ( nFrame != m_nPrevFrame )
		{
			m_pFrameVar->SetIntValue(nFrame);
		}
	}
#else
	Color clr = m_clrCrosshair;
#endif
	float flWidth = flWeaponScale * flPlayerScale * (float)iTextureW;
	float flHeight = flWeaponScale * flPlayerScale * (float)iTextureH;
	int iWidth = (int)( flWidth + 0.5f );
	int iHeight = (int)( flHeight + 0.5f );
	int iX = (int)( x + 0.5f );
	int iY = (int)( y + 0.5f );

	vgui::ISurface* pSurf = vgui::surface();

	if ( cl_hitmarker.GetBool() && m_iDamaged > 0 )
	{
		Color dmgClr(255, 40, 20, 255);
		float flScaleFactor = m_bKill ? 1.5f : 1.0f;
		float flDmgLerp = RemapValClamped(m_iDamaged, 10.0f, 150.0f, 0.0f, 0.5f);
		if ( m_bKill )
		{
			// TODO(mcoms): overkill
			flDmgLerp += RemapValClamped(m_iDamaged / 3.0f, 10.0f, 150.0f, 0.0f, 0.5f);
		}
		flScaleFactor += flDmgLerp;
		float flDmgWidth = flWidth * flScaleFactor;
		float flDmgHeight = flHeight * flScaleFactor;
		int iDmgWidth = (int)(flDmgWidth + 0.5f);
		int iDmgHeight = (int)(flDmgHeight + 0.5f);

		pSurf->DrawSetColor(dmgClr);
		pSurf->DrawSetTexture(m_iDmgCrosshairTextureID);
		pSurf->DrawTexturedRect(iX - iDmgWidth, iY - iDmgHeight, iX + iDmgWidth, iY + iDmgHeight);
		pSurf->DrawSetTexture(0);

		if (m_flDamageOffTime <= gpGlobals->curtime)
		{
			m_iDamaged = 0;
		}
	}

	if (m_szPreviousCrosshair[0] == '\0')
	{
		return BaseClass::Paint();
	}

	pSurf->DrawSetColor( clr );
	pSurf->DrawSetTexture( m_iCrosshairTextureID );
	pSurf->DrawTexturedRect( iX-iWidth, iY-iHeight, iX+iWidth, iY+iHeight );
	pSurf->DrawSetTexture(0);
}


