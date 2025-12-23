// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// ( $COMBINE_MODE == 4 || $COMBINE_MODE == 5 )
// ( $COMBINE_MODE != 6 && $DEBUG_MODE == 1 )
// defined $HDRTYPE && defined $HDRENABLED && !$HDRTYPE && $HDRENABLED
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $HDRTYPE && $LIGHTING_PREVIEW && $HDRTYPE != 0
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW

#pragma once
#include "shaderlib/cshader.h"
class compositor_ps30_Static_Index
{
	unsigned int m_nCOMBINE_MODE : 3;
#ifdef _DEBUG
	bool m_bCOMBINE_MODE : 1;
#endif	// _DEBUG
public:
	void SetCOMBINE_MODE( int i )
	{
		Assert( i >= 0 && i <= 6 );
		m_nCOMBINE_MODE = i;
#ifdef _DEBUG
		m_bCOMBINE_MODE = true;
#endif	// _DEBUG
	}

	compositor_ps30_Static_Index(  )
	{
		m_nCOMBINE_MODE = 0;
#ifdef _DEBUG
		m_bCOMBINE_MODE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bCOMBINE_MODE );
		AssertMsg( !( ( m_nCOMBINE_MODE == 4 ) || ( m_nCOMBINE_MODE == 5 ) ), "Invalid combo combination ( ( COMBINE_MODE == 4 ) || ( COMBINE_MODE == 5 ) )" );
		return ( 2 * m_nCOMBINE_MODE ) + 0;
	}
};

#define shaderStaticTest_compositor_ps30 psh_forgot_to_set_static_COMBINE_MODE


class compositor_ps30_Dynamic_Index
{
	unsigned int m_nDEBUG_MODE : 2;
#ifdef _DEBUG
	bool m_bDEBUG_MODE : 1;
#endif	// _DEBUG
public:
	void SetDEBUG_MODE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDEBUG_MODE = i;
#ifdef _DEBUG
		m_bDEBUG_MODE = true;
#endif	// _DEBUG
	}

	compositor_ps30_Dynamic_Index(  )
	{
		m_nDEBUG_MODE = 0;
#ifdef _DEBUG
		m_bDEBUG_MODE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bDEBUG_MODE );
		return ( 1 * m_nDEBUG_MODE ) + 0;
	}
};

#define shaderDynamicTest_compositor_ps30 psh_forgot_to_set_dynamic_DEBUG_MODE

