// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#pragma once
#include "shaderlib/cshader.h"
class sprite_vs30_Static_Index
{
	unsigned int m_nVERTEXCOLOR : 2;
	unsigned int m_nSRGB : 2;
#ifdef _DEBUG
	bool m_bVERTEXCOLOR : 1;
	bool m_bSRGB : 1;
#endif	// _DEBUG
public:
	void SetVERTEXCOLOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEXCOLOR = i;
#ifdef _DEBUG
		m_bVERTEXCOLOR = true;
#endif	// _DEBUG
	}

	void SetSRGB( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSRGB = i;
#ifdef _DEBUG
		m_bSRGB = true;
#endif	// _DEBUG
	}

	sprite_vs30_Static_Index(  )
	{
		m_nVERTEXCOLOR = 0;
		m_nSRGB = 0;
#ifdef _DEBUG
		m_bVERTEXCOLOR = false;
		m_bSRGB = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bVERTEXCOLOR && m_bSRGB );
		return ( 1 * m_nVERTEXCOLOR ) + ( 2 * m_nSRGB ) + 0;
	}
};

#define shaderStaticTest_sprite_vs30 vsh_forgot_to_set_static_VERTEXCOLOR + vsh_forgot_to_set_static_SRGB


class sprite_vs30_Dynamic_Index
{
public:
	sprite_vs30_Dynamic_Index(  )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_sprite_vs30 1

