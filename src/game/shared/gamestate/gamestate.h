//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef GAMESTATE_H
#define GAMESTATE_H

#ifdef _WIN32
#pragma once
#endif

#include <functional>
#include "igamesystem.h"

class CGameStateManager : public CAutoGameSystemPerFrame
{
public:
	CGameStateManager();

	virtual bool Init() OVERRIDE;
	virtual void Update(float frametime) OVERRIDE;
	virtual void Shutdown() OVERRIDE;
	void RegisterMethod(std::string methodName, const std::function<std::string(const std::string& params) >& method);
	void UnregisterMethod(std::string methodName);
	void QueueEvent(const std::string& strEvent, const std::string& strParams);

	bool IsReady() { return m_bReady; }
	void MarkReady() { m_bReady = true; }

private:
	bool m_bInit = false;
	bool m_bReady = false;
	class CHTTPServerThread* m_pServerThread = NULL;
};

extern CGameStateManager* GetGameStateManager();
#endif
