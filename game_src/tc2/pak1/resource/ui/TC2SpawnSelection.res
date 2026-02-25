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
		"labelText"			"Select your spawn location:"
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
		"wide"				"460"
		"tall"				"300"
		"visible"			"1"
		"enabled"			"1"
		"paintbackground"	"1"
		"bgcolor_override"	"0 0 0 180"
	}
	
	"overview"
	{
		"ControlName"		"CMapOverview"
		"fieldName"			"overview"
		"xpos"				"20"
		"ypos"				"40"
		"zpos"				"-1"
		"wide"				"460"
		"tall"				"460"
		"visible"			"1"
		"enabled"			"1"
		"paintbackground"	"1"
	}
	
	"ConfirmButton"
	{
		"ControlName"		"Button"
		"fieldName"			"ConfirmButton"
		"xpos"				"270"
		"ypos"				"360"
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
		"xpos"				"380"
		"ypos"				"360"
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
