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
class introscreenspaceeffect_ps30_Static_Index
{
public:
	introscreenspaceeffect_ps30_Static_Index(  )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderStaticTest_introscreenspaceeffect_ps30 1


class introscreenspaceeffect_ps30_Dynamic_Index
{
	unsigned int m_nMODE : 4;
#ifdef _DEBUG
	bool m_bMODE : 1;
#endif	// _DEBUG
public:
	void SetMODE( int i )
	{
		Assert( i >= 0 && i <= 9 );
		m_nMODE = i;
#ifdef _DEBUG
		m_bMODE = true;
#endif	// _DEBUG
	}

	introscreenspaceeffect_ps30_Dynamic_Index(  )
	{
		m_nMODE = 0;
#ifdef _DEBUG
		m_bMODE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bMODE );
		return ( 1 * m_nMODE ) + 0;
	}
};

#define shaderDynamicTest_introscreenspaceeffect_ps30 psh_forgot_to_set_dynamic_MODE

