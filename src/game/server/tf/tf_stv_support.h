//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef TF_STV_SUPPORT_H
#define TF_STV_SUPPORT_H

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CTFDemoTVSupport : public CAutoGameSystemPerFrame, public CGameEventListener
{
public:
	CTFDemoTVSupport();

	virtual bool        Init() OVERRIDE;
	virtual void        FrameUpdatePostEntityThink() OVERRIDE;
	virtual char const* Name() OVERRIDE { return "CTFDemoTVSupport"; }
	virtual void        FireGameEvent( IGameEvent* event ) OVERRIDE;
	virtual void        LevelInitPostEntity() OVERRIDE;
	virtual void        LevelShutdownPostEntity() OVERRIDE;
	bool                StartRecording( void );
	void                StopRecording( bool bFromEngine = false );
	bool                IsRecording( void ) { return m_bRecording; }
	void                Status( void );

private:
	bool IsValidPath( const char* pszFolder );

	bool                   m_bRecording;
	char                   m_szFolder[24];
	char                   m_szFilename[MAX_PATH];
	char                   m_szFolderAndFilename[MAX_PATH];
	GCSDK::CWebAPIResponse m_DemoSpecificEventList;
	GCSDK::CWebAPIValues*  m_pRoot;
	GCSDK::CWebAPIValues*  m_pDetailsNode;
	float                  m_flNextRecordStartCheckTime;
	int                    m_nStartingTickCount;
};

#endif // TF_STV_SUPPORT_H
