// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#pragma once
#include "shaderlib/cshader.h"
class bufferclearobeystencil_vs30_Static_Index
{
	unsigned int m_nUSESCOLOR : 2;
#ifdef _DEBUG
	bool m_bUSESCOLOR : 1;
#endif	// _DEBUG
public:
	void SetUSESCOLOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nUSESCOLOR = i;
#ifdef _DEBUG
		m_bUSESCOLOR = true;
#endif	// _DEBUG
	}

	bufferclearobeystencil_vs30_Static_Index(  )
	{
		m_nUSESCOLOR = 0;
#ifdef _DEBUG
		m_bUSESCOLOR = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bUSESCOLOR );
		return ( 1 * m_nUSESCOLOR ) + 0;
	}
};

#define shaderStaticTest_bufferclearobeystencil_vs30 vsh_forgot_to_set_static_USESCOLOR


class bufferclearobeystencil_vs30_Dynamic_Index
{
public:
	bufferclearobeystencil_vs30_Dynamic_Index(  )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_bufferclearobeystencil_vs30 1

