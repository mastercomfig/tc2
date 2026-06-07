//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Radial voice menu selection
//
//=============================================================================//

#ifndef TF_HUD_MENU_VOICE_SELECTION_H
#define TF_HUD_MENU_VOICE_SELECTION_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include "hudelement.h"

using namespace vgui;

class CExLabel;

#define MAX_RADIAL_VOICE_SLOTS 9

class CHudMenuVoiceSelection : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudMenuVoiceSelection, EditablePanel );

public:
	CHudMenuVoiceSelection( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	Paint( void );

	virtual void	SetVisible( bool state );

	int		HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual int GetRenderGroupPriority( void ) { return 90; }

	virtual GameActionSet_t GetPreferredActionSet() { return IsActive() ? GAME_ACTION_SET_IN_GAME_HUD : GAME_ACTION_SET_NONE; }

	void	OpenMenu( int iMenuIndex, bool bMouseLock = false );
	void	OnVoiceMenuRelease();
	int		GetCurrentMenu() { return m_iCurrentMenu; }

	bool	IsMenuOpen() { return IsVisible(); }
	bool	ShouldLockMouse();
	void	UpdateLockState();
	QAngle	GetInitialViewAngles() const { return m_vecInitialViewAngles; }
	
	void	ProcessQueuedCommands();
	
	void	AccumulateMouseDelta( float dx, float dy );

private:
	void	SelectVoiceCommand( int iItem );
	void	UpdateSelectionFromMouse();

private:
	CExLabel *m_pItemLabels[MAX_RADIAL_VOICE_SLOTS];
	CExLabel *m_pNumberLabels[MAX_RADIAL_VOICE_SLOTS];

	bool	CheckConditions( KeyValues *pMenuKV );

	int m_iCurrentMenu;
	int m_iCurrentPage;
	bool m_bInSubPage;
	int m_iSelectedItem;
	int m_iNumOptions;

	struct RadialMenuOption_t
	{
		wchar_t m_wszLabel[64];
		char m_szCommand[256];
	};
	RadialMenuOption_t m_Options[MAX_RADIAL_VOICE_SLOTS];

	QAngle m_vecInitialViewAngles;
	float  m_flMouseYawDelta;
	float  m_flMousePitchDelta;
	float  m_flOpenTime;

	struct QueuedCommand_t
	{
		char m_szCommand[256];
		int  m_iTickDelay;
	};
	CUtlVector<QueuedCommand_t> m_QueuedCommands;

	bool   m_bMouseLocked;
	bool   m_bMouseActuallyLocked;
};

extern CHudMenuVoiceSelection *GetVoiceMenu();

#endif // TF_HUD_MENU_VOICE_SELECTION_H
