//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Radial voice menu selection
//
//=============================================================================//

#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "tf_hud_menu_voice_selection.h"
#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/AnimationController.h>
#include "econ/econ_controls.h"
#include <math.h>
#include "multiplay_gamerules.h"
#include "c_baseplayer.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_radial_voicemenu_concise( "tf_radial_voicemenu_concise", "0", FCVAR_ARCHIVE, "If 1, splits the radial voice menu into 4-slot sub-menus." );
ConVar tf_radial_voicemenu_hold_delay( "tf_radial_voicemenu_hold_delay", "0.2", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Seconds to wait while holding before locking the mouse for the radial menu" );

constexpr int RADIAL_MENU_RADIUS = 210;

DECLARE_HUDELEMENT( CHudMenuVoiceSelection );

static CHudMenuVoiceSelection *g_pVoiceMenu = NULL;

CHudMenuVoiceSelection *GetVoiceMenu()
{
	return g_pVoiceMenu;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudMenuVoiceSelection::CHudMenuVoiceSelection( const char *pElementName ) 
	: CHudElement( pElementName ), BaseClass( NULL, "HudMenuVoiceSelection" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	g_pVoiceMenu = this;

	for ( int i = 0; i < MAX_RADIAL_VOICE_SLOTS; i++ )
	{
		m_pItemLabels[i] = new CExLabel( this, VarArgs("ItemLabel%d", i + 1), "" );
		m_pItemLabels[i]->SetCenterWrap( true );
		m_pItemLabels[i]->SetWide( XRES( 35 ) );
		m_pItemLabels[i]->SetTall( YRES( 60 ) );
		m_pNumberLabels[i] = new CExLabel( this, VarArgs("NumberLabel%d", i + 1), VarArgs("%d", i + 1) );
	}

	m_iCurrentMenu = -1;
	m_iCurrentPage = 0;
	m_bInSubPage = false;
	m_iSelectedItem = -1;
	m_iNumOptions = 0;
	m_vecInitialViewAngles.Init();
	m_flMouseYawDelta = 0;
	m_flMousePitchDelta = 0;
	m_flOpenTime = 0;
	m_QueuedCommands.RemoveAll();
	m_bMouseActuallyLocked = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuVoiceSelection::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	LoadControlSettings( "resource/UI/HudMenuVoiceSelection.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMenuVoiceSelection::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
		return false;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer || !pPlayer->IsAlive() || pPlayer->IsObserver() )
	{
		return false;
	}

	return m_iCurrentMenu >= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuVoiceSelection::SetVisible( bool state )
{
	if ( state == IsVisible() )
		return;

	if ( state )
	{
		m_flMouseYawDelta = 0;
		m_flMousePitchDelta = 0;
		m_iSelectedItem = -1;
		m_bMouseActuallyLocked = false;
	}
	else
	{
		m_iCurrentMenu = -1;
		m_bInSubPage = false;
	}

	BaseClass::SetVisible( state );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMenuVoiceSelection::ShouldLockMouse()
{
	if ( !m_bMouseLocked )
		return false;

	return ( gpGlobals->curtime - m_flOpenTime ) >= tf_radial_voicemenu_hold_delay.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuVoiceSelection::UpdateLockState()
{
	if ( ShouldLockMouse() && !m_bMouseActuallyLocked )
	{
		engine->GetViewAngles( m_vecInitialViewAngles );
		m_bMouseActuallyLocked = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuVoiceSelection::ProcessQueuedCommands()
{
	for ( int i = 0; i < m_QueuedCommands.Count(); )
	{
		m_QueuedCommands[i].m_iTickDelay--;
		if ( m_QueuedCommands[i].m_iTickDelay <= 0 )
		{
			if ( m_QueuedCommands[i].m_szCommand[0] != '\0' )
			{
				engine->ExecuteClientCmd( m_QueuedCommands[i].m_szCommand );
			}
			m_QueuedCommands.Remove( i );
		}
		else
		{
			i++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuVoiceSelection::AccumulateMouseDelta( float dx, float dy )
{
	// Sensitivity factor to match aiming roughly.
	m_flMouseYawDelta += dx * 0.5f;
	m_flMousePitchDelta += dy * 0.5f;

	// Clamp magnitude to prevent the virtual cursor from getting "lost" infinitely far away
	float len = sqrt( m_flMouseYawDelta * m_flMouseYawDelta + m_flMousePitchDelta * m_flMousePitchDelta );
	float maxLen = RADIAL_MENU_RADIUS; // Max radius for the virtual cursor
	if ( len > maxLen )
	{
		m_flMouseYawDelta = (m_flMouseYawDelta / len) * maxLen;
		m_flMousePitchDelta = (m_flMousePitchDelta / len) * maxLen;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuVoiceSelection::Paint( void )
{
	if ( m_iNumOptions <= 0 )
	{
		BaseClass::Paint();
		return;
	}

	bool bConcise = tf_radial_voicemenu_concise.GetBool();
	int nMaxPages = (m_iNumOptions + 3) / 4;
	int nVisibleOptions = m_iNumOptions;

	if ( bConcise )
	{
		if ( !m_bInSubPage )
		{
			nVisibleOptions = nMaxPages;
		}
		else
		{
			nVisibleOptions = 4;
			int itemsLeft = m_iNumOptions - m_iCurrentPage * 4;
			if ( itemsLeft < 4 ) nVisibleOptions = itemsLeft;
		}
	}

	int iNewSelection = -1;

	// Determine selection based on accumulated mouse delta
	float flDistanceSq = m_flMouseYawDelta * m_flMouseYawDelta + m_flMousePitchDelta * m_flMousePitchDelta;

	if ( flDistanceSq > 100.0f ) // Deadzone
	{
		float x = m_flMouseYawDelta; 
		float y = -m_flMousePitchDelta; // invert y so up is positive for atan2
		float pieAngle = atan2( y, x ) * ( 180.0f / M_PI );
		
		pieAngle = 90.0f - pieAngle; // Shift so 0 is top and clockwise is positive
		if ( pieAngle < 0 ) pieAngle += 360.0f;
		
		float sliceSize = 360.0f / nVisibleOptions;
		iNewSelection = (int)( (pieAngle + (sliceSize * 0.5f)) / sliceSize ) % nVisibleOptions;
	}

	// Dynamic positioning
	int cx = GetWide() / 2;
	int cy = GetTall() / 2;
	int radius = RADIAL_MENU_RADIUS;

	for ( int i = 0; i < MAX_RADIAL_VOICE_SLOTS; i++ )
	{
		m_pItemLabels[i]->SetVisible( false );
		m_pNumberLabels[i]->SetVisible( false );
	}

	if ( bConcise )
	{
		for ( int i = 0; i < nVisibleOptions; i++ )
		{
			float angle = ( i * 360.0f / nVisibleOptions ) * ( M_PI / 180.0f );
			int lx = cx + sin( angle ) * radius;
			int ly = cy - cos( angle ) * radius;

			CExLabel *pLabel = m_pItemLabels[i];
			CExLabel *pNumLabel = m_pNumberLabels[i];

			if ( !m_bInSubPage )
			{
				pLabel->SetText( VarArgs("Page %d", i + 1) );
			}
			else
			{
				int itemIdx = m_iCurrentPage * 4 + i;
				pLabel->SetText( m_Options[itemIdx].m_wszLabel );
			}
			pNumLabel->SetText( VarArgs("%d", i + 1) );

			pLabel->SetPos( lx - pLabel->GetWide() / 2, ly - pLabel->GetTall() / 2 );
			pLabel->SetVisible( true );
			pLabel->SetFgColor( iNewSelection == i ? Color(178, 82, 22, 255) : Color(255, 255, 255, 255) );

			pNumLabel->SetPos( lx - pNumLabel->GetWide() / 2, ly - pNumLabel->GetTall() / 2 + 20 );
			pNumLabel->SetVisible( true );
		}
	}
	else
	{
		for ( int i = 0; i < m_iNumOptions; i++ )
		{
			float angle = ( i * 360.0f / m_iNumOptions ) * ( M_PI / 180.0f );
			int lx = cx + sin( angle ) * radius;
			int ly = cy - cos( angle ) * radius;

			m_pItemLabels[i]->SetText( m_Options[i].m_wszLabel );
			m_pItemLabels[i]->SetPos( lx - m_pItemLabels[i]->GetWide() / 2, ly - m_pItemLabels[i]->GetTall() / 2 );
			m_pItemLabels[i]->SetVisible( true );
			m_pItemLabels[i]->SetFgColor( iNewSelection == i ? Color(178, 82, 22, 255) : Color(255, 255, 255, 255) );

			m_pNumberLabels[i]->SetText( VarArgs("%d", i + 1) );
			m_pNumberLabels[i]->SetPos( lx - m_pNumberLabels[i]->GetWide() / 2, ly - m_pNumberLabels[i]->GetTall() / 2 + 20 );
			m_pNumberLabels[i]->SetVisible( true );
		}
	}

	m_iSelectedItem = iNewSelection;

	BaseClass::Paint();

	// Draw indicator line representing the virtual cursor
	float dirX = m_flMouseYawDelta;
	float dirY = m_flMousePitchDelta;
	float len = sqrt( dirX * dirX + dirY * dirY );
	
	if ( len > 0 )
	{
		vgui::surface()->DrawSetColor( Color(178, 82, 22, 255) );
		vgui::surface()->DrawLine( cx, cy, cx + dirX, cy + dirY );
		
		// Draw a small dot at the end for the cursor
		vgui::surface()->DrawFilledRect( cx + dirX - 2, cy + dirY - 2, cx + dirX + 2, cy + dirY + 2 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CHudMenuVoiceSelection::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !IsVisible() )
		return 1;

	if ( !down )
		return 0;

	int iSlot = -1;

	switch ( keynum )
	{
	case KEY_1:	iSlot = 0; break;
	case KEY_2:	iSlot = 1; break;
	case KEY_3:	iSlot = 2; break;
	case KEY_4:	iSlot = 3; break;
	case KEY_5:	iSlot = 4; break;
	case KEY_6:	iSlot = 5; break;
	case KEY_7:	iSlot = 6; break;
	case KEY_8:	iSlot = 7; break;
	case KEY_9:	iSlot = 8; break;
	default:
		break;
	}

	if ( iSlot >= 0 && iSlot < m_iNumOptions )
	{
		SelectVoiceCommand( iSlot );
		return 0;
	}

	if ( m_bMouseActuallyLocked )
	{
		if ( ( pszCurrentBinding && !Q_strcmp( pszCurrentBinding, "+attack" ) ) || keynum == MOUSE_LEFT )
		{
			if ( m_iSelectedItem >= 0 )
			{
				SelectVoiceCommand( m_iSelectedItem );
			}
			return 0;
		}
		
		// Any other bind might cancel, or we just let it pass
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudMenuVoiceSelection::CheckConditions( KeyValues *pMenuKV )
{
	const char *pszClass = pMenuKV->GetString( "class", NULL );
	if ( pszClass && pszClass[0] )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pPlayer ) return false;
		
		int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
		if ( iClass > TF_CLASS_UNDEFINED && iClass < TF_CLASS_MENU_BUTTONS )
		{
			if ( Q_stricmp( pszClass, g_aRawPlayerClassNames[iClass] ) != 0 &&
				 Q_stricmp( pszClass, g_aRawPlayerClassNamesShort[iClass] ) != 0 )
			{
				return false;
			}
		}
	}

	const char *pszGamemode = pMenuKV->GetString( "gamemode", NULL );
	if ( pszGamemode && pszGamemode[0] )
	{
		if ( !TFGameRules() ) return false;
		
		if ( !Q_stricmp( pszGamemode, "mvm" ) && !TFGameRules()->IsMannVsMachineMode() ) return false;
		else if ( !Q_stricmp( pszGamemode, "arena" ) && !TFGameRules()->IsInArenaMode() ) return false;
		else if ( !Q_stricmp( pszGamemode, "koth" ) && !TFGameRules()->IsInKothMode() ) return false;
		else if ( !Q_stricmp( pszGamemode, "pass" ) && !TFGameRules()->IsPasstimeMode() ) return false;
		else if ( !Q_stricmp( pszGamemode, "medieval" ) && !TFGameRules()->IsInMedievalMode() ) return false;
		else if ( !Q_stricmp( pszGamemode, "sd" ) && !TFGameRules()->IsPlayingSpecialDeliveryMode() ) return false;
		else if ( !Q_stricmp( pszGamemode, "rd" ) && !TFGameRules()->IsPlayingRobotDestructionMode() ) return false;
		else if ( !Q_stricmp( pszGamemode, "ctf" ) && TFGameRules()->GetGameType() != TF_GAMETYPE_CTF ) return false;
		else if ( !Q_stricmp( pszGamemode, "cp" ) && TFGameRules()->GetGameType() != TF_GAMETYPE_CP ) return false;
		else if ( !Q_stricmp( pszGamemode, "pl" ) && TFGameRules()->GetGameType() != TF_GAMETYPE_ESCORT ) return false;
		else if ( !Q_stricmp( pszGamemode, "pd" ) && TFGameRules()->GetGameType() != TF_GAMETYPE_PD ) return false;
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuVoiceSelection::OpenMenu( int iMenuIndex, bool bMouseLock )
{
	if ( m_iCurrentMenu == iMenuIndex && IsVisible() )
	{
		SetVisible( false );
		m_iCurrentMenu = -1;
		return;
	}

	m_iNumOptions = 0;

	KeyValues *pRadialMenus = new KeyValues( "RadialMenus" );
	if ( pRadialMenus->LoadFromFile( g_pFullFileSystem, "scripts/radial_menus.txt", "MOD" ) )
	{
		char szMenuName[32];
		Q_snprintf( szMenuName, sizeof(szMenuName), "Menu%d", iMenuIndex + 1 );
		
		KeyValues *pMenuKV = NULL;
		for ( KeyValues *pIter = pRadialMenus->GetFirstSubKey(); pIter != NULL; pIter = pIter->GetNextKey() )
		{
			if ( Q_stricmp( pIter->GetName(), szMenuName ) == 0 )
			{
				if ( CheckConditions( pIter ) )
				{
					pMenuKV = pIter;
					break; // Choose the first one that successfully passed its conditions
				}
			}
		}

		if ( pMenuKV )
		{
			for ( KeyValues *sub = pMenuKV->GetFirstSubKey(); sub; sub = sub->GetNextKey() )
			{
				if ( Q_stricmp( sub->GetName(), "class" ) == 0 ||
					 Q_stricmp( sub->GetName(), "gamemode" ) == 0 )
				{
					continue;
				}

				if ( m_iNumOptions < MAX_RADIAL_VOICE_SLOTS )
				{
					const char *pszLabel = sub->GetString( "label", "Unknown" );
					const wchar_t *pwszLocalized = NULL;

					if ( pszLabel[0] == '#' )
					{
						pwszLocalized = g_pVGuiLocalize->Find( pszLabel );
					}

					if ( pwszLocalized )
					{
						V_wcsncpy( m_Options[m_iNumOptions].m_wszLabel, pwszLocalized, sizeof(m_Options[0].m_wszLabel) );
					}
					else
					{
						g_pVGuiLocalize->ConvertANSIToUnicode( pszLabel, m_Options[m_iNumOptions].m_wszLabel, sizeof(m_Options[0].m_wszLabel) );
					}

					Q_strncpy( m_Options[m_iNumOptions].m_szCommand, sub->GetString( "command", "" ), sizeof(m_Options[0].m_szCommand) );
					m_iNumOptions++;
				}
			}
		}
	}
	pRadialMenus->deleteThis();

	if ( m_iNumOptions == 0 )
	{
		// Fallback to legacy voicemenu
		KeyValues *pKV = new KeyValues( "MenuItems" );

		CMultiplayRules *pRules = dynamic_cast< CMultiplayRules * >( GameRules() );
		if ( pRules )
		{			
			if ( pRules->GetVoiceMenuLabels( iMenuIndex, pKV ) )
			{ 
				for ( KeyValues *sub = pKV->GetFirstSubKey(); sub; sub = sub->GetNextKey() )
				{
					if ( m_iNumOptions < MAX_RADIAL_VOICE_SLOTS )
					{
						const char *pszLabel = sub->GetName();
						const wchar_t *pwszLocalized = NULL;

						if ( pszLabel[0] == '#' )
						{
							pwszLocalized = g_pVGuiLocalize->Find( pszLabel );
						}

						if ( pwszLocalized )
						{
							V_wcsncpy( m_Options[m_iNumOptions].m_wszLabel, pwszLocalized, sizeof(m_Options[0].m_wszLabel) );
						}
						else
						{
							g_pVGuiLocalize->ConvertANSIToUnicode( pszLabel, m_Options[m_iNumOptions].m_wszLabel, sizeof(m_Options[0].m_wszLabel) );
						}

						Q_snprintf( m_Options[m_iNumOptions].m_szCommand, sizeof(m_Options[0].m_szCommand), "voicemenu %d %d", iMenuIndex, m_iNumOptions );
						m_iNumOptions++;
					}
				}
			}
		}
		pKV->deleteThis();
	}

	for ( int i = m_iNumOptions; i < MAX_RADIAL_VOICE_SLOTS; i++ )
	{
		m_pItemLabels[i]->SetVisible( false );
		m_pNumberLabels[i]->SetVisible( false );
	}

	m_iCurrentMenu = iMenuIndex;
	m_bInSubPage = false;
	m_bMouseLocked = bMouseLock;
	m_flOpenTime = gpGlobals->curtime;
	SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuVoiceSelection::OnVoiceMenuRelease()
{
	if ( m_iSelectedItem >= 0 )
	{
		SelectVoiceCommand( m_iSelectedItem );
	}
	else
	{
		m_bMouseLocked = false;
		m_bMouseActuallyLocked = false;
		m_flMouseYawDelta = 0;
		m_flMousePitchDelta = 0;
		m_iSelectedItem = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudMenuVoiceSelection::SelectVoiceCommand( int iItem )
{
	if ( tf_radial_voicemenu_concise.GetBool() )
	{
		if ( !m_bInSubPage )
		{
			int nMaxPages = (m_iNumOptions + 3) / 4;
			if ( iItem >= 0 && iItem < nMaxPages )
			{
				m_iCurrentPage = iItem;
				m_bInSubPage = true;
				m_flMouseYawDelta = 0;
				m_flMousePitchDelta = 0;
			}
			return;
		}
		
		int itemsLeft = m_iNumOptions - m_iCurrentPage * 4;
		if ( iItem >= 0 && iItem < min(4, itemsLeft) )
		{
			iItem = m_iCurrentPage * 4 + iItem;
		}
		else
		{
			return; // Invalid
		}
	}

	if ( m_iCurrentMenu >= 0 )
	{
		if ( m_Options[iItem].m_szCommand[0] != '\0' )
		{
			engine->ExecuteClientCmd( m_Options[iItem].m_szCommand );
			
			// If it's a +command, automatically fire the -command next tick
			if ( m_Options[iItem].m_szCommand[0] == '+' )
			{
				QueuedCommand_t queuedCmd;
				queuedCmd.m_szCommand[0] = '-';
				Q_strncpy( queuedCmd.m_szCommand + 1, m_Options[iItem].m_szCommand + 1, sizeof( queuedCmd.m_szCommand ) - 1 );
				queuedCmd.m_iTickDelay = 1;
				m_QueuedCommands.AddToTail( queuedCmd );
			}
		}
	}

	SetVisible( false );
	m_iCurrentMenu = -1;
}
