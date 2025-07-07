"GameMenu"
{
	"ResumeGameButton"
	{
		"label"			"#MMenu_ResumeGame"
		"command"		"ResumeGame"
		"OnlyInGame"	"1"
		"subimage" "icon_resume"
	}
	
	"ServerBrowserButton"
	{
		"label" "#MMenu_PlayMultiplayer" 
		"command" "OpenServerBrowser"
		"subimage" "glyph_multiplayer"
		"OnlyAtMenu" "1"
	}

	"ReplayBrowserButton"
	{
		"label" "#GameUI_GameMenu_ReplayDemos"
		"command" "engine replay_reloadbrowser"
		"subimage" "glyph_tv"
	}
	"VRModeButton"
	{
		"label" "#MMenu_VRMode_Activate"
		"command" "engine vr_toggle"
		"subimage" "glyph_vr"
		"OnlyWhenVREnabled" "1"
	}
	"TrainingButton"
	{
		"label" "#MMenu_PlayList_Training_Button"
		"command" "offlinepractice"
		"subimage" "glyph_practice"
		"OnlyAtMenu" "1"
	}
	
	"CharacterSetupButton"
	{
		"label" "#MMenu_CharacterSetup"
		"command" "engine open_charinfo"
		"subimage" "glyph_items"
	}

	// These buttons are only shown while in-game
	// and also are positioned by the .res file
	"CreateServerButton"
    {
	  "label"        "#MMenu_HostAGame"
	  "command"      "OpenCreateMultiplayerGameDialog"
	  "subimage"     "glyph_create"
	  "OnlyAtMenu"   "1"
    }
	
	"CallVoteButton"
	{
		"label"			""
		"command"		"callvote"
		"OnlyInGame"	"1"
		"subimage" "icon_checkbox"
		"tooltip" "#MMenu_CallVote"
	}
	
	"MutePlayersButton"
	{
		"label"			""
		"command"		"OpenPlayerListDialog"
		"OnlyInGame"	"1"
		"subimage" "glyph_muted"
		"tooltip" "#MMenu_MutePlayers"
	}
	
	"NewGameButton"
	{
	  "label"         "#MMenu_NewGame" 
		"command"       "find_game"
		"subimage"      "glyph_server"
		"OnlyInGame"	"1"
	}
	
	"NewGameBgButton"
	{
	    "label"         "#MMenu_NewGame"
		"command"       "0"
		"subimage"      "glyph_server"
		"OnlyInGame"    "1"
	}
	
	"playlist"
	{
	    "label"         ""
		"command"       "0"
		"subimage"      "0"
		"OnlyAtMenu"    "1"
	}
}