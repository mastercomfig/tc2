//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IScheme.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui/ISurface.h>
#include <filesystem.h>

#include "interactivewebpanel.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CInteractiveWebPanel::CInteractiveWebPanel( vgui::Panel *pParent, const char *pName, const char* path, bool bDynamic ) : vgui::EditablePanel( pParent, pName )
{
    m_bInited = false;
    m_szPath = path;

	m_pHTML = new CInteractiveHTML(this, "InteractiveWebHTML", bDynamic, false );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CInteractiveWebPanel::~CInteractiveWebPanel()
{
}

void CInteractiveWebPanel::ApplySchemeSettings(IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// based off of CTeamMenu::LoadMapPage

	const char* pszPath = m_szPath.c_str();

	if ( !g_pFullFileSystem->FileExists( pszPath ) )
	{
		Warning("Could not find web panel resource!\n");
		return;
	}

	// it's a local HTML file
	char localURL[ _MAX_PATH + 7 ];
	Q_strncpy( localURL, "file://", sizeof( localURL ) );

	char pPathData[ _MAX_PATH ];
	g_pFullFileSystem->GetLocalPath( pszPath, pPathData, sizeof(pPathData) );
	Q_strncat( localURL, pPathData, sizeof( localURL ), COPY_ALL_CHARACTERS );

	// force steam to dump a local copy
	g_pFullFileSystem->GetLocalCopy( pPathData );

	m_pHTML->SetVisible( true );
	m_pHTML->OpenURL( localURL, NULL );
	m_pHTML->SetContextMenuEnabled( false );

	InvalidateLayout();
	Repaint();
}

void CInteractiveWebPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pHTML->SetSize(GetWide(), GetTall());
	int zoomLevel = vgui::scheme()->GetProportionalNormalizedValue(100);
	m_pHTML->SetZoomLevel((float)zoomLevel);
}

void CInteractiveWebPanel::AddCommandListener(std::string commandName,
	const std::function<void(const std::string& psQuery)>& func)
{
	m_pHTML->AddCommandListener(commandName, func);
}

void CInteractiveWebPanel::RemoveCommandListener(std::string commandName)
{
	m_pHTML->RemoveCommandListener(commandName);
}

