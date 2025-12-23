//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Run procedural glint generation inner loop in pixel shader
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "shaderlib/cshader.h"

#include "eyeglint_vs30.inc"
#include "eyeglint_ps30.inc"

DEFINE_FALLBACK_SHADER( EyeGlint, EyeGlint_dx9 )
BEGIN_VS_SHADER( EyeGlint_dx9, "Help for EyeGlint" )

BEGIN_SHADER_PARAMS
END_SHADER_PARAMS

SHADER_INIT
{
}

SHADER_FALLBACK
{
	if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
	{
		return "Wireframe";
	}
	return 0;
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		pShaderShadow->EnableDepthWrites( false );

		pShaderShadow->EnableBlending( true );
		pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );		// Additive blending

		int pTexCoords[3] = { 2, 2, 3 };
		pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 3, pTexCoords, 0 );

		pShaderShadow->EnableCulling( false );

		pShaderShadow->EnableSRGBWrite( false ); // linear texture

		DECLARE_STATIC_VERTEX_SHADER( eyeglint_vs30 );
		SET_STATIC_VERTEX_SHADER( eyeglint_vs30 );

		DECLARE_STATIC_PIXEL_SHADER( eyeglint_ps30 );
		SET_STATIC_PIXEL_SHADER( eyeglint_ps30 );
	}

	DYNAMIC_STATE
	{
		DECLARE_DYNAMIC_VERTEX_SHADER( eyeglint_vs30 );
		SET_DYNAMIC_VERTEX_SHADER( eyeglint_vs30 );

		DECLARE_DYNAMIC_PIXEL_SHADER( eyeglint_ps30 );
		SET_DYNAMIC_PIXEL_SHADER( eyeglint_ps30 );
	}
	Draw();
}
END_SHADER
