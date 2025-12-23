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
class motion_blur_ps30_Static_Index
{
public:
	motion_blur_ps30_Static_Index(  )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderStaticTest_motion_blur_ps30 1


class motion_blur_ps30_Dynamic_Index
{
	unsigned int m_nQUALITY : 3;
#ifdef _DEBUG
	bool m_bQUALITY : 1;
#endif	// _DEBUG
public:
	void SetQUALITY( int i )
	{
		Assert( i >= 0 && i <= 3 );
		m_nQUALITY = i;
#ifdef _DEBUG
		m_bQUALITY = true;
#endif	// _DEBUG
	}

	motion_blur_ps30_Dynamic_Index(  )
	{
		m_nQUALITY = 0;
#ifdef _DEBUG
		m_bQUALITY = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bQUALITY );
		return ( 1 * m_nQUALITY ) + 0;
	}
};

#define shaderDynamicTest_motion_blur_ps30 psh_forgot_to_set_dynamic_QUALITY

