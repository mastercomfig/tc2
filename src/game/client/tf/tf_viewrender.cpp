//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Responsible for drawing the scene
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "tf_viewrender.h"
#include "viewpostprocess.h"
#include <game/client/iviewport.h>
#include "clienteffectprecachesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecachePostProcessingGlowEffects )
	CLIENTEFFECT_MATERIAL( "dev/glow_blur_x" )
	CLIENTEFFECT_MATERIAL( "dev/glow_blur_y" )
	CLIENTEFFECT_MATERIAL( "dev/glow_color" )
	CLIENTEFFECT_MATERIAL( "dev/glow_downsample" )
	CLIENTEFFECT_MATERIAL( "dev/halo_add_to_screen" )
CLIENTEFFECT_REGISTER_END()

static CTFViewRender g_ViewRender;

CTFViewRender::CTFViewRender()
{
	view = ( IViewRender * )this;
}

struct ConVarFlags
{
	const char *name;
	int flags;
	const char *optional_default;
};

ConVarFlags s_flaggedConVars[] =
{
	{ "r_screenfademinsize", FCVAR_CHEAT, "0" },
	{ "r_screenfademaxsize", FCVAR_CHEAT, "0" },
	{ "r_staticprop_lod", FCVAR_CHEAT, "-1" },
	{ "lod_TransitionDist", FCVAR_CHEAT, "800" },
	{ "cl_particle_retire_cost", FCVAR_CHEAT, "0" },
	{ "dsp_off", FCVAR_CHEAT, "0" },
	{ "r_drawropes", FCVAR_CHEAT, "1" },
	{ "mat_depthbias_decal", FCVAR_CHEAT, "-262144" },
	{ "mat_depthbias_normal", FCVAR_CHEAT, "0.0f" },
	{ "mat_slopescaledepthbias_decal", FCVAR_CHEAT, "-0.5" },
	{ "mat_slopescaledepthbias_normal", FCVAR_CHEAT, "0.0f" },
	{ "mat_softwarelighting", FCVAR_CHEAT, "0" },
	{ "r_dopixelvisibility", FCVAR_CHEAT, "1" },
	{ "r_shader_srgb", FCVAR_CHEAT, "0" },
	{ "mat_dxlevel", FCVAR_SPONLY | FCVAR_NOT_CONNECTED, NULL },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFViewRender::Init()
{
	for ( int i=0; i<ARRAYSIZE( s_flaggedConVars ); ++i )
	{
		ConVar *flaggedConVar = cvar->FindVar( s_flaggedConVars[i].name );
		if ( flaggedConVar )
		{
			flaggedConVar->AddFlags( s_flaggedConVars[i].flags );

			if ( s_flaggedConVars[i].optional_default )
			{
				flaggedConVar->SetDefault( s_flaggedConVars[i].optional_default );
				flaggedConVar->Revert();
			}
		}
	}

	BaseClass::Init();
}

//-----------------------------------------------------------------------------
// Purpose: Renders extra 2D effects in derived classes while the 2D view is on the stack
//-----------------------------------------------------------------------------
void CTFViewRender::Render2DEffectsPostHUD( const CViewSetup &viewTF )
{
	BaseClass::Render2DEffectsPostHUD( viewTF );

#if defined( _X360 )
	// if we're in the intro menus
	if ( gViewPortInterface->GetActivePanel() != NULL )
	{
		DoEnginePostProcessing( viewTF.x, viewTF.y, viewTF.width, viewTF.height, false, true );
	}
#endif //_X360
}

