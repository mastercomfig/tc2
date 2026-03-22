//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include <vgui/IScheme.h>
#include <vgui_controls/EditablePanel.h>
#include <filesystem.h>

#include "interactivewebpanel.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CInteractiveWebPanel::CInteractiveWebPanel( vgui::Panel *pParent, const char *pName, const char* path, bool bDynamic, bool bLoadOnStart ) : vgui::EditablePanel( pParent, pName )
{
    m_szPath = path;
    m_bInited = false;
	m_bLoadOnStart = bLoadOnStart;

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

	if (m_bLoadOnStart)
	{
		LoadInteractivePanel(false);
	}
	else if (m_bInited)
	{
		// if we haven't inited, then don't load the panel. let someone else do it.
		// but once that someone else did init, we do want to reload on ApplySchemeSettings again.
		LoadInteractivePanel(true);
	}
}

void CInteractiveWebPanel::LoadInteractivePanel()
{
	LoadInteractivePanel(false);
}

void CInteractiveWebPanel::LoadInteractivePanel(bool bForceReload)
{
	if (m_bInited && !bForceReload)
	{
		return;
	}

	// based off of CTeamMenu::LoadMapPage

	const char* pszPath = m_szPath.c_str();

	// it goes through our web server
	CFmtStr1024 fmt("http://127.0.0.1:58270/%s", pszPath);

	m_bInited = true;

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
	// reverse the proportional scale
	int zoomLevel = vgui::scheme()->GetProportionalNormalizedValue( 100 );
	m_pHTML->SetZoomLevel((float)zoomLevel);
}

void CInteractiveWebPanel::SetVisible(bool state)
{
	BaseClass::SetVisible( state );
	if ( m_pHTML )
	{
		m_pHTML->SetVisible( state );
	}
}

