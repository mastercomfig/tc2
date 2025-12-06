//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef CRASHREPORT_H
#define CRASHREPORT_H

#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"

class CCrashReporter : public CAutoGameSystemPerFrame
{
public:
	CCrashReporter();

	virtual bool Init();
	virtual void Shutdown();

	static void AssertCallbackFunc( const char *pchFile, int nLine, const char *pchMessage );
	static SpewRetval_t SpewFunc( SpewType_t type, const tchar* pMsg );

	void ReportInfo( const char *logger, const char* message );
	void ReportWarning( const char* logger, const char* message );
	void ReportError( const char* logger, const char* message );

private:
	bool m_bInit = false;
	SpewOutputFunc_t m_OutputFuncPrev;
};

extern CCrashReporter* GetCrashReporter();
#endif
