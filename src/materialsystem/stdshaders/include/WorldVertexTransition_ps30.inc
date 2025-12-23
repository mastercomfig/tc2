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
class worldvertextransition_ps30_Static_Index
{
	unsigned int m_nMACROS : 2;
#ifdef _DEBUG
	bool m_bMACROS : 1;
#endif	// _DEBUG
public:
	void SetMACROS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMACROS = i;
#ifdef _DEBUG
		m_bMACROS = true;
#endif	// _DEBUG
	}

	worldvertextransition_ps30_Static_Index(  )
	{
		m_nMACROS = 0;
#ifdef _DEBUG
		m_bMACROS = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bMACROS );
		return ( 1 * m_nMACROS ) + 0;
	}
};

#define shaderStaticTest_worldvertextransition_ps30 psh_forgot_to_set_static_MACROS


class worldvertextransition_ps30_Dynamic_Index
{
public:
	worldvertextransition_ps30_Dynamic_Index(  )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_worldvertextransition_ps30 1

