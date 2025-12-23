// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#pragma once
#include "shaderlib/cshader.h"
class lightmappedreflective_vs30_Static_Index
{
	unsigned int m_nBASETEXTURE : 2;
#ifdef _DEBUG
	bool m_bBASETEXTURE : 1;
#endif	// _DEBUG
public:
	void SetBASETEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURE = i;
#ifdef _DEBUG
		m_bBASETEXTURE = true;
#endif	// _DEBUG
	}

	lightmappedreflective_vs30_Static_Index(  )
	{
		m_nBASETEXTURE = 0;
#ifdef _DEBUG
		m_bBASETEXTURE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bBASETEXTURE );
		return ( 1 * m_nBASETEXTURE ) + 0;
	}
};

#define shaderStaticTest_lightmappedreflective_vs30 vsh_forgot_to_set_static_BASETEXTURE


class lightmappedreflective_vs30_Dynamic_Index
{
public:
	lightmappedreflective_vs30_Dynamic_Index(  )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_lightmappedreflective_vs30 1

