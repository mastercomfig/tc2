// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $HDRTYPE && defined $HDRENABLED && !$HDRTYPE && $HDRENABLED
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $HDRTYPE && $LIGHTING_PREVIEW && $HDRTYPE != 0
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW

#pragma once
#include "shaderlib/cshader.h"
class colorcorrection_ps30_Static_Index
{
public:
	colorcorrection_ps30_Static_Index(  )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderStaticTest_colorcorrection_ps30 1


class colorcorrection_ps30_Dynamic_Index
{
	unsigned int m_nNUM_LOOKUPS : 3;
#ifdef _DEBUG
	bool m_bNUM_LOOKUPS : 1;
#endif	// _DEBUG
public:
	void SetNUM_LOOKUPS( int i )
	{
		Assert( i >= 0 && i <= 4 );
		m_nNUM_LOOKUPS = i;
#ifdef _DEBUG
		m_bNUM_LOOKUPS = true;
#endif	// _DEBUG
	}

	colorcorrection_ps30_Dynamic_Index(  )
	{
		m_nNUM_LOOKUPS = 0;
#ifdef _DEBUG
		m_bNUM_LOOKUPS = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bNUM_LOOKUPS );
		return ( 1 * m_nNUM_LOOKUPS ) + 0;
	}
};

#define shaderDynamicTest_colorcorrection_ps30 psh_forgot_to_set_dynamic_NUM_LOOKUPS

