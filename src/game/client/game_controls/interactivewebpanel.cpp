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

	// it goes through our web server
	CFmtStr1024 fmt("http://127.0.0.1:58270/%s", pszPath);

	m_pHTML->SetVisible( true );
	m_pHTML->OpenURL( fmt.Get(), NULL );
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

