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

#include <unordered_map>
#include <string>
#include <functional>
#include "vgui_controls/HTML.h"

using namespace vgui;

class CInteractiveHTML : public HTML
{
    DECLARE_CLASS_SIMPLE(CInteractiveHTML, HTML);

	CInteractiveHTML(Panel* parent,const char* name, bool allowJavaScript = false, bool bPopupWindow = false) 
	: vgui::HTML(parent, name, allowJavaScript, bPopupWindow) {};

	void AddCommandListener( std::string commandName, const std::function<void( const std::string &psQuery ) > & func );
	void RemoveCommandListener( std::string commandName);
	bool OnStartRequest(const char* url, const char* target, const char* pchPostData, bool bIsRedirect);

private:
	static std::string UrlCommandPrefix;
	std::unordered_map<std::string, std::function<void( const std::string& psQuery )>> m_commandFunctions;
};

#endif	// INTERACTIVEWEBPANEL_H
