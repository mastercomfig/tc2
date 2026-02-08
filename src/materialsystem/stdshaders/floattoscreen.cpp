//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "screenspaceeffect_vs30.inc"
#include "floattoscreen_ps30.inc"
#include "convar.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER_FLAGS( floattoscreen, "Help for floattoscreen", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( PIXSHADER, SHADER_PARAM_TYPE_STRING, "floattoscreen_ps30", "Name of the pixel shader to use" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if( params[FBTEXTURE]->IsDefined() )
		{
			LoadTexture( FBTEXTURE );
		}
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
			return "Wireframe";
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			// convert from linear to gamma on write.
			pShaderShadow->EnableSRGBWrite( true );

			// Pre-cache shaders
			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, false );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs30 );

			char szBuf[256];
			RenameShaderToShaderModel30( params[PIXSHADER]->GetStringValue(), szBuf );
			pShaderShadow->SetPixelShader( szBuf, 0 );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, FBTEXTURE, -1 );
			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs30 );

			pShaderAPI->SetPixelShaderIndex( 0 );
		}
		Draw();
	}
END_SHADER
