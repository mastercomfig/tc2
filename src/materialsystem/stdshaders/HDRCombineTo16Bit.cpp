//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"
#include "common_hlsl_cpp_consts.h"
#include "HDRCombineTo16Bit_vs30.inc"
#include "HDRCombineTo16Bit_ps30.inc"
#include "convar.h"

BEGIN_VS_SHADER_FLAGS( HDRCombineTo16Bit, "Help for HDRCombineTo16Bit", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SOURCEMRTRENDERTARGET, SHADER_PARAM_TYPE_TEXTURE, "", "" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadTexture( SOURCEMRTRENDERTARGET );
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if (!g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			Assert( 0 );
			return "Wireframe";
		}
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableAlphaWrites( false );
			pShaderShadow->EnableDepthTest( false );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			DECLARE_STATIC_VERTEX_SHADER( hdrcombineto16bit_vs30 );
			SET_STATIC_VERTEX_SHADER( hdrcombineto16bit_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( hdrcombineto16bit_ps30 );
			SET_STATIC_PIXEL_SHADER( hdrcombineto16bit_ps30 );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, SOURCEMRTRENDERTARGET, -1 );
			DECLARE_DYNAMIC_VERTEX_SHADER( hdrcombineto16bit_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( hdrcombineto16bit_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( hdrcombineto16bit_ps30 );
			SET_DYNAMIC_PIXEL_SHADER( hdrcombineto16bit_ps30 );
		}
		Draw();
	}
END_SHADER
