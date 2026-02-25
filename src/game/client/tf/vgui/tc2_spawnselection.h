//========= Copyright Team Comtress 2, All rights reserved. ============//
//
// Purpose: TC2 Spawn Selection Panel - overhead map with spawn markers
//
//=============================================================================//

#ifndef TC2_SPAWNSELECTION_H
#define TC2_SPAWNSELECTION_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include "tf_shareddefs.h"
#include "GameEventListener.h"
#include "mapoverview.h"

namespace vgui
{
	class EditablePanel;
	class Label;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn node marker on the overhead map
//-----------------------------------------------------------------------------
class CTC2SpawnNodePanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CTC2SpawnNodePanel, vgui::Panel );

public:
	CTC2SpawnNodePanel( vgui::Panel *parent, const char *panelName, int nodeIndex );
	virtual ~CTC2SpawnNodePanel();

	void SetNodeData( const TCSpawnNode_t &node );
	void SetSelected( bool bSelected ) { m_bSelected = bSelected; }
	bool IsSelected() const { return m_bSelected; }
	int GetNodeIndex() const { return m_nNodeIndex; }
	const TCSpawnNode_t& GetNodeData() const { return m_Node; }

	virtual void Paint() override;
	virtual void OnCursorEntered() override;
	virtual void OnCursorExited() override;
	virtual void OnMousePressed( vgui::MouseCode code ) override;

private:
	TCSpawnNode_t m_Node;
	int m_nNodeIndex;
	bool m_bSelected;
	bool m_bHovered;
	Color m_NodeColor;
};

//-----------------------------------------------------------------------------
// Purpose: Main spawn selection panel with overhead map
//-----------------------------------------------------------------------------
class CTC2SpawnSelectionPanel : public vgui::Frame, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTC2SpawnSelectionPanel, vgui::Frame );

public:
	CTC2SpawnSelectionPanel( vgui::Panel *parent );
	virtual ~CTC2SpawnSelectionPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) override;
	virtual void PerformLayout() override;
	virtual void OnCommand( const char *command ) override;
	virtual void OnThink() override;

	void ShowPanel( bool bShow );
	void UpdateSpawnNodes();
	void SelectSpawnNode( int nodeIndex );

	// IGameEventListener
	virtual void FireGameEvent( IGameEvent *event ) override;

	MESSAGE_FUNC_PARAMS( OnSpawnNodeSelected, "SpawnNodeSelected", pData );

private:
	void PositionNodeMarkers();
	void SendSpawnSelection();

	vgui::EditablePanel *m_pMapPanel;
	vgui::Label *m_pTitleLabel;
	vgui::Button *m_pConfirmButton;
	vgui::Button *m_pCancelButton;

	CMapOverview* m_pMapOverview;

	CUtlVector<CTC2SpawnNodePanel*> m_NodePanels;
	int m_nSelectedNode;
};

#endif // TC2_SPAWNSELECTION_H
