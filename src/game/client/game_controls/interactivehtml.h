//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef INTERACTIVEHTML_H
#define INTERACTIVEHTML_H

#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/HTML.h"

using namespace vgui;

class CInteractiveHTML : public HTML
{
    DECLARE_CLASS_SIMPLE(CInteractiveHTML, HTML);

	CInteractiveHTML(Panel* parent,const char* name, bool allowJavaScript = false, bool bPopupWindow = false) 
	: HTML(parent, name, allowJavaScript, bPopupWindow) {}
};

#endif	// INTERACTIVEWEBPANEL_H
