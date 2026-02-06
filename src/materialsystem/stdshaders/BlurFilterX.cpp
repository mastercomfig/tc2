//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "BlurFilter_vs30.inc"
#include "BlurFilter_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER_FLAGS( BlurFilterX, "Help for BlurFilterX", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if( params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE );
		}
	}
	
	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "BlurFilterX_DX80";
		}
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );

			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, false );
			pShaderShadow->EnableSRGBWrite( false );

			// Pre-cache shaders
			DECLARE_STATIC_VERTEX_SHADER( blurfilter_vs30 );
			SET_STATIC_VERTEX_SHADER( blurfilter_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( blurfilter_ps30 );
			SET_STATIC_PIXEL_SHADER( blurfilter_ps30 );

			if ( IS_FLAG_SET( MATERIAL_VAR_ADDITIVE ) )
				EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, -1 );

			float v[4];

			// The temp buffer is 1/4 back buffer size
			ITexture *src_texture = params[BASETEXTURE]->GetTextureValue();
			int width = src_texture->GetActualWidth();
			float dX = 1.0f / width;

			// Tap offsets
			v[0] = 1.3366f * dX;
			v[1] = 0.0f;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, v, 1 );
			v[0] = 3.4295f * dX;
			v[1] = 0.0f;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, v, 1 );
			v[0] = 5.4264f * dX;
			v[1] = 0.0f;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, v, 1 );

			v[0] = 7.4359f * dX;
			v[1] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( 0, v, 1 );
			v[0] = 9.4436f * dX;
			v[1] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( 1, v, 1 );
			v[0] = 11.4401f * dX;
			v[1] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( 2, v, 1 );
			v[0] = v[1] = v[2] = v[3] = 1.0;
			pShaderAPI->SetPixelShaderConstant( 3, v, 1 );

			pShaderAPI->SetVertexShaderIndex( 0 );

			DECLARE_DYNAMIC_PIXEL_SHADER( blurfilter_ps30 );
			SET_DYNAMIC_PIXEL_SHADER( blurfilter_ps30 );
		}
		Draw();
	}
END_SHADER
