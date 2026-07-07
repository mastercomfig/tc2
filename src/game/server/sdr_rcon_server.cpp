#include "cbase.h"
#include "igamesystem.h"
#include "icvar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CSDRRconServer;

static SpewOutputFunc_t g_pOldSpew = NULL;

SpewRetval_t SDRRconSpewFunc( SpewType_t spewType, const tchar *pMsg );

class CSDRRconServer : public CAutoGameSystem
{
public:
	CSDRRconServer() : CAutoGameSystem("CSDRRconServer")
	{
		m_pInterceptClient = NULL;
	}

	virtual bool Init() OVERRIDE
	{
		g_pOldSpew = GetSpewOutputFunc();
		SpewOutputFunc( SDRRconSpewFunc );
		return true;
	}

	virtual void Shutdown() OVERRIDE
	{
		SpewOutputFunc( g_pOldSpew );
	}

	void SetInterceptClient( CBasePlayer *pPlayer )
	{
		m_pInterceptClient = pPlayer;
	}

	CBasePlayer *GetInterceptClient()
	{
		return m_pInterceptClient;
	}

private:
	CBasePlayer *m_pInterceptClient;
};

static CSDRRconServer s_SDRRconServer;

SpewRetval_t SDRRconSpewFunc( SpewType_t spewType, const tchar *pMsg )
{
	static bool bInSpew = false;
	
	if ( !bInSpew && s_SDRRconServer.GetInterceptClient() )
	{
		bInSpew = true;
		engine->ClientPrintf( s_SDRRconServer.GetInterceptClient()->edict(), pMsg );
		bInSpew = false;
	}
	
	if ( g_pOldSpew )
		return g_pOldSpew( spewType, pMsg );
		
	return SPEW_CONTINUE;
}

CSDRRconServer* SDRRconServer()
{
	return &s_SDRRconServer;
}

CON_COMMAND_F( sdr_rcon_exec, "Internal SDR RCON execution", FCVAR_HIDDEN | FCVAR_DONTRECORD )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer ) return;
	
	if ( args.ArgC() < 3 )
	{
		engine->ClientPrintf( pPlayer->edict(), "Usage: sdr_rcon_exec <password> <command>\n" );
		return;
	}
	
	ConVarRef rcon_password("rcon_password");
	if ( !rcon_password.IsValid() || V_strlen( rcon_password.GetString() ) == 0 )
	{
		engine->ClientPrintf( pPlayer->edict(), "RCON is not enabled on this server.\n" );
		return;
	}
	
	if ( V_strcmp( args[1], rcon_password.GetString() ) != 0 )
	{
		engine->ClientPrintf( pPlayer->edict(), "Bad RCON password.\n" );
		return;
	}

	// Extract the actual command. args.ArgS() has the password as the first token.
	// We advance past it.
	const char *pArgS = args.ArgS();
	
	// Skip leading whitespace
	while ( *pArgS && isspace(*pArgS) ) pArgS++;
	
	// Skip password
	if ( *pArgS == '\"' )
	{
		pArgS++;
		while ( *pArgS && *pArgS != '\"' ) pArgS++;
		if ( *pArgS == '\"' ) pArgS++;
	}
	else
	{
		while ( *pArgS && !isspace(*pArgS) ) pArgS++;
	}
	
	// Skip whitespace before the command
	while ( *pArgS && isspace(*pArgS) ) pArgS++;
	
	if ( V_strlen( pArgS ) == 0 ) return;

	SDRRconServer()->SetInterceptClient( pPlayer );
	engine->ServerCommand( UTIL_VarArgs( "%s\n", pArgS ) );
	engine->ServerExecute();
	SDRRconServer()->SetInterceptClient( NULL );
}
