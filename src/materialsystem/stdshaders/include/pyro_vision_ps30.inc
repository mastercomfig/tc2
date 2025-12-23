// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// ( STRIPES == 0 ) &&  ( STRIPES_USE_NORMAL2 == 1 )
// ( EFFECT != 1 ) &&  ( COLOR_BAR == 1 )
// ( EFFECT != 2 ) &&  ( VISUALIZE_DOF == 1 )
// ( EFFECT == 2 || EFFECT == 3 ) && ( STRIPES == 1 || STRIPES_USE_NORMAL2 == 1 || COLOR_BAR == 1 || BASETEXTURE2 == 1 || FANCY_BLENDING == 1 || VERTEX_LIT == 1 || PIXELFOGTYPE == 1 || SELFILLUM == 1 )
// ( EFFECT != 3 ) && ( HEATHAZE == 1 )
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
class pyro_vision_ps30_Static_Index
{
	unsigned int m_nEFFECT : 3;
	unsigned int m_nVERTEX_LIT : 2;
	unsigned int m_nBASETEXTURE2 : 2;
	unsigned int m_nFANCY_BLENDING : 2;
	unsigned int m_nSELFILLUM : 2;
	unsigned int m_nCOLOR_BAR : 2;
	unsigned int m_nSTRIPES : 2;
	unsigned int m_nSTRIPES_USE_NORMAL2 : 2;
#ifdef _DEBUG
	bool m_bEFFECT : 1;
	bool m_bVERTEX_LIT : 1;
	bool m_bBASETEXTURE2 : 1;
	bool m_bFANCY_BLENDING : 1;
	bool m_bSELFILLUM : 1;
	bool m_bCOLOR_BAR : 1;
	bool m_bSTRIPES : 1;
	bool m_bSTRIPES_USE_NORMAL2 : 1;
#endif	// _DEBUG
public:
	void SetEFFECT( int i )
	{
		Assert( i >= 0 && i <= 3 );
		m_nEFFECT = i;
#ifdef _DEBUG
		m_bEFFECT = true;
#endif	// _DEBUG
	}

	void SetVERTEX_LIT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEX_LIT = i;
#ifdef _DEBUG
		m_bVERTEX_LIT = true;
#endif	// _DEBUG
	}

	void SetBASETEXTURE2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURE2 = i;
#ifdef _DEBUG
		m_bBASETEXTURE2 = true;
#endif	// _DEBUG
	}

	void SetFANCY_BLENDING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFANCY_BLENDING = i;
#ifdef _DEBUG
		m_bFANCY_BLENDING = true;
#endif	// _DEBUG
	}

	void SetSELFILLUM( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSELFILLUM = i;
#ifdef _DEBUG
		m_bSELFILLUM = true;
#endif	// _DEBUG
	}

	void SetCOLOR_BAR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCOLOR_BAR = i;
#ifdef _DEBUG
		m_bCOLOR_BAR = true;
#endif	// _DEBUG
	}

	void SetSTRIPES( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSTRIPES = i;
#ifdef _DEBUG
		m_bSTRIPES = true;
#endif	// _DEBUG
	}

	void SetSTRIPES_USE_NORMAL2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSTRIPES_USE_NORMAL2 = i;
#ifdef _DEBUG
		m_bSTRIPES_USE_NORMAL2 = true;
#endif	// _DEBUG
	}

	pyro_vision_ps30_Static_Index(  )
	{
		m_nEFFECT = 0;
		m_nVERTEX_LIT = 0;
		m_nBASETEXTURE2 = 0;
		m_nFANCY_BLENDING = 0;
		m_nSELFILLUM = 0;
		m_nCOLOR_BAR = 0;
		m_nSTRIPES = 0;
		m_nSTRIPES_USE_NORMAL2 = 0;
#ifdef _DEBUG
		m_bEFFECT = false;
		m_bVERTEX_LIT = false;
		m_bBASETEXTURE2 = false;
		m_bFANCY_BLENDING = false;
		m_bSELFILLUM = false;
		m_bCOLOR_BAR = false;
		m_bSTRIPES = false;
		m_bSTRIPES_USE_NORMAL2 = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bEFFECT && m_bVERTEX_LIT && m_bBASETEXTURE2 && m_bFANCY_BLENDING && m_bSELFILLUM && m_bCOLOR_BAR && m_bSTRIPES && m_bSTRIPES_USE_NORMAL2 );
		return ( 8 * m_nEFFECT ) + ( 32 * m_nVERTEX_LIT ) + ( 64 * m_nBASETEXTURE2 ) + ( 128 * m_nFANCY_BLENDING ) + ( 256 * m_nSELFILLUM ) + ( 512 * m_nCOLOR_BAR ) + ( 1024 * m_nSTRIPES ) + ( 2048 * m_nSTRIPES_USE_NORMAL2 ) + 0;
	}
};

#define shaderStaticTest_pyro_vision_ps30 psh_forgot_to_set_static_EFFECT + psh_forgot_to_set_static_VERTEX_LIT + psh_forgot_to_set_static_BASETEXTURE2 + psh_forgot_to_set_static_FANCY_BLENDING + psh_forgot_to_set_static_SELFILLUM + psh_forgot_to_set_static_COLOR_BAR + psh_forgot_to_set_static_STRIPES + psh_forgot_to_set_static_STRIPES_USE_NORMAL2


class pyro_vision_ps30_Dynamic_Index
{
	unsigned int m_nPIXELFOGTYPE : 2;
	unsigned int m_nVISUALIZE_DOF : 2;
	unsigned int m_nHEATHAZE : 2;
#ifdef _DEBUG
	bool m_bPIXELFOGTYPE : 1;
	bool m_bVISUALIZE_DOF : 1;
	bool m_bHEATHAZE : 1;
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

	void SetVISUALIZE_DOF( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVISUALIZE_DOF = i;
#ifdef _DEBUG
		m_bVISUALIZE_DOF = true;
#endif	// _DEBUG
	}

	void SetHEATHAZE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHEATHAZE = i;
#ifdef _DEBUG
		m_bHEATHAZE = true;
#endif	// _DEBUG
	}

	pyro_vision_ps30_Dynamic_Index(  )
	{
		m_nPIXELFOGTYPE = 0;
		m_nVISUALIZE_DOF = 0;
		m_nHEATHAZE = 0;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
		m_bVISUALIZE_DOF = false;
		m_bHEATHAZE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bPIXELFOGTYPE && m_bVISUALIZE_DOF && m_bHEATHAZE );
		return ( 1 * m_nPIXELFOGTYPE ) + ( 2 * m_nVISUALIZE_DOF ) + ( 4 * m_nHEATHAZE ) + 0;
	}
};

#define shaderDynamicTest_pyro_vision_ps30 psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_VISUALIZE_DOF + psh_forgot_to_set_dynamic_HEATHAZE

