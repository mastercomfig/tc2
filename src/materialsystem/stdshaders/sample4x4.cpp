//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "common_hlsl_cpp_consts.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER_FLAGS( Sample4x4, "Help for Sample4x4", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( PIXSHADER, SHADER_PARAM_TYPE_STRING, "sample4x4_ps30", "Name of the pixel shader to use" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}
	
	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableAlphaWrites( true );
			
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );
			
			pShaderShadow->SetVertexShader( "Downsample_vs30", 0 );

			char szBuf[256];
			RenameShaderToShaderModel30( params[PIXSHADER]->GetStringValue(), szBuf );
			pShaderShadow->SetPixelShader( szBuf, 0 );

// 			if ( IsAlphaModulating() )
// 			{
// 				pShaderShadow->EnableBlending( true );
// 				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA,
// 										  SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
// 			}
// 			else
// 			{
// 				pShaderShadow->EnableBlending( true );
// 				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA,
// 										  SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
// //				pShaderShadow->EnableBlending( false );
// 			}
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, -1 );
			ITexture *src_texture=params[BASETEXTURE]->GetTextureValue();

			int width=src_texture->GetActualWidth();
			int height=src_texture->GetActualHeight();

			float v[4];
			float dX = 1.0f / width;
			float dY = 1.0f / height;

			v[0] = -dX;
			v[1] = -dY;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, v, 1 );
			v[0] = -dX;
			v[1] = dY;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, v, 1 );
			v[0] = dX;
			v[1] = -dY;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, v, 1 );
			v[0] = dX;
			v[1] = dY;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, v, 1 );

			pShaderAPI->SetVertexShaderIndex( 0 );
			pShaderAPI->SetPixelShaderIndex( 0 );

			// store the ALPHA material var into c0
			v[0] = ALPHA;
			pShaderAPI->SetPixelShaderConstant( 0, v, 1 );
			
		}
		Draw();
	}
END_SHADER
