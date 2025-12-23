// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#pragma once
#include "shaderlib/cshader.h"
class portal_vs30_Static_Index
{
	unsigned int m_nHASALPHAMASK : 2;
	unsigned int m_nHASSTATICTEXTURE : 2;
	unsigned int m_nUSEALTERNATEVIEW : 2;
#ifdef _DEBUG
	bool m_bHASALPHAMASK : 1;
	bool m_bHASSTATICTEXTURE : 1;
	bool m_bUSEALTERNATEVIEW : 1;
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

	void SetUSEALTERNATEVIEW( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nUSEALTERNATEVIEW = i;
#ifdef _DEBUG
		m_bUSEALTERNATEVIEW = true;
#endif	// _DEBUG
	}

	portal_vs30_Static_Index(  )
	{
		m_nHASALPHAMASK = 0;
		m_nHASSTATICTEXTURE = 0;
		m_nUSEALTERNATEVIEW = 0;
#ifdef _DEBUG
		m_bHASALPHAMASK = false;
		m_bHASSTATICTEXTURE = false;
		m_bUSEALTERNATEVIEW = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bHASALPHAMASK && m_bHASSTATICTEXTURE && m_bUSEALTERNATEVIEW );
		return ( 4 * m_nHASALPHAMASK ) + ( 8 * m_nHASSTATICTEXTURE ) + ( 16 * m_nUSEALTERNATEVIEW ) + 0;
	}
};

#define shaderStaticTest_portal_vs30 vsh_forgot_to_set_static_HASALPHAMASK + vsh_forgot_to_set_static_HASSTATICTEXTURE + vsh_forgot_to_set_static_USEALTERNATEVIEW


class portal_vs30_Dynamic_Index
{
	unsigned int m_nSKINNING : 2;
	unsigned int m_nADDSTATIC : 2;
#ifdef _DEBUG
	bool m_bSKINNING : 1;
	bool m_bADDSTATIC : 1;
#endif	// _DEBUG
public:
	void SetSKINNING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSKINNING = i;
#ifdef _DEBUG
		m_bSKINNING = true;
#endif	// _DEBUG
	}

	void SetADDSTATIC( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nADDSTATIC = i;
#ifdef _DEBUG
		m_bADDSTATIC = true;
#endif	// _DEBUG
	}

	portal_vs30_Dynamic_Index(  )
	{
		m_nSKINNING = 0;
		m_nADDSTATIC = 0;
#ifdef _DEBUG
		m_bSKINNING = false;
		m_bADDSTATIC = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bSKINNING && m_bADDSTATIC );
		return ( 1 * m_nSKINNING ) + ( 2 * m_nADDSTATIC ) + 0;
	}
};

#define shaderDynamicTest_portal_vs30 vsh_forgot_to_set_dynamic_SKINNING + vsh_forgot_to_set_dynamic_ADDSTATIC

