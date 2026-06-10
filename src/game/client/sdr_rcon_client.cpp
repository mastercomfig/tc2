#include "cbase.h"

CON_COMMAND_F( rcon_sdr, "Executes an SDR RCON command.", FCVAR_DONTRECORD )
{
	if ( args.ArgC() < 2 )
	{
		Msg("Usage: rcon_sdr <command> [args...]\n");
		return;
	}

	static ConVarRef rcon_password( "rcon_password" );
	if ( V_strlen( rcon_password.GetString() ) == 0 )
	{
		Msg( "Please set rcon_password first.\n" );
		return;
	}

	engine->ServerCmd( CFmtStrMax( "sdr_rcon_exec \"%s\" %s", rcon_password.GetString(), args.ArgS() ) );
}
