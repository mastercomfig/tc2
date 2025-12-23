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
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs30 );

//			DECLARE_STATIC_PIXEL_SHADER( floattoscreen_ps30 );
//			SET_STATIC_PIXEL_SHADER( floattoscreen_ps30 );
			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				const char *szPixelShader = params[PIXSHADER]->GetStringValue();
				size_t iLength = Q_strlen( szPixelShader );

				if( (iLength > 5) && (Q_stricmp( &szPixelShader[iLength - 5], "_ps20" ) == 0) ) //detect if it's trying to load a ps20 shader
				{
					//replace it with the ps20b shader
					char *szNewName = (char *)stackalloc( sizeof( char ) * (iLength + 2) );
					memcpy( szNewName, szPixelShader, sizeof( char ) * iLength );
					szNewName[iLength] = 'b';
					szNewName[iLength + 1] = '\0';
					pShaderShadow->SetPixelShader( szNewName, 0 );
				}
				else
				{
					pShaderShadow->SetPixelShader( params[PIXSHADER]->GetStringValue(), 0 );
				}
			}
			else
			{
				pShaderShadow->SetPixelShader( params[PIXSHADER]->GetStringValue(), 0 );
			}
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, FBTEXTURE, -1 );
			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs30 );

//			DECLARE_DYNAMIC_PIXEL_SHADER( floattoscreen_ps30 );
//			SET_DYNAMIC_PIXEL_SHADER( floattoscreen_ps30 );
			pShaderAPI->SetPixelShaderIndex( 0 );
		}
		Draw();
	}
END_SHADER
