//=================== Copyright (c) 2025 PracticeMedicine, all rights reserved. ===================//
//
// Purpose: Sentry integration
//
//=================================================================================================//

#include "cbase.h"

#include <tier0/platform.h>
#include "sentry.h"
#include "crashreport.h"
#include <string>
#include "tier0/icommandline.h"

// memdbgon.h must be the last include in a cpp file!!!!
#include "tier0/memdbgon.h"

#define SENTRY_DSN_LINK "https://a037d8059c0ebd86db1a37f25cf797af@o182209.ingest.us.sentry.io/4510205029711872"
#define SENTRY_RELEASE "tc2@1.0.6"
#define SENTRY_ENVIRONMENT "prod"


CCrashReporter::CCrashReporter()
	: CAutoGameSystemPerFrame("crashreport")
{
}

bool CCrashReporter::Init() 
{
#ifdef GAME_DLL
	sentry_options_t* options = sentry_options_new();
	sentry_options_set_dsn(options, SENTRY_DSN_LINK);

	sentry_options_set_release(options, SENTRY_RELEASE);
	sentry_options_set_debug(options, 1);
	sentry_options_set_environment(options, SENTRY_ENVIRONMENT);
	
	sentry_init(options);
#endif

	return true;
}

void CCrashReporter::Shutdown()
{
#ifdef GAME_DLL
	sentry_close();
#endif
}

// capture events
void CCrashReporter::ReportInfo( const char* logger, const char* message )
{
	sentry_capture_event( sentry_value_new_message_event(
		/*   level */ SENTRY_LEVEL_INFO,
		/*  logger */ logger,
		/* message */ message
	) );
}

void CCrashReporter::ReportWarning( const char* logger, const char* message )
{
	sentry_capture_event( sentry_value_new_message_event(
		/*   level */ SENTRY_LEVEL_WARNING,
		/*  logger */ logger,
		/* message */ message
	) );
}

void CCrashReporter::ReportError( const char* logger, const char* message )
{
	sentry_capture_event( sentry_value_new_message_event(
		/*   level */ SENTRY_LEVEL_ERROR,
		/*  logger */ logger,
		/* message */ message
	) );
}

static CCrashReporter s_CrashReporter;
CCrashReporter *GetCrashReporter() { return &s_CrashReporter; }
