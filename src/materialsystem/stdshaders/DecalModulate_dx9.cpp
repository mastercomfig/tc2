//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "cpp_shader_constant_register_map.h"

#include "vertexlit_and_unlit_generic_vs30.inc"
#include "decalmodulate_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( DecalModulate, DecalModulate_DX9 )

extern ConVar r_flashlight_version2;

BEGIN_VS_SHADER( DecalModulate_dx9, 
			  "Help for DecalModulate_dx9" )
			  
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS
	
	SHADER_FALLBACK
	{
		if (g_pHardwareConfig->GetDXSupportLevel() < 90)
			return "DecalModulate_DX6";
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );

		const bool bFastVertexTextures = g_pHardwareConfig->HasFastVertexTextures();
		if ( bFastVertexTextures )
		{
			// The vertex shader uses the vertex id stream
			SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );
			SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
		}
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableAlphaTest( true );
			pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 0.0f );
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DECAL );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			// Be sure not to write to dest alpha
			pShaderShadow->EnableAlphaWrites( false );

			// SRGB conversions hose the blend on some hardware, so keep we everything in gamma space
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, false );
			pShaderShadow->EnableSRGBWrite( false );

			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR );
			pShaderShadow->DisableFogGammaCorrection( true ); //fog should stay exactly middle grey
			FogToGrey();

			const bool bFastVertexTextures = g_pHardwareConfig->HasFastVertexTextures();

			DECLARE_STATIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR,  false );
			SET_STATIC_VERTEX_SHADER_COMBO( CUBEMAP,  false );
			SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT,  false );
			SET_STATIC_VERTEX_SHADER_COMBO( FLASHLIGHT,  false );
			SET_STATIC_VERTEX_SHADER_COMBO( SEAMLESS_BASE,  false );
			SET_STATIC_VERTEX_SHADER_COMBO( SEAMLESS_DETAIL,  false );
			SET_STATIC_VERTEX_SHADER_COMBO( SEPARATE_DETAIL_UVS, false );
			SET_STATIC_VERTEX_SHADER_COMBO( DECAL, bFastVertexTextures );
			SET_STATIC_VERTEX_SHADER_COMBO( DONT_GAMMA_CONVERT_VERTEX_COLOR, 0 );
			SET_STATIC_VERTEX_SHADER_COMBO( TREESWAY, 0 );
			SET_STATIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( decalmodulate_ps30 );
			SET_STATIC_PIXEL_SHADER( decalmodulate_ps30 );

			// Set stream format (note that this shader supports compression)
			unsigned int flags = VERTEX_POSITION | VERTEX_FORMAT_COMPRESSED;
#ifndef _X360
			// The VS30 shader offsets decals along the normal (for morphed geom)
			flags |= bFastVertexTextures ? VERTEX_NORMAL : 0;
#endif
			int pTexCoordDim[3] = { 2, 0, 3 };
			int nTexCoordCount = 1;
			int userDataSize = 0;

#ifndef _X360
			if ( bFastVertexTextures )
			{
				nTexCoordCount = 3;
			}
#endif

			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, pTexCoordDim, userDataSize );
		}
		DYNAMIC_STATE
		{
			if ( pShaderAPI->InFlashlightMode() && ( !IsX360() && ( r_flashlight_version2.GetInt() == 0 ) ) )
			{
				// Don't draw anything for the flashlight pass
				Draw( false );
				return;
			}

			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

			// Set an identity base texture transformation
			Vector4D transformation[2];
			transformation[0].Init( 1.0f, 0.0f, 0.0f, 0.0f );
			transformation[1].Init( 0.0f, 1.0f, 0.0f, 0.0f );
		 	pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, transformation[0].Base(), 2 ); 

			MaterialFogMode_t fogType = s_pShaderAPI->GetSceneFogMode();
			int fogIndex = ( fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z ) ? 1 : 0;

			pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );					

			float vEyePos_SpecExponent[4];
			pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
			vEyePos_SpecExponent[3] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

			const bool bFastVertexTextures = g_pHardwareConfig->HasFastVertexTextures();
			if ( bFastVertexTextures )
			{
				SetHWMorphVertexShaderState( VERTEX_SHADER_SHADER_SPECIFIC_CONST_6, VERTEX_SHADER_SHADER_SPECIFIC_CONST_7, SHADER_VERTEXTEXTURE_SAMPLER0 );
			}

			DECLARE_DYNAMIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs30 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DYNAMIC_LIGHT, 0 );	// Use simplest possible vertex lighting, since ps is so simple
			SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT_VERTEX, 0 );		//
			SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT_LIGHTMAP, 0);		//
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, fogIndex );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( LIGHTING_PREVIEW, 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( MORPHING, bFastVertexTextures && pShaderAPI->IsHWMorphingEnabled() );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( vertexlit_and_unlit_generic_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( decalmodulate_ps30 );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo1( true ) );
			SET_DYNAMIC_PIXEL_SHADER( decalmodulate_ps30 );

			if ( bFastVertexTextures )
			{
				bool bUnusedTexCoords[3] = { false, false, !pShaderAPI->IsHWMorphingEnabled() };
				pShaderAPI->MarkUnusedVertexFields( 0, 3, bUnusedTexCoords );
			}
		}
		Draw( );
	}
END_SHADER
