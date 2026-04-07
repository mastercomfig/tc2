//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HUD_TF_CROSSHAIR_H
#define HUD_TF_CROSSHAIR_H
#ifdef _WIN32
#pragma once
#endif

#include "hud_crosshair.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct CCrosshairElement
{
	CCrosshairElement();
	~CCrosshairElement();

	void Init();
	void Shutdown();
	void Update( const char* szTextureName );
	void Draw( int x, int y, float flScale, Color color, int iGap = 0 );
	bool HasCrosshair() { return m_szTextureName[0] != '\0'; }

	// texture ID
	int m_iTextureID;
	// name of the current crosshair
	char m_szTextureName[256];
	IVguiMatInfo* m_pMaterial;
	
	IVguiMatInfoVar* m_pFrameVar;
	int m_nNumFrames;
	int m_nFrame;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudTFCrosshair : public CHudCrosshair
{
public:
	DECLARE_CLASS_SIMPLE( CHudTFCrosshair, CHudCrosshair );

	CHudTFCrosshair( const char *name );

	virtual void Init() OVERRIDE;
	virtual void LevelShutdown( void ) OVERRIDE;
	virtual bool ShouldDraw() OVERRIDE;

protected:
	virtual void Paint() OVERRIDE;
	virtual void FireGameEvent( IGameEvent * event ) OVERRIDE;
	void HandleDamageEvent(CTFPlayer* pAttacker, CBaseCombatCharacter* pVictim, int iDamage, int iHealth);

private:
	CCrosshairElement m_Crosshair;
	CCrosshairElement m_DmgCrosshair;
	CCrosshairElement m_StickbombViewCrosshair;

	int					m_iDamaged;
	bool				m_bKill;
	float				m_flDamageOffTime;

	float				m_flTimeToHideUntil;
};


#endif // HUD_TF_CROSSHAIR_H
