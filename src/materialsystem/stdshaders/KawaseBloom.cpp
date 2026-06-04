//========= Copyright Valve Corporation, All rights reserved. ============//

#include "BaseVSShader.h"
#include "screenspaceeffect_vs30.inc"
#include "kawasebloom_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER_FLAGS( KawaseBloom, "Help for KawaseBloom", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DISTANCE, SHADER_PARAM_TYPE_FLOAT, "0.0", "Kawase Blur Distance" )
		SHADER_PARAM( BLOOMAMOUNT, SHADER_PARAM_TYPE_FLOAT, "1.0", "Bloom scale" )
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
			return "Wireframe";
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

			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, 0 );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( kawasebloom_ps30 );
			SET_STATIC_PIXEL_SHADER( kawasebloom_ps30 );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, -1 );

			float v[4];

			ITexture *src_texture = params[BASETEXTURE]->GetTextureValue();
			int width = src_texture->GetActualWidth();
			int height = src_texture->GetActualHeight();

			v[0] = params[DISTANCE]->GetFloatValue();
			v[1] = 1.0f / width;
			v[2] = 1.0f / height;
			if ( params[BLOOMAMOUNT]->IsDefined() )
			{
				v[3] = params[BLOOMAMOUNT]->GetFloatValue();
			}
			else
			{
				v[3] = 1.0f;
			}
			pShaderAPI->SetPixelShaderConstant( 0, v, 1 );

			float vTexelSizes[4] = { 0, 0, 0, 0 };
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, vTexelSizes, 1 );

			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( kawasebloom_ps30 );
			SET_DYNAMIC_PIXEL_SHADER( kawasebloom_ps30 );
		}
		Draw();
	}
END_SHADER
