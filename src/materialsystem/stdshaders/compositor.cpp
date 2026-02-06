//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

#include "compositor_ps30.inc"
#include "materialsystem/combineoperations.h"

#include "tier0/memdbgon.h"

struct CompositorInfo_t
{
	static const int cTextureCount = 4;
	static const int cSelectorCount = 16;

	CompositorInfo_t( ) { memset( this, -1, sizeof( *this ) ); }

	int m_nCombineMode;
	int m_nTextureInputCount;
	int m_nTexTransform[ cTextureCount ];
	int m_nTexAdjustLevels[ cTextureCount ];
	int m_nSrcTexture[ cTextureCount ];
	int m_nSelector[ cSelectorCount ];

	int m_nTexturesPerPass;
	int m_bDebug;
};


static void DrawCompositorStage_ps30( CBaseVSShader *pShader, IMaterialVar **params, IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression, const CompositorInfo_t &info );

BEGIN_VS_SHADER( Compositor, "Help for Compositor" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( COMBINE_MODE, SHADER_PARAM_TYPE_INTEGER, "", "Which mode should the combiner operate in? 0: Mul, 1: Add, 2: Lerp, 3: Select" )
		SHADER_PARAM( TEXTUREINPUTCOUNT, SHADER_PARAM_TYPE_INTEGER, "", "" )

		SHADER_PARAM( TEXTRANSFORM0, SHADER_PARAM_TYPE_MATRIX4X2, "", "" )
		SHADER_PARAM( TEXTRANSFORM1, SHADER_PARAM_TYPE_MATRIX4X2, "", "" )
		SHADER_PARAM( TEXTRANSFORM2, SHADER_PARAM_TYPE_MATRIX4X2, "", "" )
		SHADER_PARAM( TEXTRANSFORM3, SHADER_PARAM_TYPE_MATRIX4X2, "", "" )

		SHADER_PARAM( TEXADJUSTLEVELS0, SHADER_PARAM_TYPE_VEC3, "", "" )
		SHADER_PARAM( TEXADJUSTLEVELS1, SHADER_PARAM_TYPE_VEC3, "", "" )
		SHADER_PARAM( TEXADJUSTLEVELS2, SHADER_PARAM_TYPE_VEC3, "", "" )
		SHADER_PARAM( TEXADJUSTLEVELS3, SHADER_PARAM_TYPE_VEC3, "", "" )

		SHADER_PARAM( SRCTEXTURE0, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( SRCTEXTURE1, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( SRCTEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( SRCTEXTURE3, SHADER_PARAM_TYPE_TEXTURE, "", "" )

		SHADER_PARAM( SELECTOR0,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR1,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR2,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR3,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR4,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR5,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR6,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR7,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR8,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR9,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR10, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR11,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR12,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR13,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR14,  SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( SELECTOR15,  SHADER_PARAM_TYPE_INTEGER, "", "" )

		SHADER_PARAM( DEBUG_MODE, SHADER_PARAM_TYPE_BOOL, "", "" )
	END_SHADER_PARAMS

	void SetupVarsCompositorInfo( CompositorInfo_t* pOutInfo )
	{
		Assert( pOutInfo != NULL );

		( *pOutInfo ).m_nCombineMode			= COMBINE_MODE;
		( *pOutInfo ).m_nTextureInputCount		= TEXTUREINPUTCOUNT;
		( *pOutInfo ).m_nTexTransform[ 0 ]		= TEXTRANSFORM0;
		( *pOutInfo ).m_nTexTransform[ 1 ]		= TEXTRANSFORM1;
		( *pOutInfo ).m_nTexTransform[ 2 ]		= TEXTRANSFORM2;
		( *pOutInfo ).m_nTexTransform[ 3 ]		= TEXTRANSFORM3;
		( *pOutInfo ).m_nTexAdjustLevels[ 0 ]	= TEXADJUSTLEVELS0;
		( *pOutInfo ).m_nTexAdjustLevels[ 1 ]	= TEXADJUSTLEVELS1;
		( *pOutInfo ).m_nTexAdjustLevels[ 2 ]	= TEXADJUSTLEVELS2;
		( *pOutInfo ).m_nTexAdjustLevels[ 3 ]	= TEXADJUSTLEVELS3;
		( *pOutInfo ).m_nSrcTexture[ 0 ]		= SRCTEXTURE0;
		( *pOutInfo ).m_nSrcTexture[ 1 ]		= SRCTEXTURE1;
		( *pOutInfo ).m_nSrcTexture[ 2 ]		= SRCTEXTURE2;
		( *pOutInfo ).m_nSrcTexture[ 3 ]		= SRCTEXTURE3;
		( *pOutInfo ).m_nSelector[  0 ]			=  SELECTOR0;
		( *pOutInfo ).m_nSelector[  1 ]			=  SELECTOR1;
		( *pOutInfo ).m_nSelector[  2 ]			=  SELECTOR2;
		( *pOutInfo ).m_nSelector[  3 ]			=  SELECTOR3;
		( *pOutInfo ).m_nSelector[  4 ]			=  SELECTOR4;
		( *pOutInfo ).m_nSelector[  5 ]			=  SELECTOR5;
		( *pOutInfo ).m_nSelector[  6 ]			=  SELECTOR6;
		( *pOutInfo ).m_nSelector[  7 ]			=  SELECTOR7;
		( *pOutInfo ).m_nSelector[  8 ]			=  SELECTOR8;
		( *pOutInfo ).m_nSelector[  9 ]			=  SELECTOR9;
		( *pOutInfo ).m_nSelector[ 10 ]			= SELECTOR10;
		( *pOutInfo ).m_nSelector[ 11 ]			= SELECTOR11;
		( *pOutInfo ).m_nSelector[ 12 ]			= SELECTOR12;
		( *pOutInfo ).m_nSelector[ 13 ]			= SELECTOR13;
		( *pOutInfo ).m_nSelector[ 14 ]			= SELECTOR14;
		( *pOutInfo ).m_nSelector[ 15 ]			= SELECTOR15;
		( *pOutInfo ).m_bDebug					= DEBUG_MODE;
		( *pOutInfo).m_nTexturesPerPass = 4;
	}


	SHADER_INIT
	{
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if ( !g_pHardwareConfig->SupportsVertexAndPixelShaders() )
		{
			Assert( 0 );
			return "Wireframe";
		}
		return 0;
	}

	SHADER_DRAW
	{
		CompositorInfo_t info;
		SetupVarsCompositorInfo( &info );

		DrawCompositorStage_ps30( this, params, pShaderShadow, pShaderAPI, vertexCompression, info );
	}
END_SHADER

static void DrawCompositorStage_common( CBaseVSShader *pShader, IMaterialVar **params, IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression, const CompositorInfo_t &info )
{
	int nCombineMode = params[ info.m_nCombineMode ]->GetIntValue();

	SHADOW_STATE
	{
		pShader->SetInitialShadowState();

		// It's a quad, don't cull me bro.
		pShaderShadow->EnableCulling( false );
		pShaderShadow->EnablePolyOffset( SHADER_POLYOFFSET_DISABLE );

		pShaderShadow->EnableDepthWrites( false );
		pShaderShadow->EnableDepthTest( false );

		// In 2.0b shaders, we use the skin shader and alpha carries specular mask information--
		// so we need to write it
		pShaderShadow->EnableAlphaWrites( true );

		pShaderShadow->EnableSRGBWrite( true );

		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, nCombineMode != ECO_Select );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );

		int fmt = VERTEX_POSITION;
		pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

		pShaderShadow->SetVertexShader( "compositor_vs30", 0 );
	}

	DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();

		pShaderAPI->SetVertexShaderIndex( 0 );

		int textureCount = GetIntParam( info.m_nTextureInputCount, params );
		int textureCountThisPass = Min( textureCount, info.m_nTexturesPerPass );

		float f4TextureCountThisPass[] = { ( float ) textureCountThisPass, 0.0f, 0.0f, 0.0f };
		Assert( ARRAYSIZE( f4TextureCountThisPass ) == 4 );

		pShaderAPI->SetPixelShaderConstant( 6, f4TextureCountThisPass );

		if ( textureCountThisPass > 0 ) pShader->BindTexture( SHADER_SAMPLER0, info.m_nSrcTexture[0] );
		if ( textureCountThisPass > 0 ) pShader->SetVertexShaderMatrix2x4( 2, info.m_nTexTransform[0] );

		if ( nCombineMode == ECO_Select )
		{
			static const float cFac = 1.0f / 16.0f;

			float selectors[] =
			{
				cFac * GetIntParam( info.m_nSelector[  0 ], params ),
				cFac * GetIntParam( info.m_nSelector[  1 ], params ),
				cFac * GetIntParam( info.m_nSelector[  2 ], params ),
				cFac * GetIntParam( info.m_nSelector[  3 ], params ),
				cFac * GetIntParam( info.m_nSelector[  4 ], params ),
				cFac * GetIntParam( info.m_nSelector[  5 ], params ),
				cFac * GetIntParam( info.m_nSelector[  6 ], params ),
				cFac * GetIntParam( info.m_nSelector[  7 ], params ),
				cFac * GetIntParam( info.m_nSelector[  8 ], params ),
				cFac * GetIntParam( info.m_nSelector[  9 ], params ),
				cFac * GetIntParam( info.m_nSelector[ 10 ], params ),
				cFac * GetIntParam( info.m_nSelector[ 11 ], params ),
				cFac * GetIntParam( info.m_nSelector[ 12 ], params ),
				cFac * GetIntParam( info.m_nSelector[ 13 ], params ),
				cFac * GetIntParam( info.m_nSelector[ 14 ], params ),
				cFac * GetIntParam( info.m_nSelector[ 15 ], params )
			};

			pShaderAPI->SetPixelShaderConstant( 7, selectors, 4 );
		}
	}
}

static void DrawCompositorStage_ps30( CBaseVSShader *pShader, IMaterialVar **params, IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression, const CompositorInfo_t &info )
{
	int nCombineMode = params[ info.m_nCombineMode ]->GetIntValue();

	DrawCompositorStage_common( pShader, params, pShaderShadow, pShaderAPI, vertexCompression, info );

	SHADOW_STATE
	{
		pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER2, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER3, true );

		DECLARE_STATIC_PIXEL_SHADER( compositor_ps30 );
		SET_STATIC_PIXEL_SHADER_COMBO( COMBINE_MODE, nCombineMode );
		SET_STATIC_PIXEL_SHADER( compositor_ps30 );

		pShader->Draw();
	}

	DYNAMIC_STATE
	{
		int textureCount = GetIntParam( info.m_nTextureInputCount, params );
		int textureCountThisPass = Min( textureCount, info.m_nTexturesPerPass );
		Assert( textureCount > 0 ); // Valid, but would be really weird. 
		Assert( textureCountThisPass > 0 ); // That's bogus

		// Slot 0 is mostly already handled by common 
		if ( textureCountThisPass > 0 ) pShader->SetPixelShaderConstant( 2, info.m_nTexAdjustLevels[ 0 ] );

		if ( textureCountThisPass > 1 ) pShader->BindTexture( SHADER_SAMPLER1,   info.m_nSrcTexture[ 1 ]  );
		if ( textureCountThisPass > 1 ) pShader->SetVertexShaderMatrix2x4( 4,  info.m_nTexTransform[ 1 ] );
		if ( textureCountThisPass > 1 ) pShader->SetPixelShaderConstant( 3, info.m_nTexAdjustLevels[ 1 ] );

		if ( textureCountThisPass > 2 ) pShader->BindTexture( SHADER_SAMPLER2,   info.m_nSrcTexture[ 2 ] );
		if ( textureCountThisPass > 2 ) pShader->SetVertexShaderMatrix2x4( 6,  info.m_nTexTransform[ 2 ] );
		if ( textureCountThisPass > 2 ) pShader->SetPixelShaderConstant( 4, info.m_nTexAdjustLevels[ 2 ] );

		if ( textureCountThisPass > 3 ) pShader->BindTexture( SHADER_SAMPLER3,   info.m_nSrcTexture[ 3 ] );
		if ( textureCountThisPass > 3 ) pShader->SetVertexShaderMatrix2x4( 8,  info.m_nTexTransform[ 3 ] );
		if ( textureCountThisPass > 3 ) pShader->SetPixelShaderConstant( 5, info.m_nTexAdjustLevels[ 3 ] );
			
		DECLARE_DYNAMIC_PIXEL_SHADER( compositor_ps30 );
		// TODO(mcoms): stickers?
		SET_DYNAMIC_PIXEL_SHADER_COMBO( DEBUG_MODE, 0 );
		SET_DYNAMIC_PIXEL_SHADER( compositor_ps30 );

		pShader->Draw();
	}
}

