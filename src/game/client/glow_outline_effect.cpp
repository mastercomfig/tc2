//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Functionality to render a glowing outline around client renderable objects.
//
//===============================================================================

#include "cbase.h"
#include "glow_outline_effect.h"
#include "model_types.h"
#include "shaderapi/ishaderapi.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "view_shared.h"
#include "viewpostprocess.h"
#include "cdll_client_int.h"

static ITexture* s_pRtGlowColor = NULL;
static ITexture* s_pRtGameSceneBackup = NULL;

#define FULL_FRAME_TEXTURE "_rt_FullFrameFB"

#ifdef GLOWS_ENABLE

// If you've added IMatRenderContext::OverrideDepthFunc (see ::DrawGlowOccluded below),
// then you can enable this and have single-pass glows for "glow when occluded" outlines.
// ***PLEASE*** increment MATERIAL_SYSTEM_INTERFACE_VERSION when you add this!
#define ADDED_OVERRIDE_DEPTH_FUNC 0

ConVar glow_outline_effect_enable( "glow_outline_effect_enable", "1", FCVAR_ARCHIVE, "Enable entity outline glow effects." );
ConVar glow_outline_width( "glow_outline_width", "2.0f", FCVAR_CHEAT, "Width of glow outline effect in screen space." );

extern bool g_bDumpRenderTargets; // in viewpostprocess.cpp

CGlowObjectManager g_GlowObjectManager;

struct ShaderStencilState_t
{
	bool m_bEnable;
	StencilOperation_t m_FailOp;
	StencilOperation_t m_ZFailOp;
	StencilOperation_t m_PassOp;
	StencilComparisonFunction_t m_CompareFunc;
	int m_nReferenceValue;
	uint32 m_nTestMask;
	uint32 m_nWriteMask;

	ShaderStencilState_t()
	{
		m_bEnable = false;
		m_PassOp = m_FailOp = m_ZFailOp = STENCILOPERATION_KEEP;
		m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
		m_nReferenceValue = 0;
		m_nTestMask = m_nWriteMask = 0xFFFFFFFF;
	}

	void SetStencilState( CMatRenderContextPtr &pRenderContext  )
	{
		pRenderContext->SetStencilEnable( m_bEnable );
		if ( m_bEnable )
		{
			pRenderContext->SetStencilFailOperation( m_FailOp );
			pRenderContext->SetStencilZFailOperation( m_ZFailOp );
			pRenderContext->SetStencilPassOperation( m_PassOp );
			pRenderContext->SetStencilCompareFunction( m_CompareFunc );
			pRenderContext->SetStencilReferenceValue( m_nReferenceValue );
			pRenderContext->SetStencilTestMask( m_nTestMask );
			pRenderContext->SetStencilWriteMask( m_nWriteMask );
		}
	}
};

void CGlowObjectManager::RenderGlowEffects( const CViewSetup *pSetup, int nSplitScreenSlot )
{
	if ( g_pMaterialSystemHardwareConfig->SupportsPixelShaders_2_0() )
	{
		if ( glow_outline_effect_enable.GetBool() )
		{
			CMatRenderContextPtr pRenderContext( materials );

			int nX, nY, nWidth, nHeight;
			pRenderContext->GetViewport( nX, nY, nWidth, nHeight );

			PIXEvent _pixEvent( pRenderContext, "EntityGlowEffects" );
			ApplyEntityGlowEffects( pSetup, nSplitScreenSlot, pRenderContext, glow_outline_width.GetFloat(), nX, nY, nWidth, nHeight );
		}
	}
}

void CGlowObjectManager::DrawGlowAlways(int nSplitScreenSlot, CMatRenderContextPtr& pRenderContext)
{
	ShaderStencilState_t stencilState;
	stencilState.m_bEnable = true;
	stencilState.m_nReferenceValue = 0x80;
	stencilState.m_nTestMask = 0x80;
	stencilState.m_nWriteMask = 0x80;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
	stencilState.m_PassOp = STENCILOPERATION_REPLACE;
	stencilState.m_FailOp = STENCILOPERATION_KEEP;
	stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;
	stencilState.SetStencilState(pRenderContext);

	pRenderContext->OverrideDepthEnable(false, false);
	render->SetBlend(1.0f);
	for (int i = 0; i < m_GlowObjectDefinitions.Count(); i++)
	{
		auto& current = m_GlowObjectDefinitions[i];
		if (current.IsUnused() || !current.ShouldDraw(nSplitScreenSlot) || !current.m_bRenderWhenOccluded || !current.m_bRenderWhenUnoccluded)
			continue;

		Vector vGlowColor = current.m_vGlowColor * current.m_flGlowAlpha;
		render->SetColorModulation(vGlowColor.Base()); // This only sets rgb, not alpha
		IMaterial* pMatGlowColor = materials->FindMaterial( "dev/glow_color", TEXTURE_GROUP_OTHER, true );
		current.DrawModel( pMatGlowColor );
	}
}

void CGlowObjectManager::DrawGlowOccluded(int nSplitScreenSlot, CMatRenderContextPtr& pRenderContext)
{
#if ADDED_OVERRIDE_DEPTH_FUNC	// Enable this when the TF2 team has added IMatRenderContext::OverrideDepthFunc or similar.
	ShaderStencilState_t stencilState;
	stencilState.m_bEnable = true;
	stencilState.m_nReferenceValue = 1;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
	stencilState.m_PassOp = STENCILOPERATION_REPLACE;
	stencilState.m_FailOp = STENCILOPERATION_KEEP;
	stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;
	stencilState.SetStencilState(pRenderContext);

	pRenderContext->OverrideDepthEnable(true, false);

	// Not implemented, we need this feature to be able to do this in 1 pass. Otherwise,
	// we'd have to do 2 passes, 1st to mark on the stencil where the depth test failed,
	// 2nd to actually utilize that information and draw color there.
	pRenderContext->OverrideDepthFunc(true, SHADER_DEPTHFUNC_NEARER);

	for (int i = 0; i < m_GlowObjectDefinitions.Count(); i++)
	{
		auto& current = m_GlowObjectDefinitions[i];
		if (current.IsUnused() || !current.ShouldDraw(nSplitScreenSlot) || !current.m_bRenderWhenOccluded || current.m_bRenderWhenUnoccluded)
			continue;

		render->SetBlend(current.m_flGlowAlpha);
		Vector vGlowColor = current.m_vGlowColor * current.m_flGlowAlpha;
		render->SetColorModulation(&vGlowColor[0]); // This only sets rgb, not alpha
		IMaterial* pMatGlowColor = materials->FindMaterial( "dev/glow_color", TEXTURE_GROUP_OTHER, true );
		current.DrawModel( pMatGlowColor );
	}

	pRenderContext->OverrideDepthFunc(false, SHADER_DEPTHFUNC_NEAREROREQUAL);
#else	// 2-pass as a proof of concept so I can take a nice screenshot.	
	pRenderContext->OverrideDepthEnable(true, false);

	ShaderStencilState_t stencilState;
	stencilState.m_bEnable = true;
	stencilState.m_nReferenceValue = 0x80;
	stencilState.m_nTestMask = 0x80;
	stencilState.m_nWriteMask = 0x80;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
	stencilState.m_PassOp = STENCILOPERATION_REPLACE;
	stencilState.m_FailOp = STENCILOPERATION_KEEP;
	stencilState.m_ZFailOp = STENCILOPERATION_KEEP;
	stencilState.SetStencilState(pRenderContext);

	// Pass 1: Draw depthtest-passing pixels to the stencil buffer (writes 2)
	{
		render->SetBlend(0.0f);
		pRenderContext->OverrideAlphaWriteEnable(true, false);
		pRenderContext->OverrideColorWriteEnable(true, false);

		for (int i = 0; i < m_GlowObjectDefinitions.Count(); i++)
		{
			auto& current = m_GlowObjectDefinitions[i];
			if (current.IsUnused() || !current.ShouldDraw(nSplitScreenSlot) || !current.m_bRenderWhenOccluded || current.m_bRenderWhenUnoccluded)
				continue;
			IMaterial* pMatGlowColor = materials->FindMaterial( "dev/glow_color", TEXTURE_GROUP_OTHER, true );
			current.DrawModel( pMatGlowColor );
		}
	}

	pRenderContext->OverrideAlphaWriteEnable(false, true);
	pRenderContext->OverrideColorWriteEnable(false, true);

	pRenderContext->OverrideDepthEnable(false, false);

	stencilState.m_bEnable = true;
	stencilState.m_nReferenceValue = 0x80;
	stencilState.m_nTestMask = 0x80;
	stencilState.m_nWriteMask = 0x80;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_NOTEQUAL;
	stencilState.m_PassOp = STENCILOPERATION_REPLACE;
	stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;
	stencilState.m_FailOp = STENCILOPERATION_KEEP;
	stencilState.SetStencilState(pRenderContext);

	// Pass 2: Draw color unconditionally, but mask out pixels from first pass using stencil
	render->SetBlend(1.0f);
	for (int i = 0; i < m_GlowObjectDefinitions.Count(); i++)
	{
		auto& current = m_GlowObjectDefinitions[i];
		if (current.IsUnused() || !current.ShouldDraw(nSplitScreenSlot) || !current.m_bRenderWhenOccluded || current.m_bRenderWhenUnoccluded)
			continue;

		Vector vGlowColor = current.m_vGlowColor * current.m_flGlowAlpha;
		// tonemapping accurate color
		if ( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
		{
			Vector vScale = pRenderContext->GetToneMappingScaleLinear();
			if ( vScale.x > 0.00001f ) vGlowColor.x /= vScale.x;
			if ( vScale.y > 0.00001f ) vGlowColor.y /= vScale.y;
			if ( vScale.z > 0.00001f ) vGlowColor.z /= vScale.z;
		}
		render->SetColorModulation(vGlowColor.Base());
		IMaterial* pMatGlowColor = materials->FindMaterial( "dev/glow_color", TEXTURE_GROUP_OTHER, true );
		current.DrawModel( pMatGlowColor );
	}
#endif
}

void CGlowObjectManager::DrawGlowVisible(int nSplitScreenSlot, CMatRenderContextPtr& pRenderContext)
{
	ShaderStencilState_t stencilState;
	stencilState.m_bEnable = true;
	stencilState.m_nWriteMask = 0x80;
	stencilState.m_nTestMask = 0x80;
	stencilState.m_nReferenceValue = 0x80;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
	stencilState.m_PassOp = STENCILOPERATION_REPLACE;
	stencilState.m_FailOp = STENCILOPERATION_KEEP;
	stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;

	stencilState.SetStencilState(pRenderContext);

	pRenderContext->OverrideDepthEnable(true, false);
	render->SetBlend(1.0f);
	for (int i = 0; i < m_GlowObjectDefinitions.Count(); i++)
	{
		auto& current = m_GlowObjectDefinitions[i];
		if (current.IsUnused() || !current.ShouldDraw(nSplitScreenSlot) || current.m_bRenderWhenOccluded || !current.m_bRenderWhenUnoccluded)
			continue;

		Vector vGlowColor = current.m_vGlowColor * current.m_flGlowAlpha;
		// tonemapping accurate color
		if ( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
		{
			Vector vScale = pRenderContext->GetToneMappingScaleLinear();
			if ( vScale.x > 0.00001f ) vGlowColor.x /= vScale.x;
			if ( vScale.y > 0.00001f ) vGlowColor.y /= vScale.y;
			if ( vScale.z > 0.00001f ) vGlowColor.z /= vScale.z;
		}
		render->SetColorModulation(vGlowColor.Base()); // This only sets rgb, not alpha
		IMaterial* pMatGlowColor = materials->FindMaterial( "dev/glow_color", TEXTURE_GROUP_OTHER, true );
		current.DrawModel( pMatGlowColor );
	}
}

void CGlowObjectManager::RenderGlowModels( const CViewSetup *pSetup, int nSplitScreenSlot, CMatRenderContextPtr &pRenderContext )
{
	//==========================================================================================//
	// This renders solid pixels with the correct coloring for each object that needs the glow.	//
	// After this function returns, this image will then be blurred and added into the frame	//
	// buffer with the objects stenciled out.													//
	//==========================================================================================//
	pRenderContext->PushRenderTargetAndViewport();

	// Save modulation color and blend
	Vector vOrigColor;
	render->GetColorModulation( vOrigColor.Base() );
	const float flOrigBlend = render->GetBlend();

	// avoid touching _rt_FullFrameFB which is used elsewhere
	if ( !s_pRtGlowColor || !s_pRtGameSceneBackup )
	{
		ImageFormat format = materials->GetBackBufferFormat();
		if ( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			format = IMAGE_FORMAT_RGBA16161616F;

		ITexture* pRtFullFrameFB = materials->FindTexture( FULL_FRAME_TEXTURE, TEXTURE_GROUP_RENDER_TARGET );

		materials->BeginRenderTargetAllocation();

		// we use RT_SIZE_NO_CHANGE here and the frame texture's actual size to avoid DX9 rect resize issues when copying
		// usually there's some 0.5 padding or whatever on the hardware and the engine naively isn't aware of this, so if you do a
		// CopyTextureToRenderTargetEx that isn't aware of the underlying issue, you'll cause a slight blur, which changes bloom
		s_pRtGlowColor = materials->CreateNamedRenderTargetTextureEx2(
			"_rt_GlowColor", pRtFullFrameFB->GetActualWidth(), pRtFullFrameFB->GetActualHeight(),
			RT_SIZE_NO_CHANGE, format,
			MATERIAL_RT_DEPTH_NONE, TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT, 0 );
		s_pRtGameSceneBackup = materials->CreateNamedRenderTargetTextureEx2(
			"_rt_GlowGameSceneBackup", pRtFullFrameFB->GetActualWidth(), pRtFullFrameFB->GetActualHeight(),
			RT_SIZE_NO_CHANGE, format,
			MATERIAL_RT_DEPTH_NONE, TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT, 0 );

		s_pRtGlowColor->AddRef();
		s_pRtGameSceneBackup->AddRef();

		materials->EndRenderTargetAllocation();
	}

	// copy our scene to our backup texture
	pRenderContext->CopyRenderTargetToTexture( s_pRtGameSceneBackup );

	// Clear backbuffer color, including alpha, to make sure this pass is isolated
	pRenderContext->ClearColor4ub( 0, 0, 0, 0 );
	// do NOT clear engine stencil, keep depth for testing
	pRenderContext->ClearBuffers( true, false, false );

	// Set override material for glow color
	IMaterial *pMatGlowColor = NULL;

	pMatGlowColor = materials->FindMaterial( "dev/glow_color", TEXTURE_GROUP_OTHER, true );
	modelrender->ForcedMaterialOverride( pMatGlowColor );
	
	// Don't write alpha
	pRenderContext->OverrideAlphaWriteEnable( true, false );

	ShaderStencilState_t stencilState;
	stencilState.m_bEnable = false;
	stencilState.m_nReferenceValue = 0;
	stencilState.m_nTestMask = 0xFF;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
	stencilState.m_PassOp = STENCILOPERATION_KEEP;
	stencilState.m_FailOp = STENCILOPERATION_KEEP;
	stencilState.m_ZFailOp = STENCILOPERATION_KEEP;

	stencilState.SetStencilState( pRenderContext );

	//==================//
	// Draw the objects //
	//==================//
	{
		// Draw "glow when visible" objects
		DrawGlowVisible(nSplitScreenSlot, pRenderContext);

		// Draw "glow when occluded" objects
		DrawGlowOccluded(nSplitScreenSlot, pRenderContext);

		// Draw "glow always" objects
		DrawGlowAlways(nSplitScreenSlot, pRenderContext);

	}

	if ( g_bDumpRenderTargets )
	{
		DumpTGAofRenderTarget( pSetup->width, pSetup->height, "GlowModels" );
	}

	// Restore modulation color and blend
	modelrender->ForcedMaterialOverride( NULL );
	render->SetColorModulation( vOrigColor.Base() );
	render->SetBlend( flOrigBlend );
	
	ShaderStencilState_t stencilStateDisable;
	stencilStateDisable.m_bEnable = false;
	stencilStateDisable.SetStencilState( pRenderContext );

	pRenderContext->OverrideDepthEnable( false, false );

	// Copy out the glow models to our texture
	pRenderContext->CopyRenderTargetToTexture( s_pRtGlowColor );

	// Re-enable writes here for when we copy our backup to the backbuffer
	pRenderContext->OverrideColorWriteEnable( false, false );
	pRenderContext->OverrideAlphaWriteEnable( false, false );

	// Copy with proper coordinates
	pRenderContext->CopyTextureToRenderTargetEx( 0, s_pRtGameSceneBackup, nullptr );

	pRenderContext->PopRenderTargetAndViewport();
}

void CGlowObjectManager::ApplyEntityGlowEffects( const CViewSetup *pSetup, int nSplitScreenSlot, CMatRenderContextPtr &pRenderContext, float flBloomScale, int x, int y, int w, int h )
{
	bool bAtLeastOneGlow = false;

	for ( int i = 0; i < m_GlowObjectDefinitions.Count(); ++ i )
	{
		if ( m_GlowObjectDefinitions[i].IsUnused() || !m_GlowObjectDefinitions[i].ShouldDraw( nSplitScreenSlot ) )
			continue;

		bAtLeastOneGlow = true;
		break;
	}

	// If there aren't any objects to glow, don't do all this stuff
	if ( !bAtLeastOneGlow )
		return;

	ShaderStencilState_t stencilStateDisable;
	stencilStateDisable.m_bEnable = false;

	//=============================================
	// Render the glow colors to _rt_FullFrameFB 
	//=============================================
	{
		PIXEvent pixEvent( pRenderContext, "RenderGlowModels" );
		RenderGlowModels( pSetup, nSplitScreenSlot, pRenderContext );
	}

	// Get viewport
	const int nSrcWidth = pSetup->width;
	const int nSrcHeight = pSetup->height;
	int nViewportX, nViewportY, nViewportWidth, nViewportHeight;
	pRenderContext->GetViewport( nViewportX, nViewportY, nViewportWidth, nViewportHeight );

	// Get material and texture pointers
	ITexture *pRtQuarterSize1 = materials->FindTexture( "_rt_SmallFB1", TEXTURE_GROUP_RENDER_TARGET );

	// Bloom glow models from _rt_FullFrameFB0 to backbuffer while stenciling out inside of models
	{
		//=======================================================================================================//
		// At this point, pRtQuarterSize0 is filled with the fully colored glow around everything as solid glowy //
		// blobs. Now we need to stencil out the original objects by only writing pixels that have no            //
		// stencil bits set in the range we care about.                                                          //
		//=======================================================================================================//
		IMaterial *pMatHaloAddToScreen = materials->FindMaterial( "dev/halo_add_to_screen", TEXTURE_GROUP_OTHER, true );
		
		// We use a created _rt_GlowColor as our texture, not whatever the material is set to
		IMaterialVar* pBaseTexVar = pMatHaloAddToScreen->FindVar( "$basetexture", NULL );
		if ( pBaseTexVar && s_pRtGlowColor )
		{
			pBaseTexVar->SetTextureValue( s_pRtGlowColor );
		}

		// Do not fade the glows out at all (weight = 1.0)
		IMaterialVar *pDimVar = pMatHaloAddToScreen->FindVar( "$C0_X", NULL );
		pDimVar->SetFloatValue( flBloomScale );

		// Set stencil state
		ShaderStencilState_t stencilState;
		stencilState.m_bEnable = true;
		stencilState.m_nWriteMask = 0x0; // We're not changing stencil
		stencilState.m_nTestMask = 0x80;
		stencilState.m_nReferenceValue = 0x0;
		stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL;
		stencilState.m_PassOp = STENCILOPERATION_KEEP;
		stencilState.m_FailOp = STENCILOPERATION_KEEP;
		stencilState.m_ZFailOp = STENCILOPERATION_KEEP;
		stencilState.SetStencilState( pRenderContext );

		// Disable alpha overwriting when drawing
		pRenderContext->OverrideAlphaWriteEnable( true, false );

		// Draw quad
		pRenderContext->DrawScreenSpaceRectangle( pMatHaloAddToScreen, 0, 0, nViewportWidth, nViewportHeight,
			0.0f, -0.5f, nSrcWidth / 4 - 1, nSrcHeight / 4 - 1,
			pRtQuarterSize1->GetActualWidth(),
			pRtQuarterSize1->GetActualHeight() );

		// Re-enable state
		pRenderContext->OverrideAlphaWriteEnable( false, false );

		stencilStateDisable.SetStencilState( pRenderContext );
	}
}

void CGlowObjectManager::GlowObjectDefinition_t::DrawModel( IMaterial* pMatGlowColor )
{
	if ( m_hEntity.Get() )
	{
		modelrender->ForcedMaterialOverride( pMatGlowColor );
		m_hEntity->DrawModel( STUDIO_RENDER );
		C_BaseEntity *pAttachment = m_hEntity->FirstMoveChild();

		while ( pAttachment != NULL )
		{
			if ( !g_GlowObjectManager.HasGlowEffect( pAttachment ) && pAttachment->ShouldDraw() && pAttachment->CanGlow() )
			{
				modelrender->ForcedMaterialOverride( pMatGlowColor );
				pAttachment->DrawModel( STUDIO_RENDER );
			}
			pAttachment = pAttachment->NextMovePeer();
		}
	}
}

#endif // GLOWS_ENABLE
