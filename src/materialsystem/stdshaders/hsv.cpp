//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "screenspaceeffect_vs30.inc"
#include "hsv_ps30.inc"

BEGIN_VS_SHADER_FLAGS( HSV, "Help for HSV", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE );
	}

	SHADER_INIT
	{
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if (g_pHardwareConfig->GetDXSupportLevel() < 90)
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
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, false );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( hsv_ps30 );
			SET_STATIC_PIXEL_SHADER( hsv_ps30 );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 );
			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs30 );
		}
		Draw();
	}
END_SHADER
