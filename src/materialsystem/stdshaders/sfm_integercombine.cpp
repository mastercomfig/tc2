//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "sfm_combine_vs30.inc"
#include "sfm_integercombine_ps30.inc"

BEGIN_VS_SHADER_FLAGS( sfm_integercombine_shader, "Help for SFM integer HDR combine pass", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		// Original full resolution texture
		SHADER_PARAM( ORIGINALTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )

		// Blurred quarter-resolution texture
		SHADER_PARAM( BLURREDTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )

		// How much bloom gets added in
		SHADER_PARAM( BLOOMAMOUNT, SHADER_PARAM_TYPE_VEC4, "", "" )

	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadTexture( ORIGINALTEXTURE );
		LoadTexture( BLURREDTEXTURE );
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
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
			pShaderShadow->EnableDepthTest( false );
			pShaderShadow->EnableAlphaWrites( false );
			pShaderShadow->EnableBlending( false );
			pShaderShadow->EnableCulling( false );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 2, 0, 0 ); // Two texture coordinates (first for high res, second for low res)

			DECLARE_STATIC_VERTEX_SHADER( sfm_combine_vs30 );
			SET_STATIC_VERTEX_SHADER( sfm_combine_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( sfm_integercombine_ps30 );
			SET_STATIC_PIXEL_SHADER( sfm_integercombine_ps30 );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, ORIGINALTEXTURE, -1 );
			BindTexture( SHADER_SAMPLER1, BLURREDTEXTURE, -1 );

			SetPixelShaderConstant( 0, BLOOMAMOUNT );

			DECLARE_DYNAMIC_VERTEX_SHADER( sfm_combine_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( sfm_combine_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( sfm_integercombine_ps30 );
			SET_DYNAMIC_PIXEL_SHADER( sfm_integercombine_ps30 );
		}
		Draw();
	}
END_SHADER
