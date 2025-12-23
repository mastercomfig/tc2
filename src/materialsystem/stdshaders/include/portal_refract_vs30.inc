// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#pragma once
#include "shaderlib/cshader.h"
class portal_refract_vs30_Static_Index
{
	unsigned int m_nSTAGE : 2;
#ifdef _DEBUG
	bool m_bSTAGE : 1;
#endif	// _DEBUG
public:
	void SetSTAGE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nSTAGE = i;
#ifdef _DEBUG
		m_bSTAGE = true;
#endif	// _DEBUG
	}

	portal_refract_vs30_Static_Index(  )
	{
		m_nSTAGE = 0;
#ifdef _DEBUG
		m_bSTAGE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bSTAGE );
		return ( 1 * m_nSTAGE ) + 0;
	}
};

#define shaderStaticTest_portal_refract_vs30 vsh_forgot_to_set_static_STAGE


class portal_refract_vs30_Dynamic_Index
{
public:
	portal_refract_vs30_Dynamic_Index(  )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_portal_refract_vs30 1

