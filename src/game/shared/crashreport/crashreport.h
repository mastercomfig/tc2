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

	void ReportInfo( const char *logger, const char* message );
	void ReportWarning( const char* logger, const char* message );
	void ReportError( const char* logger, const char* message );
};

extern CCrashReporter* GetCrashReporter();
#endif
