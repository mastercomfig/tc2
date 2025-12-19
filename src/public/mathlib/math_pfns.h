//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef _MATH_PFNS_H_
#define _MATH_PFNS_H_

#pragma once

#include <math.h>
#include "../thirdparty/DirectXMath-apr2025/Inc/DirectXMath.h"
#define USE_SSE2
#include "../thirdparty/sse_mathfun/sse_mathfun.h"

FORCEINLINE void SinCos( float radians, float *RESTRICT sine, float *RESTRICT cosine )
{
	DirectX::XMScalarSinCos( sine, cosine, radians );
}
#define FastSinCos ::SinCos

FORCEINLINE float CosFast(float x)
{
	// Compiler doesn't optimize ::cosf call, use a vectorized cos. This is better than DirectX::XMScalarCos
	const __m128 vec = _mm_set_ss(x);
	const __m128 r = cos_ps(vec);
	float temp;
	_mm_store_ss(&temp, r);
	return temp;
}
#define FastCos ::CosFast

#define FastSqrt ::sqrtf

FORCEINLINE float RSqrt( float x )
{
	// The compiler will generate ideal instructions for a Newton-Raphson
	// Specifying it directly results in worse assembly.
	return 1.0f / sqrtf(x);
}
#define	FastRSqrt ::RSqrt

FORCEINLINE float RSqrtFast( float x )
{
	// This results in the compiler simplifying down to a plain rsqrtss
	const __m128 vec = _mm_set_ss(x);
	const __m128 r = _mm_rsqrt_ps(vec);
	float temp;
	_mm_store_ss(&temp, r);
	return temp;
}
#define FastRSqrtFast ::RSqrtFast

#endif // _MATH_PFNS_H_
