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
class downsample_nohdr_ps30_Static_Index
{
	unsigned int m_nCSTRIKE : 2;
#ifdef _DEBUG
	bool m_bCSTRIKE : 1;
#endif	// _DEBUG
public:
	void SetCSTRIKE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCSTRIKE = i;
#ifdef _DEBUG
		m_bCSTRIKE = true;
#endif	// _DEBUG
	}

	downsample_nohdr_ps30_Static_Index(  )
	{
		m_nCSTRIKE = 0;
#ifdef _DEBUG
		m_bCSTRIKE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bCSTRIKE );
		return ( 1 * m_nCSTRIKE ) + 0;
	}
};

#define shaderStaticTest_downsample_nohdr_ps30 psh_forgot_to_set_static_CSTRIKE


class downsample_nohdr_ps30_Dynamic_Index
{
public:
	downsample_nohdr_ps30_Dynamic_Index(  )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_downsample_nohdr_ps30 1

