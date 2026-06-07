//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "c_baseplayer.h"
#include "menu.h"
#include "KeyValues.h"
#include "multiplay_gamerules.h"
#if defined ( TF_CLIENT_DLL )
#include "tf_gc_client.h"
#include "hud_basechat.h"
#include "hud_chat.h"
#include "tf_hud_menu_voice_selection.h"
#endif // TF_CLIENT_DLL

static int g_ActiveVoiceMenu = 0;

#if defined( TF_CLIENT_DLL )
extern ConVar tf_voice_command_suspension_mode;
#endif

void OpenVoiceMenu( int index )
{
	// do not show the menu if the player is dead or is an observer
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsAlive() || pPlayer->IsObserver() )
		return;

#if defined ( TF_CLIENT_DLL )
	if ( GTFGCClientSystem() && GTFGCClientSystem()->BHaveChatSuspensionInCurrentMatch() && tf_voice_command_suspension_mode.GetInt() == 1 )
	{
		CBaseHudChat *pHUDChat = ( CBaseHudChat * ) GET_HUDELEMENT( CHudChat );
		if ( pHUDChat )
		{
			char szLocalized[100];
			g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find( "#TF_Voice_Unavailable" ), szLocalized, sizeof( szLocalized ) );
			pHUDChat->ChatPrintf( 0, CHAT_FILTER_NONE, "%s ", szLocalized );
		}
		
		return;
	}
#endif // TF_CLIENT_DLL 

#if defined( TF_CLIENT_DLL )
	CHudMenuVoiceSelection *pMenu = (CHudMenuVoiceSelection *) gHUD.FindElement( "CHudMenuVoiceSelection" );
#else
	CHudMenu *pMenu = (CHudMenu *) gHUD.FindElement( "CHudMenu" );
#endif
	if ( !pMenu )
		return;

	// if they hit the key again, close the menu
	if ( g_ActiveVoiceMenu == index )
	{
		if ( pMenu->IsMenuOpen() )
		{
#if defined( TF_CLIENT_DLL )
			pMenu->SetVisible( false );
#else
			pMenu->HideMenu();
#endif
			g_ActiveVoiceMenu = 0;
			return;
		}
	}

	if ( index > 0 && index < 9 )
	{
#if defined( TF_CLIENT_DLL )
		pMenu->OpenMenu( index - 1 );
#else
		KeyValues *pKV = new KeyValues( "MenuItems" );

		CMultiplayRules *pRules = dynamic_cast< CMultiplayRules * >( GameRules() );
		if ( pRules )
		{			
			if ( !pRules->GetVoiceMenuLabels( index-1, pKV ) )
			{ 
				pKV->deleteThis();
				return;
			}
		}

		pMenu->ShowMenu_KeyValueItems( pKV );

		pKV->deleteThis();
#endif
		g_ActiveVoiceMenu = index;
	}
	else
	{
		g_ActiveVoiceMenu = 0;
	}
}

static void OpenVoiceMenu_1( void ) { OpenVoiceMenu( 1 ); }
static void OpenVoiceMenu_2( void ) { OpenVoiceMenu( 2 ); }
static void OpenVoiceMenu_3( void ) { OpenVoiceMenu( 3 ); }

static void StartVoiceMenu_1( void )
{
#if defined( TF_CLIENT_DLL )
	CHudMenuVoiceSelection *pMenu = GetVoiceMenu();
	if ( pMenu ) pMenu->OpenMenu( 0, true ); // Index 0, lock mouse
#else
	OpenVoiceMenu( 1 );
#endif
}
static void EndVoiceMenu_1( void )
{
#if defined( TF_CLIENT_DLL )
	CHudMenuVoiceSelection *pMenu = GetVoiceMenu();
	if ( pMenu && pMenu->IsMenuOpen() && pMenu->GetCurrentMenu() == 0 ) pMenu->OnVoiceMenuRelease();
#endif
}

static void StartVoiceMenu_2( void )
{
#if defined( TF_CLIENT_DLL )
	CHudMenuVoiceSelection *pMenu = GetVoiceMenu();
	if ( pMenu ) pMenu->OpenMenu( 1, true );
#else
	OpenVoiceMenu( 2 );
#endif
}
static void EndVoiceMenu_2( void )
{
#if defined( TF_CLIENT_DLL )
	CHudMenuVoiceSelection *pMenu = GetVoiceMenu();
	if ( pMenu && pMenu->IsMenuOpen() && pMenu->GetCurrentMenu() == 1 ) pMenu->OnVoiceMenuRelease();
#endif
}

static void StartVoiceMenu_3( void )
{
#if defined( TF_CLIENT_DLL )
	CHudMenuVoiceSelection *pMenu = GetVoiceMenu();
	if ( pMenu ) pMenu->OpenMenu( 2, true );
#else
	OpenVoiceMenu( 3 );
#endif
}
static void EndVoiceMenu_3( void )
{
#if defined( TF_CLIENT_DLL )
	CHudMenuVoiceSelection *pMenu = GetVoiceMenu();
	if ( pMenu && pMenu->IsMenuOpen() && pMenu->GetCurrentMenu() == 2 ) pMenu->OnVoiceMenuRelease();
#endif
}

ConCommand voice_menu_1( "voice_menu_1", OpenVoiceMenu_1, "Opens voice menu 1" );
ConCommand voice_menu_2( "voice_menu_2", OpenVoiceMenu_2, "Opens voice menu 2" );
ConCommand voice_menu_3( "voice_menu_3", OpenVoiceMenu_3, "Opens voice menu 3" );

ConCommand start_voice_menu_1( "+voice_menu_1", StartVoiceMenu_1, "Opens voice menu 1 (hold)" );
ConCommand end_voice_menu_1( "-voice_menu_1", EndVoiceMenu_1, "Closes voice menu 1 (release)" );
ConCommand start_voice_menu_2( "+voice_menu_2", StartVoiceMenu_2, "Opens voice menu 2 (hold)" );
ConCommand end_voice_menu_2( "-voice_menu_2", EndVoiceMenu_2, "Closes voice menu 2 (release)" );
ConCommand start_voice_menu_3( "+voice_menu_3", StartVoiceMenu_3, "Opens voice menu 3 (hold)" );
ConCommand end_voice_menu_3( "-voice_menu_3", EndVoiceMenu_3, "Closes voice menu 3 (release)" );

CON_COMMAND( menuselect, "menuselect" )
{
	if ( args.ArgC() < 2 )
		return;

	if( g_ActiveVoiceMenu == 0 )
	{
		// if we didn't have a menu open, maybe a plugin did.  send it on to the server.
		const char *cmd = VarArgs( "menuselect %s", args[1] );
		engine->ServerCmd( cmd );
		return;
	}

	int iSelection = atoi( args[ 1 ] );

	switch( g_ActiveVoiceMenu )
	{
	case 1:
	case 2:
	case 3:
		{
			char cmd[128];
			Q_snprintf( cmd, sizeof(cmd), "voicemenu %d %d", g_ActiveVoiceMenu - 1, iSelection - 1 );
			engine->ServerCmd( cmd );
		}
		break;

	default:
		{
			// if we didn't have a menu open, maybe a plugin did.  send it on to the server.
			const char *cmd = VarArgs( "menuselect %d", iSelection );
			engine->ServerCmd( cmd );
		}
		break;
	}

	// reset menu
	g_ActiveVoiceMenu = 0;
}