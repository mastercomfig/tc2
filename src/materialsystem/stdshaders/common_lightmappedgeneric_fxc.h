//========= Copyright Valve Corporation, All rights reserved. ============//


#if defined( _X360 )

void GetBaseTextureAndNormal( sampler base, sampler base2, sampler bump, bool bBase2, bool bBump, float3 coords, float3 vWeights,
								out float4 vResultBase, out float4 vResultBase2, out float4 vResultBump )
{
	vResultBase = 0;
	vResultBase2 = 0;
	vResultBump = 0;

	if ( !bBump )
	{
		vResultBump = float4(0, 0, 1, 1);
	}

#if SEAMLESS

	vWeights = max( vWeights - 0.3, 0 );

	vWeights *= 1.0f / dot( vWeights, float3(1,1,1) );

	[branch]
	if (vWeights.x > 0)
	{
		vResultBase  += vWeights.x * tex2D( base,  coords.zy );

		if ( bBase2 )
		{
			vResultBase2 += vWeights.x * tex2D( base2, coords.zy );
		}

		if ( bBump )
		{
			vResultBump  += vWeights.x * tex2D( bump,  coords.zy );
		}
	}

	[branch]
	if (vWeights.y > 0)
	{
		vResultBase  += vWeights.y * tex2D( base,  coords.xz );

		if ( bBase2 )
		{
			vResultBase2 += vWeights.y * tex2D( base2, coords.xz );
		}
		if ( bBump )
		{
			vResultBump  += vWeights.y * tex2D( bump,  coords.xz );
		}
	}

	[branch]
	if (vWeights.z > 0)
	{
		vResultBase  += vWeights.z * tex2D( base,  coords.xy );
		if ( bBase2 )
		{
			vResultBase2 += vWeights.z * tex2D( base2, coords.xy );
		}

		if ( bBump )
		{
			vResultBump  += vWeights.z * tex2D( bump,  coords.xy );
		}
	}

#else  // not seamless

	vResultBase = tex2D( base, coords.xy );

	if ( bBase2 )
	{
		vResultBase2 = tex2D( base2, coords.xy );
	}

	if ( bBump )
	{
		vResultBump  = tex2D( bump, coords.xy );
	}

#endif


}

#else // PC

void GetBaseTextureAndNormal( sampler base, sampler base2, sampler bump, bool bBase2, bool bBump, float3 coords, float3 vWeights,
							 out float4 vResultBase, out float4 vResultBase2, out float4 vResultBump )
{
	vResultBase = 0;
	vResultBase2 = 0;
	vResultBump = 0;

	if ( !bBump )
	{
		vResultBump = float4(0, 0, 1, 1);
	}

#if SEAMLESS

	vResultBase  += vWeights.x * tex2D( base, coords.zy );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.x * tex2D( base2, coords.zy );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.x * tex2D( bump, coords.zy );
	}

	vResultBase  += vWeights.y * tex2D( base, coords.xz );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.y * tex2D( base2, coords.xz );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.y * tex2D( bump, coords.xz );
	}

	vResultBase  += vWeights.z * tex2D( base, coords.xy );
	if ( bBase2 )
	{
		vResultBase2 += vWeights.z * tex2D( base2, coords.xy );
	}
	if ( bBump )
	{
		vResultBump  += vWeights.z * tex2D( bump, coords.xy );
	}

#else  // not seamless

	vResultBase  = tex2D( base, coords.xy );
	if ( bBase2 )
	{
		vResultBase2 = tex2D( base2, coords.xy );
	}
	if ( bBump )
	{
		vResultBump  = tex2D( bump, coords.xy );
	}
#endif

}

#endif

// misyl:
// Bicubic lightmap code lovingly taken and adapted from Godot
// ( https://github.com/godotengine/godot/pull/89919 )
// Licensed under MIT.

float4 BSplineCubicWeights( float a )
{
	float a2 = a * a;
	float a3 = a2 * a;
	return float4(
		-a3 + 3.0f*a2 - 3.0f*a + 1.0f,
		 3.0f*a3 - 6.0f*a2 + 4.0f,
		-3.0f*a3 + 3.0f*a2 + 3.0f*a + 1.0f,
		 a3
	) * ( 1.0f / 6.0f );
}

#ifndef BICUBIC_LIGHTMAP
#define BICUBIC_LIGHTMAP 0
#endif

float3 LightMapSample( sampler LightmapSampler, float2 vTexCoord, float4 texSize )
{
#	if ( !defined( _X360 ) || !defined( USE_32BIT_LIGHTMAPS_ON_360 ) )
	{
#if BICUBIC_LIGHTMAP
        const float2 vTextureSize = float2( texSize.x, texSize.y );
        const float2 vTexelSize = float2( 1.0f, 1.0f ) / vTextureSize;

        float2 tc = vTexCoord.xy * vTextureSize - float2( 0.5f, 0.5f );
        float2 center_tc = floor( tc + 0.5f );
        float2 fuv = tc - center_tc;

        float3 wx;
        wx.x = 0.5f * ( 0.5f - fuv.x ) * ( 0.5f - fuv.x );
        wx.y = 0.75f - fuv.x * fuv.x;
        wx.z = 0.5f * ( 0.5f + fuv.x ) * ( 0.5f + fuv.x );

        float3 wy;
        wy.x = 0.5f * ( 0.5f - fuv.y ) * ( 0.5f - fuv.y );
        wy.y = 0.75f - fuv.y * fuv.y;
        wy.z = 0.5f * ( 0.5f + fuv.y ) * ( 0.5f + fuv.y );

        float2 zero = float2( 0.0f, 0.0f );

        float3 center = tex2D( LightmapSampler, vTexCoord.xy, zero, zero ).rgb;

        float3 result = 0.0f;
        float sumW = 0.0f;
        float edge_sharpness = 200.0f;

        // 9-tap Bilateral Quadratic B-spline Kernel
        [unroll]
        for ( int y = 0; y < 3; y++ )
        {
            [unroll]
            for ( int x = 0; x < 3; x++ )
            {
                float2 sampleUV = ( center_tc + float2( x - 1.0f, y - 1.0f ) + float2( 0.5f, 0.5f ) ) * vTexelSize;
                float3 sampleColor = tex2D( LightmapSampler, sampleUV, zero, zero ).rgb;
                
                float w_spatial = wx[x] * wy[y];
                
                // Bilateral Range weight
                float3 diff = sampleColor - center;
                float w_range = 1.0f / ( 1.0f + dot( diff, diff ) * edge_sharpness );
                
                float weight = w_spatial * w_range;
                result += sampleColor * weight;
                sumW += weight;
            }
        }

        // Calculate how much weight was "lost" due to bilateral rejection.
        float center_weight = saturate( 1.0f - sumW );
        return result + center * center_weight;
#else
        return tex2D( LightmapSampler, vTexCoord ).rgb;
#endif
    }
#	else
	{
#		if 0 //1 for cheap sampling, 0 for accurate scaling from the individual samples
		{
			float4 sample = tex2D( LightmapSampler, vTexCoord );

			return sample.rgb * sample.a;
		}
#		else
		{
			float4 Weights;
			float4 samples_0; //no arrays allowed in inline assembly
			float4 samples_1;
			float4 samples_2;
			float4 samples_3;
			
			asm {
				tfetch2D samples_0, vTexCoord.xy, LightmapSampler, OffsetX = -0.5, OffsetY = -0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
				tfetch2D samples_1, vTexCoord.xy, LightmapSampler, OffsetX =  0.5, OffsetY = -0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
				tfetch2D samples_2, vTexCoord.xy, LightmapSampler, OffsetX = -0.5, OffsetY =  0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false
				tfetch2D samples_3, vTexCoord.xy, LightmapSampler, OffsetX =  0.5, OffsetY =  0.5, MinFilter=point, MagFilter=point, MipFilter=keep, UseComputedLOD=false

				getWeights2D Weights, vTexCoord.xy, LightmapSampler
			};

			Weights = float4( (1-Weights.x)*(1-Weights.y), Weights.x*(1-Weights.y), (1-Weights.x)*Weights.y, Weights.x*Weights.y );

			float3 result;
			result.rgb  = samples_0.rgb * (samples_0.a * Weights.x);
			result.rgb += samples_1.rgb * (samples_1.a * Weights.y);
			result.rgb += samples_2.rgb * (samples_2.a * Weights.z);
			result.rgb += samples_3.rgb * (samples_3.a * Weights.w);
		
			return result;
		}
#		endif
	}
#	endif
}

