//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "worldvertextransition_dx8_helper.h"
#include "BaseVSShader.h"

#include "WorldVertexTransition_vs30.inc"
#include "WorldVertexTransition_ps30.inc"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void InitParamsWorldVertexTransitionEditor_DX8( IMaterialVar** params, WorldVertexTransitionEditor_DX8_Vars_t &info )
{
	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
}

void InitWorldVertexTransitionEditor_DX8( CBaseVSShader *pShader, IMaterialVar** params, WorldVertexTransitionEditor_DX8_Vars_t &info )
{
	if ( params[info.m_nBaseTextureVar]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTextureVar );
	}

	if ( params[info.m_nBaseTexture2Var]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTexture2Var );
	}
}

void DrawWorldVertexTransitionEditor_DX8( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, WorldVertexTransitionEditor_DX8_Vars_t &info )
{
	SHADOW_STATE
	{
		// This is the dx8 worldcraft version (non-bumped always.. too bad)
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );

		int fmt = VERTEX_POSITION | VERTEX_COLOR;
		pShaderShadow->VertexShaderVertexFormat( fmt, 2, 0, 0 );

		DECLARE_STATIC_VERTEX_SHADER( worldvertextransition_vs30 );
		SET_STATIC_VERTEX_SHADER( worldvertextransition_vs30 );

		DECLARE_STATIC_PIXEL_SHADER( worldvertextransition_ps30 );
		SET_STATIC_PIXEL_SHADER_COMBO( MACROS, 0 );
		SET_STATIC_PIXEL_SHADER( worldvertextransition_ps30 );
	
		pShader->FogToFogColor();
	}
	DYNAMIC_STATE
	{
		pShader->BindTexture( SHADER_SAMPLER0, info.m_nBaseTextureVar, info.m_nBaseTextureFrameVar );
		pShader->BindTexture( SHADER_SAMPLER1, info.m_nBaseTexture2Var, info.m_nBaseTexture2FrameVar );

		// Texture 3 = lightmap
		pShaderAPI->BindStandardTexture( SHADER_SAMPLER2, TEXTURE_LIGHTMAP );
		
		pShader->EnablePixelShaderOverbright( 0, true, true );

		pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, info.m_nBaseTextureTransformVar );
		pShader->SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, info.m_nBaseTexture2TransformVar );

		DECLARE_DYNAMIC_VERTEX_SHADER( worldvertextransition_vs30 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
		SET_DYNAMIC_VERTEX_SHADER( worldvertextransition_vs30 );
		
		DECLARE_DYNAMIC_PIXEL_SHADER( worldvertextransition_ps30 );
		SET_DYNAMIC_PIXEL_SHADER( worldvertextransition_ps30 );
	}
	pShader->Draw();
}
