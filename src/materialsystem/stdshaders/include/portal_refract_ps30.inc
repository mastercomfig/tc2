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
class portal_refract_ps30_Static_Index
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

	portal_refract_ps30_Static_Index(  )
	{
		m_nSTAGE = 0;
#ifdef _DEBUG
		m_bSTAGE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bSTAGE );
		return ( 2 * m_nSTAGE ) + 0;
	}
};

#define shaderStaticTest_portal_refract_ps30 psh_forgot_to_set_static_STAGE


class portal_refract_ps30_Dynamic_Index
{
	unsigned int m_nPIXELFOGTYPE : 2;
#ifdef _DEBUG
	bool m_bPIXELFOGTYPE : 1;
#endif	// _DEBUG
public:
	void SetPIXELFOGTYPE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPIXELFOGTYPE = i;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = true;
#endif	// _DEBUG
	}

	portal_refract_ps30_Dynamic_Index(  )
	{
		m_nPIXELFOGTYPE = 0;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bPIXELFOGTYPE );
		return ( 1 * m_nPIXELFOGTYPE ) + 0;
	}
};

#define shaderDynamicTest_portal_refract_ps30 psh_forgot_to_set_dynamic_PIXELFOGTYPE

