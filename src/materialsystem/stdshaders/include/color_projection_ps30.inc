// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $HDRTYPE && defined $HDRENABLED && !$HDRTYPE && $HDRENABLED
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $HDRTYPE && $LIGHTING_PREVIEW && $HDRTYPE != 0
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW
// defined $HDRTYPE && defined $HDRENABLED && !$HDRTYPE && $HDRENABLED
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $HDRTYPE && $LIGHTING_PREVIEW && $HDRTYPE != 0
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW

#pragma once
#include "shaderlib/cshader.h"
class color_projection_ps30_Static_Index
{
public:
	color_projection_ps30_Static_Index(  )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderStaticTest_color_projection_ps30 1


class color_projection_ps30_Dynamic_Index
{
	unsigned int m_nNEED_BLINDMK : 2;
	unsigned int m_nNEED_ANOMYLIZE : 2;
	unsigned int m_nNEED_MONOCHROME : 2;
#ifdef _DEBUG
	bool m_bNEED_BLINDMK : 1;
	bool m_bNEED_ANOMYLIZE : 1;
	bool m_bNEED_MONOCHROME : 1;
#endif	// _DEBUG
public:
	void SetNEED_BLINDMK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNEED_BLINDMK = i;
#ifdef _DEBUG
		m_bNEED_BLINDMK = true;
#endif	// _DEBUG
	}

	void SetNEED_ANOMYLIZE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNEED_ANOMYLIZE = i;
#ifdef _DEBUG
		m_bNEED_ANOMYLIZE = true;
#endif	// _DEBUG
	}

	void SetNEED_MONOCHROME( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNEED_MONOCHROME = i;
#ifdef _DEBUG
		m_bNEED_MONOCHROME = true;
#endif	// _DEBUG
	}

	color_projection_ps30_Dynamic_Index(  )
	{
		m_nNEED_BLINDMK = 0;
		m_nNEED_ANOMYLIZE = 0;
		m_nNEED_MONOCHROME = 0;
#ifdef _DEBUG
		m_bNEED_BLINDMK = false;
		m_bNEED_ANOMYLIZE = false;
		m_bNEED_MONOCHROME = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bNEED_BLINDMK && m_bNEED_ANOMYLIZE && m_bNEED_MONOCHROME );
		return ( 1 * m_nNEED_BLINDMK ) + ( 2 * m_nNEED_ANOMYLIZE ) + ( 4 * m_nNEED_MONOCHROME ) + 0;
	}
};

#define shaderDynamicTest_color_projection_ps30 psh_forgot_to_set_dynamic_NEED_BLINDMK + psh_forgot_to_set_dynamic_NEED_ANOMYLIZE + psh_forgot_to_set_dynamic_NEED_MONOCHROME

