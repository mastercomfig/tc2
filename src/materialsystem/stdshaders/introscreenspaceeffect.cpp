//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "screenspaceeffect_vs30.inc"
#include "IntroScreenSpaceEffect_ps30.inc"

BEGIN_VS_SHADER_FLAGS( IntroScreenSpaceEffect, "Help for IntroScreenSpaceEffect", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( MODE, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENABLESRGB, SHADER_PARAM_TYPE_BOOL, "0", "" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE );

		if ( !params[ENABLESRGB]->IsDefined() )
		{
			params[ENABLESRGB]->SetIntValue( 0 );
		}
	}

	SHADER_INIT
	{
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if ( IsPC() && ( g_pHardwareConfig->GetDXSupportLevel() < 90 || g_pHardwareConfig->PreferReducedFillrate() ) )
			return "IntroScreenSpaceEffect_dx80";
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

			if ( params[ENABLESRGB]->GetIntValue() )
			{
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );
				pShaderShadow->EnableSRGBWrite( true );
			}

			int fmt = VERTEX_POSITION;

			const bool bHasVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );
			if ( bHasVertexColor )
			{
				fmt |= VERTEX_COLOR;
			}

			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, bHasVertexColor );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( introscreenspaceeffect_ps30 );
			SET_STATIC_PIXEL_SHADER( introscreenspaceeffect_ps30 );

			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
		}
		DYNAMIC_STATE
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 );
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_1 );
			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( introscreenspaceeffect_ps30 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( MODE, params[MODE]->GetIntValue() );
			SET_DYNAMIC_PIXEL_SHADER( introscreenspaceeffect_ps30 );

			SetPixelShaderConstant( 0, ALPHA );
		}
		Draw();
	}
END_SHADER
