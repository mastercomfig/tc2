//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Refactored voice menu handling for improved clarity and performance
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
#endif // TF_CLIENT_DLL

static int g_ActiveVoiceMenu = 0;

void OpenVoiceMenu(int index)
{
    // Check for valid local player, and ensure they're alive and not observing.
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if (!pPlayer || !pPlayer->IsAlive() || pPlayer->IsObserver())
        return;

#if defined ( TF_CLIENT_DLL )
    if (auto* pGCClient = GTFGCClientSystem())
    {
        if (pGCClient->BHaveChatSuspensionInCurrentMatch())
        {
            if (CBaseHudChat* pHUDChat = static_cast<CBaseHudChat*>(GET_HUDELEMENT(CHudChat)))
            {
                char szLocalized[100];
                g_pVGuiLocalize->ConvertUnicodeToANSI(g_pVGuiLocalize->Find("#TF_Voice_Unavailable"),
                    szLocalized, sizeof(szLocalized));
                pHUDChat->ChatPrintf(0, CHAT_FILTER_NONE, "%s ", szLocalized);
            }
            return;
        }
    }
#endif // TF_CLIENT_DLL

    // Get the HUD menu element.
    CHudMenu* pMenu = static_cast<CHudMenu*>(gHUD.FindElement("CHudMenu"));
    if (!pMenu)
        return;

    // If the same menu is active and open, toggle it off.
    if (g_ActiveVoiceMenu == index && pMenu->IsMenuOpen())
    {
        pMenu->HideMenu();
        g_ActiveVoiceMenu = 0;
        return;
    }

    // Valid voice menus are 1 through 8.
    if (index >= 1 && index <= 8)
    {
        KeyValues* pKV = new KeyValues("MenuItems");

        if (CMultiplayRules* pRules = dynamic_cast<CMultiplayRules*>(GameRules()))
        {
            if (!pRules->GetVoiceMenuLabels(index - 1, pKV))
            {
                pKV->deleteThis();
                return;
            }
        }

        pMenu->ShowMenu_KeyValueItems(pKV);
        pKV->deleteThis();
        g_ActiveVoiceMenu = index;
    }
    else
    {
        g_ActiveVoiceMenu = 0;
    }
}

static void OpenVoiceMenu_1(void) { OpenVoiceMenu(1); }
static void OpenVoiceMenu_2(void) { OpenVoiceMenu(2); }
static void OpenVoiceMenu_3(void) { OpenVoiceMenu(3); }

ConCommand voice_menu_1("voice_menu_1", OpenVoiceMenu_1, "Opens voice menu 1");
ConCommand voice_menu_2("voice_menu_2", OpenVoiceMenu_2, "Opens voice menu 2");
ConCommand voice_menu_3("voice_menu_3", OpenVoiceMenu_3, "Opens voice menu 3");

CON_COMMAND(menuselect, "menuselect")
{
    if (args.ArgC() < 2)
        return;

    // If no voice menu is active, forward the command to the server.
    if (g_ActiveVoiceMenu == 0)
    {
        engine->ServerCmd(VarArgs("menuselect %s", args[1]));
        return;
    }

    int iSelection = atoi(args[1]);
    char cmd[128];

    // For voice menus 1-3, format the voicemenu command accordingly.
    if (g_ActiveVoiceMenu >= 1 && g_ActiveVoiceMenu <= 3)
    {
        Q_snprintf(cmd, sizeof(cmd), "voicemenu %d %d", g_ActiveVoiceMenu - 1, iSelection - 1);
    }
    else
    {
        // Fallback to sending a generic menuselect command.
        Q_snprintf(cmd, sizeof(cmd), "menuselect %d", iSelection);
    }
    engine->ServerCmd(cmd);

    // Reset the active menu.
    g_ActiveVoiceMenu = 0;
}
