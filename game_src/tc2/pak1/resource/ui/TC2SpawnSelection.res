"Resource/UI/TC2SpawnSelection.res"
{
	"TC2SpawnSelection"
	{
		"ControlName"		"Frame"
		"fieldName"			"TC2SpawnSelection"
		"xpos"				"c-250"
		"ypos"				"c-200"
		"wide"				"f0"
		"tall"				"f0"
		"visible"			"1"
		"enabled"			"1"
		"paintbackground"	"1"
	}
	
	"TitleLabel"
	{
		"ControlName"		"Label"
		"fieldName"			"TitleLabel"
		"xpos"				"20"
		"ypos"				"10"
		"wide"				"460"
		"tall"				"20"
		"visible"			"1"
		"enabled"			"1"
		"labelText"			"#TC2_SelectRedeploy"
		"textAlignment"		"west"
		"font"				"HudFontMediumBold"
		"fgcolor"			"TanLight"
	}
	
	"MapPanel"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"MapPanel"
		"xpos"				"20"
		"ypos"				"40"
		"wide"				"400"
		"tall"				"400"
		"visible"			"1"
		"enabled"			"1"
		"paintbackground"	"1"
	}
	
	"overview"
	{
		"ControlName"		"CMapOverview"
		"fieldName"			"overview"
		"xpos"				"20"
		"ypos"				"40"
		"zpos"				"-1"
		"wide"				"400"
		"tall"				"400"
		"visible"			"1"
		"enabled"			"1"
		"paintbackground"	"1"
	}
	
	"ConfirmButton"
	{
		"ControlName"		"Button"
		"fieldName"			"ConfirmButton"
		"xpos"				"190"
		"ypos"				"400"
		"wide"				"100"
		"tall"				"30"
		"visible"			"1"
		"enabled"			"1"
		"labelText"			"Spawn"
		"command"			"confirm"
		"textAlignment"		"center"
		"font"				"HudFontSmallBold"
	}
	
	"CancelButton"
	{
		"ControlName"		"Button"
		"fieldName"			"CancelButton"
		"xpos"				"300"
		"ypos"				"400"
		"wide"				"100"
		"tall"				"30"
		"visible"			"1"
		"enabled"			"1"
		"labelText"			"Cancel"
		"command"			"cancel"
		"textAlignment"		"center"
		"font"				"HudFontSmallBold"
	}
}
