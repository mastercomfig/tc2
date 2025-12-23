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
class portal_ps30_Static_Index
{
	unsigned int m_nHASALPHAMASK : 2;
	unsigned int m_nHASSTATICTEXTURE : 2;
#ifdef _DEBUG
	bool m_bHASALPHAMASK : 1;
	bool m_bHASSTATICTEXTURE : 1;
#endif	// _DEBUG
public:
	void SetHASALPHAMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHASALPHAMASK = i;
#ifdef _DEBUG
		m_bHASALPHAMASK = true;
#endif	// _DEBUG
	}

	void SetHASSTATICTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHASSTATICTEXTURE = i;
#ifdef _DEBUG
		m_bHASSTATICTEXTURE = true;
#endif	// _DEBUG
	}

	portal_ps30_Static_Index(  )
	{
		m_nHASALPHAMASK = 0;
		m_nHASSTATICTEXTURE = 0;
#ifdef _DEBUG
		m_bHASALPHAMASK = false;
		m_bHASSTATICTEXTURE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bHASALPHAMASK && m_bHASSTATICTEXTURE );
		return ( 8 * m_nHASALPHAMASK ) + ( 16 * m_nHASSTATICTEXTURE ) + 0;
	}
};

#define shaderStaticTest_portal_ps30 psh_forgot_to_set_static_HASALPHAMASK + psh_forgot_to_set_static_HASSTATICTEXTURE


class portal_ps30_Dynamic_Index
{
	unsigned int m_nADDSTATIC : 2;
	unsigned int m_nHDRENABLED : 2;
	unsigned int m_nPIXELFOGTYPE : 2;
#ifdef _DEBUG
	bool m_bADDSTATIC : 1;
	bool m_bHDRENABLED : 1;
	bool m_bPIXELFOGTYPE : 1;
#endif	// _DEBUG
public:
	void SetADDSTATIC( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nADDSTATIC = i;
#ifdef _DEBUG
		m_bADDSTATIC = true;
#endif	// _DEBUG
	}

	void SetHDRENABLED( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHDRENABLED = i;
#ifdef _DEBUG
		m_bHDRENABLED = true;
#endif	// _DEBUG
	}

	void SetPIXELFOGTYPE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPIXELFOGTYPE = i;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = true;
#endif	// _DEBUG
	}

	portal_ps30_Dynamic_Index(  )
	{
		m_nADDSTATIC = 0;
		m_nHDRENABLED = 0;
		m_nPIXELFOGTYPE = 0;
#ifdef _DEBUG
		m_bADDSTATIC = false;
		m_bHDRENABLED = false;
		m_bPIXELFOGTYPE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bADDSTATIC && m_bHDRENABLED && m_bPIXELFOGTYPE );
		return ( 1 * m_nADDSTATIC ) + ( 2 * m_nHDRENABLED ) + ( 4 * m_nPIXELFOGTYPE ) + 0;
	}
};

#define shaderDynamicTest_portal_ps30 psh_forgot_to_set_dynamic_ADDSTATIC + psh_forgot_to_set_dynamic_HDRENABLED + psh_forgot_to_set_dynamic_PIXELFOGTYPE

