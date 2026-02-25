//========= Copyright Team Comtress 2, All rights reserved. ============//
//
// Purpose: TC2 Spawn Selection Panel implementation
//
//=============================================================================//

#include "cbase.h"
#include "tc2_spawnselection.h"
#include "c_tf_player.h"
#include "tf_gamerules.h"
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/EditablePanel.h>
#include "iclientmode.h"
#include "ienginevgui.h"
// Viewport/class menu access
#include <game/client/iviewport.h>
#include "tf_shareddefs.h"
// Map overview projection (for better world->map mapping)
#include "game_controls/mapoverview.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar tf_tc2_spawn_ui_debug( "tf_tc2_spawn_ui_debug", "0", FCVAR_NONE, "Debug spawn selection UI" );

//-----------------------------------------------------------------------------
// Purpose: Spawn node marker constructor
//-----------------------------------------------------------------------------
CTC2SpawnNodePanel::CTC2SpawnNodePanel( Panel *parent, const char *panelName, int nodeIndex ) 
	: BaseClass( parent, panelName )
{
	m_nNodeIndex = nodeIndex;
	m_bSelected = false;
	m_bHovered = false;
	m_NodeColor = Color( 255, 255, 255, 255 );
	
	SetSize( 16, 16 );
	SetZPos( 100 );
	SetPaintBackgroundEnabled( false );
}

CTC2SpawnNodePanel::~CTC2SpawnNodePanel()
{
}

void CTC2SpawnNodePanel::SetNodeData( const TCSpawnNode_t &node )
{
	m_Node = node;
	
	// Set color based on node type
	switch ( node.eType )
	{
	case TCSPAWN_TEAMSPAWN:
		m_NodeColor = Color( 100, 255, 100, 255 ); // Green
		break;
	case TCSPAWN_TELEPORTER:
		m_NodeColor = Color( 100, 150, 255, 255 ); // Blue
		break;
	case TCSPAWN_CONTROLPOINT:
		m_NodeColor = Color( 255, 200, 50, 255 ); // Gold
		break;
	default:
		m_NodeColor = Color( 255, 255, 255, 255 );
		break;
	}
}

void CTC2SpawnNodePanel::Paint()
{
	BaseClass::Paint();
	
	int w, h;
	GetSize( w, h );
	
	Color drawColor = m_NodeColor;

	// Unavailable nodes: dim & cross out
	if ( !m_Node.bAvailable )
	{
		drawColor = Color( 80, 80, 80, 160 );
	}
	
	if ( m_bSelected )
	{
		// Selected: bright white outline
		surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );
		surface()->DrawOutlinedRect( 0, 0, w, h );
		surface()->DrawOutlinedRect( 1, 1, w - 1, h - 1 );
	}
	else if ( m_bHovered )
	{
		// Hovered: lighter version
		drawColor = Color( 
			Min( drawColor.r() + 50, 255 ),
			Min( drawColor.g() + 50, 255 ),
			Min( drawColor.b() + 50, 255 ),
			255
		);
	}
	
	// Draw filled rect for spawn point (placeholder; circle sprite later)
	surface()->DrawSetColor( drawColor );
	surface()->DrawFilledRect( 2, 2, w-2, h-2 );

	if ( !m_Node.bAvailable )
	{
		surface()->DrawSetColor( Color( 200, 0, 0, 200 ) );
		surface()->DrawLine( 2, 2, w-2, h-2 );
		surface()->DrawLine( w-2, 2, 2, h-2 );
	}
	
	// Draw outline
	surface()->DrawSetColor( Color( 0, 0, 0, 200 ) );
	surface()->DrawOutlinedRect( 0, 0, w, h );
}

void CTC2SpawnNodePanel::OnCursorEntered()
{
	BaseClass::OnCursorEntered();
	m_bHovered = true;
}

void CTC2SpawnNodePanel::OnCursorExited()
{
	BaseClass::OnCursorExited();
	m_bHovered = false;
}

void CTC2SpawnNodePanel::OnMousePressed( MouseCode code )
{
	if ( code == MOUSE_LEFT )
	{
		if ( !m_Node.bAvailable )
			return; // ignore clicks on blocked nodes
		// Notify parent that this node was selected
		PostMessage( GetParent()->GetParent(), new KeyValues( "SpawnNodeSelected", "nodeIndex", m_nNodeIndex ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Main spawn selection panel constructor
//-----------------------------------------------------------------------------
CTC2SpawnSelectionPanel::CTC2SpawnSelectionPanel( Panel *parent )
	: BaseClass( parent, "TC2SpawnSelection" )
{
	m_nSelectedNode = -1;

	SetTitle( "", true );
	SetSizeable( false );
	SetMoveable( false );
	SetCloseButtonVisible( false );
	SetProportional( true );
	
	m_pMapPanel = new EditablePanel( this, "MapPanel" );
	m_pTitleLabel = new Label( this, "TitleLabel", "Select your deployment:" );
	m_pConfirmButton = new Button( this, "ConfirmButton", "Spawn", this, "confirm" );
	m_pCancelButton = new Button( this, "CancelButton", "Cancel", this, "cancel" );

	m_pMapOverview = new CMapOverview( "overview", this );
	char mapname[MAX_MAP_NAME];
	Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname ) );
	m_pMapOverview->SetMap( mapname );
	m_pMapOverview->SetMode( CMapOverview::MAP_MODE_FULL );
	
	m_pConfirmButton->SetEnabled( false );
	
	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
	
	ListenForGameEvent( "teamplay_round_start" );
	ListenForGameEvent( "player_spawn" );
}

CTC2SpawnSelectionPanel::~CTC2SpawnSelectionPanel()
{
	m_NodePanels.PurgeAndDeleteElements();
}

void CTC2SpawnSelectionPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	LoadControlSettings( "Resource/UI/TC2SpawnSelection.res" );
	
	SetBgColor( pScheme->GetColor( "SpawnSelectionBG", Color( 46, 43, 42, 255 ) ) );
}

void CTC2SpawnSelectionPanel::PerformLayout()
{
	BaseClass::PerformLayout();
	
	// Center the panel
	int screenW, screenH;
	surface()->GetScreenSize( screenW, screenH );
	
	int w, h;
	GetSize( w, h );
	
	SetPos( ( screenW - w ) / 2, ( screenH - h ) / 2 );
	
	PositionNodeMarkers();
}

void CTC2SpawnSelectionPanel::OnCommand( const char *command )
{
	if ( FStrEq( command, "confirm" ) )
	{
		SendSpawnSelection();
		// Close this panel and open the class selection for the player's team
		ShowPanel( false );

		C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocal && gViewPortInterface )
		{
			int iTeam = pLocal->GetTeamNumber();
			if ( iTeam == TF_TEAM_RED )
			{
				gViewPortInterface->ShowPanel( PANEL_CLASS_RED, true );
			}
			else if ( iTeam == TF_TEAM_BLUE )
			{
				gViewPortInterface->ShowPanel( PANEL_CLASS_BLUE, true );
			}
		}
	}
	else if ( FStrEq( command, "cancel" ) )
	{
		ShowPanel( false );
		engine->ServerCmd( "menuclosed" );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void CTC2SpawnSelectionPanel::OnThink()
{
	BaseClass::OnThink();
	
	// Update button state
	m_pConfirmButton->SetEnabled( m_nSelectedNode >= 0 );
}

void CTC2SpawnSelectionPanel::ShowPanel( bool bShow )
{
	if ( bShow )
	{
		engine->ServerCmd( "menuopen" );
		UpdateSpawnNodes();
		SetVisible( true );
		MoveToFront();
		RequestFocus();
		SetMouseInputEnabled( true );
		SetKeyBoardInputEnabled( false );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}

void CTC2SpawnSelectionPanel::UpdateSpawnNodes()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;
	
	// Clear existing node panels
	m_NodePanels.PurgeAndDeleteElements();
	m_nSelectedNode = -1;
	
	// Get spawn nodes from player
	const CUtlVector<TCSpawnNode_t> &nodes = pLocalPlayer->GetSpawnNodes();
	
	if ( tf_tc2_spawn_ui_debug.GetBool() )
	{
		Msg( "TC2 Spawn UI: Updating with %d nodes\n", nodes.Count() );
	}
	
	// Create a panel for each spawn node
	for ( int i = 0; i < nodes.Count(); ++i )
	{
		CTC2SpawnNodePanel *pNodePanel = new CTC2SpawnNodePanel( m_pMapPanel, VarArgs( "Node%d", i ), i );
		pNodePanel->SetNodeData( nodes[i] );
		m_NodePanels.AddToTail( pNodePanel );
	}
	
	PositionNodeMarkers();
}

void CTC2SpawnSelectionPanel::SelectSpawnNode( int nodeIndex )
{
	if ( nodeIndex < 0 || nodeIndex >= m_NodePanels.Count() )
		return;
	
	// Deselect all nodes
	for ( int i = 0; i < m_NodePanels.Count(); ++i )
	{
		m_NodePanels[i]->SetSelected( false );
	}
	
	// Select the clicked node
	m_NodePanels[nodeIndex]->SetSelected( true );
	m_nSelectedNode = nodeIndex;
	
	if ( tf_tc2_spawn_ui_debug.GetBool() )
	{
		Msg( "TC2 Spawn UI: Selected node %d\n", nodeIndex );
	}
}

void CTC2SpawnSelectionPanel::OnSpawnNodeSelected( KeyValues* pData )
{
	int nodeIndex = pData->GetInt( "nodeIndex", -1 );
	SelectSpawnNode( nodeIndex );
}

void CTC2SpawnSelectionPanel::PositionNodeMarkers()
{
	// Convert world positions to map panel coordinates.
	// Prefer using the MapOverview's world->map projection if available; otherwise fall back to simple bbox mapping.
	if ( m_NodePanels.Count() <= 0 )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	const CUtlVector<TCSpawnNode_t> &nodes = pLocalPlayer->GetSpawnNodes();
	if ( nodes.Count() <= 0 )
		return;

	// Try to use MapOverview's world->map transform for better accuracy
	CMapOverview *pOverview = dynamic_cast<CMapOverview*>( g_pMapOverview );
	bool bUsedOverview = ( pOverview != NULL );

	Vector2D vMinMap( FLT_MAX, FLT_MAX );
	Vector2D vMaxMap( -FLT_MAX, -FLT_MAX );
	CUtlVector<Vector2D> mapPts;
	mapPts.EnsureCapacity( nodes.Count() );

	int mapW, mapH;
	m_pMapPanel->GetSize( mapW, mapH );

	if ( bUsedOverview )
	{
		for ( int i = 0; i < nodes.Count(); ++i )
		{
			Vector2D mp = pOverview->WorldToMap( nodes[i].vecPosition );
			
			// mp is returned in map pixels. MapOverview standard size is 1024x1024
			float nx = mp.x / 1024.0f;
			float ny = mp.y / 1024.0f;
				
			int panelX = (int)( nx * mapW );
			int panelY = (int)( ny * mapH );

			m_NodePanels[i]->SetPos( panelX - 8, panelY - 8 );
		}
	}
	else
	{
		// Fallback: simple world-space bbox on X/Y
		Vector vMin( FLT_MAX, FLT_MAX, FLT_MAX );
		Vector vMax( -FLT_MAX, -FLT_MAX, -FLT_MAX );
		for ( int i = 0; i < nodes.Count(); ++i )
		{
			const Vector &p = nodes[i].vecPosition;
			vMin.x = MIN( vMin.x, p.x ); vMin.y = MIN( vMin.y, p.y );
			vMax.x = MAX( vMax.x, p.x ); vMax.y = MAX( vMax.y, p.y );
		}
		
		float extentX = MAX( 1.0f, vMax.x - vMin.x );
		float extentY = MAX( 1.0f, vMax.y - vMin.y );

		int margin = 24; // keep markers away from edge
		float usableW = (float)mapW - margin * 2;
		float usableH = (float)mapH - margin * 2;
		
		for ( int i = 0; i < m_NodePanels.Count() && i < nodes.Count(); ++i )
		{
			const Vector &p = nodes[i].vecPosition;
			float nx = ( p.x - vMin.x ) / extentX; // 0..1
			float ny = ( p.y - vMin.y ) / extentY; // 0..1

			int panelX = margin + (int)( nx * usableW );
			int panelY = margin + (int)( (1.0f - ny) * usableH ); // invert Y so top is max Y

			m_NodePanels[i]->SetPos( panelX - 8, panelY - 8 ); // center (marker size 16)
		}
	}

	if ( tf_tc2_spawn_ui_debug.GetBool() )
	{
		if ( bUsedOverview )
		{
			Msg( "TC2 Spawn UI: mapped points using MapOverview bounds to %dx%d\n", mapW, mapH );
		}
		else
		{
			Msg( "TC2 Spawn UI: world bbox (fallback) mapped to %dx%d\n", mapW, mapH );
		}
	}
}

void CTC2SpawnSelectionPanel::SendSpawnSelection()
{
	if ( m_nSelectedNode < 0 || m_nSelectedNode >= m_NodePanels.Count() )
		return;
	
	const TCSpawnNode_t &node = m_NodePanels[m_nSelectedNode]->GetNodeData();
	
	// Send command to server
	engine->ClientCmd( VarArgs( "tc2_select_spawn %.2f %.2f %.2f", node.vecPosition.x, node.vecPosition.y, node.vecPosition.z ) );
	
	if ( tf_tc2_spawn_ui_debug.GetBool() )
	{
		Msg( "TC2 Spawn UI: Sent spawn selection to server\n" );
	}
}

void CTC2SpawnSelectionPanel::FireGameEvent( IGameEvent *event )
{
#if 0
	const char *eventName = event->GetName();
	if ( FStrEq( eventName, "teamplay_round_start" ) || FStrEq( eventName, "player_spawn" ) )
	{
		// Close panel if round starts or player spawns
		ShowPanel( false );
	}
#endif
}
