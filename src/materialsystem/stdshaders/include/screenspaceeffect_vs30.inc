// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#pragma once
#include "shaderlib/cshader.h"
class screenspaceeffect_vs30_Static_Index
{
	unsigned int m_nVERTEXCOLOR : 2;
#ifdef _DEBUG
	bool m_bVERTEXCOLOR : 1;
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

	screenspaceeffect_vs30_Static_Index(  )
	{
		m_nVERTEXCOLOR = 0;
#ifdef _DEBUG
		m_bVERTEXCOLOR = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bVERTEXCOLOR );
		return ( 1 * m_nVERTEXCOLOR ) + 0;
	}
};

#define shaderStaticTest_screenspaceeffect_vs30 vsh_forgot_to_set_static_VERTEXCOLOR


class screenspaceeffect_vs30_Dynamic_Index
{
public:
	screenspaceeffect_vs30_Dynamic_Index(  )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_screenspaceeffect_vs30 1

