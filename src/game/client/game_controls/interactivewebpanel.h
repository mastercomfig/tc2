//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef INTERACTIVEWEBPANEL_H
#define INTERACTIVEWEBPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "vgui_controls/HTML.h"
#include <string>
#include "interactivehtml.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CInteractiveWebPanel : public EditablePanel
{
    DECLARE_CLASS_SIMPLE(CInteractiveWebPanel, EditablePanel);

    CInteractiveWebPanel(vgui::Panel* parent, const char* name, const char* path, bool bDynamic = false, bool bLoadOnStart = true);
    virtual ~CInteractiveWebPanel();

    void ApplySchemeSettings(IScheme* pScheme) override;
    void PerformLayout() override;

    void LoadInteractivePanel();

private:
    void LoadInteractivePanel(bool bForceReload);

    bool m_bInited;
    bool m_bLoadOnStart;
    std::string m_szPath;

    CInteractiveHTML* m_pHTML;
};

#endif	// INTERACTIVEWEBPANEL_H