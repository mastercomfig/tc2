//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Crash reporter
//
//===========================================================================//

#include "sentry.h"

#include "cbase.h"

#include "crashreport.h"

#include <tier0/platform.h>

// memdbgon.h must be the last include in a cpp file!!!!
#include "tier0/memdbgon.h"

#define SENTRY_DSN_LINK "https://a037d8059c0ebd86db1a37f25cf797af@o182209.ingest.us.sentry.io/4510205029711872"
#define SENTRY_RELEASE "tc2@1.0.6"
#define SENTRY_ENVIRONMENT "prod"

#if defined( _WIN32 )
	#define CRASHPAD_HANDLER	PLATFORM_BIN_DIR "\\crashpad_handler.exe"
	#define SENTRY_DB_PATH		PLATFORM_BIN_DIR "\\.sentry"
#elif defined( POSIX )
	#define CRASHPAD_HANDLER	PLATFORM_BIN_DIR "/crashpad_handler"
	#define SENTRY_DB_PATH		PLATFORM_BIN_DIR "/.sentry"
#endif

static CCrashReporter s_CrashReporter;
CCrashReporter* GetCrashReporter() { return &s_CrashReporter; }


CCrashReporter::CCrashReporter()
	: CAutoGameSystemPerFrame("crashreport")
{
}

bool CCrashReporter::Init() 
{
#ifdef GAME_DLL
	if (!engine->IsDedicatedServer())
		return true;
#endif

	if ( CommandLine()->FindParm ( "-nosentry" ) )
		return true;

	if (m_bInit)
		return true;

	SetAssertFailedNotifyFunc( AssertCallbackFunc );
	m_OutputFuncPrev = GetSpewOutputFunc();
	SpewOutputFunc(&SpewFunc);

	sentry_options_t* options = sentry_options_new();
	sentry_options_set_dsn(options, SENTRY_DSN_LINK);

	sentry_options_set_handler_path(options, CRASHPAD_HANDLER);
	sentry_options_set_database_path(options, SENTRY_DB_PATH);

	sentry_options_set_release(options, SENTRY_RELEASE);
	sentry_options_set_environment(options, SENTRY_ENVIRONMENT);

	sentry_options_set_enable_logs(options, 1);
	
	sentry_init(options);

	m_bInit = true;

	return true;
}

void CCrashReporter::Shutdown()
{
#ifdef GAME_DLL
	if (!engine->IsDedicatedServer())
		return;
#endif

	if (m_bInit)
	{
		SpewOutputFunc(m_OutputFuncPrev);
		sentry_close();
	}
}

void CCrashReporter::AssertCallbackFunc( const char* pchFile, int nLine, const char* pchMessage )
{
	sentry_value_t event = sentry_value_new_message_event( SENTRY_LEVEL_ERROR, "assert", pchMessage );
	sentry_value_set_stacktrace(event, nullptr, 0);
	sentry_capture_event( event );
}

SpewRetval_t CCrashReporter::SpewFunc(SpewType_t type, const tchar* pMsg)
{
	SpewRetval_t ret = s_CrashReporter.m_OutputFuncPrev(type, pMsg);
	switch (type)
	{
	case SPEW_WARNING:
		sentry_log_warn(pMsg);
		break;
	case SPEW_ERROR:
		s_CrashReporter.ReportError("spew", pMsg);
		break;
	default:
		// nothing
		break;
	}

	return ret;
}

// capture events
void CCrashReporter::ReportInfo( const char* logger, const char* message )
{
	sentry_capture_event( sentry_value_new_message_event( SENTRY_LEVEL_INFO, logger, message ) );
}

void CCrashReporter::ReportWarning( const char* logger, const char* message )
{
	sentry_capture_event( sentry_value_new_message_event( SENTRY_LEVEL_WARNING, logger, message ) );
}

void CCrashReporter::ReportError( const char* logger, const char* message )
{
	sentry_value_t event = sentry_value_new_message_event( SENTRY_LEVEL_ERROR, logger, message );
	sentry_value_set_stacktrace(event, nullptr, 0);
	sentry_capture_event( event );
}