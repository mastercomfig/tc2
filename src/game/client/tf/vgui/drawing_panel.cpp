//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "drawing_panel.h"
#include "softline.h"
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include "voice_status.h"
#include "c_tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

extern ConVar cl_mute_all_comms;
extern ConVar cl_enable_text_chat;

Color g_DrawPanel_TeamColors[TF_TEAM_COUNT] = 
{
	COLOR_TF_SPECTATOR,		// unassigned
	COLOR_TF_SPECTATOR,		// spectator
	COLOR_TF_RED,			// red
	COLOR_TF_BLUE,			// blue
};

DECLARE_BUILD_FACTORY( CDrawingPanel );

ConVar cl_spec_hud_draw( "cl_spec_hud_draw", "0" );
ConVar cl_spec_hud_draw_show( "cl_spec_hud_draw_show", "1" );
ConVar cl_spec_hud_draw_fade( "cl_spec_hud_draw_fade", "0" );
ConVar cl_spec_hud_draw_dist( "cl_spec_hud_draw_dist", "1024" );
ConVar cl_spec_hud_draw_color( "cl_spec_hud_draw_color", "1", 0, "", true, 0, true, 3 );

CDrawingPanel::CDrawingPanel( Panel *parent, const char*name ) : Panel( parent, name )
{
	m_bDrawingLines = false;
	m_fLastMapLine = 0;
	m_iMouseX = 0;
	m_iMouseY = 0;
	m_iPanelType = DRAWING_PANEL_TYPE_NONE;
	m_bTeamColors = false;
	m_bFading = false;

	m_nWhiteTexture = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile( m_nWhiteTexture, "vgui/white", true, false );

	ListenForGameEvent( "cl_drawline" );
}

void CDrawingPanel::SetVisible( bool bState )
{
	if ( m_iPanelType != DRAWING_PANEL_TYPE_OBSERVER )
	{
		ClearAllLines();
	}

	BaseClass::SetVisible( bState );
}

void CDrawingPanel::ReadColor( const char* pszToken, Color& color )
{
	if ( pszToken && *pszToken )
	{
		int r = 0, g = 0, b = 0, a = 255;
		if ( sscanf( pszToken, "%d %d %d %d", &r, &g, &b, &a ) >= 3 )
		{
			// it's a direct color
			color = Color( r, g, b, a );
		}
		else
		{
			vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
			color = pScheme->GetColor( pszToken, Color( 0, 0, 0, 0 ) );
		}
	}
}

void CDrawingPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	ReadColor( inResourceData->GetString( "linecolor", "" ), m_colorLine );
	m_bTeamColors = inResourceData->GetBool( "team_colors", false );
}

void CDrawingPanel::Paint()
{
	for ( int iIndex = 0; iIndex < ARRAYSIZE( m_vecDrawnLines ); iIndex++ )
	{
		//Draw the lines
		for ( int i = 0; i < m_vecDrawnLines[iIndex].Count(); i++ )
		{
			if ( !m_vecDrawnLines[iIndex][i].bSetBlipCentre )
			{
				Vector2D vecBlipPos;
				// when we're an observer, worldpos is actually in world space. who would've thunk.
				if ( m_iPanelType == DRAWING_PANEL_TYPE_OBSERVER )
				{
					int iBlipX;
					int iBlipY;
					if ( !GetVectorInHudSpace( m_vecDrawnLines[iIndex][i].worldpos, iBlipX, iBlipY ) )
					{
						// skip this line since it's not on screen rn
						continue;
					}
					vecBlipPos.x = iBlipX;
					vecBlipPos.y = iBlipY;
					// in this case, we can't set bSetBlipCentre because it will be different frame to frame
				}
				else
				{
					vecBlipPos.x = m_vecDrawnLines[iIndex][i].worldpos.x;
					vecBlipPos.y = m_vecDrawnLines[iIndex][i].worldpos.y;
					// we don't need to "compute" the pos anymore
					m_vecDrawnLines[iIndex][i].bSetBlipCentre = true;
				}
				//Msg("drawing line with blippos %f, %f\n", vecBlipPos.x, vecBlipPos.y);
				m_vecDrawnLines[iIndex][i].blipcentre = vecBlipPos;
				//Msg( "  which is blipcentre=%f, %f\n", m_vecDrawnLines[iIndex][i].blipcentre.x, m_vecDrawnLines[iIndex][i].blipcentre.y );
			}

			float x = m_vecDrawnLines[iIndex][i].blipcentre.x;
			float y = m_vecDrawnLines[iIndex][i].blipcentre.y;

			if ( m_vecDrawnLines[iIndex][i].bLink )
			{
				if ( i > 1 )
				{
					m_vecDrawnLines[iIndex][i].linkpos = m_vecDrawnLines[iIndex][i - 1].blipcentre;

					if ( !m_vecDrawnLines[iIndex][i].bSetLinkBlipCentre )
					{
						Vector2D vecBlipPos2;
						vecBlipPos2.x = m_vecDrawnLines[iIndex][i].linkpos.x;
						vecBlipPos2.y = m_vecDrawnLines[iIndex][i].linkpos.y;
						m_vecDrawnLines[iIndex][i].linkblipcentre = vecBlipPos2;
						// observer uses world pos, so it's different frame to frame
						if ( m_iPanelType != DRAWING_PANEL_TYPE_OBSERVER )
						{
							m_vecDrawnLines[iIndex][i].bSetLinkBlipCentre = true;
						}
					}

					float x2 = m_vecDrawnLines[iIndex][i].linkblipcentre.x;
					float y2 = m_vecDrawnLines[iIndex][i].linkblipcentre.y;

 					int alpha = 255;
					if ( m_iPanelType == DRAWING_PANEL_TYPE_MATCH_SUMMARY || m_iPanelType == DRAWING_PANEL_TYPE_OBSERVER )
					{
						float t = gpGlobals->curtime - m_vecDrawnLines[iIndex][i].created_time;
					
						if ( t < DRAWN_LINE_SOLID_TIME )
						{
						}
						else if ( t < DRAWN_LINE_SOLID_TIME + DRAWN_LINE_FADE_TIME )
						{
							alpha = 255 - ( ( t - DRAWN_LINE_SOLID_TIME ) / DRAWN_LINE_FADE_TIME ) * 255.0f;
						}
						else
						{
							continue;
						}
					}

					//Msg("drawing line from %f,%f to %f,%f\n", x, y, x2, y2);

					vgui::surface()->DrawSetTexture( m_nWhiteTexture );
					vgui::Vertex_t start, end;

					Color drawColor = m_colorLine;
					int iColorDrawIndex = -1;
					if ( m_iPanelType == DRAWING_PANEL_TYPE_OBSERVER )
					{
						iColorDrawIndex = m_vecDrawnLines[iIndex][i].iColorIndex == 0 ? -1 : m_vecDrawnLines[iIndex][i].iColorIndex;
					}
					else if ( m_bTeamColors )
					{
						iColorDrawIndex = 0;
						C_BasePlayer* pPlayer = UTIL_PlayerByIndex( iIndex );
						if ( pPlayer )
						{
							iColorDrawIndex = pPlayer->GetTeamNumber();
						}
					}
					if ( iColorDrawIndex >= 0 )
					{
						drawColor = g_DrawPanel_TeamColors[iColorDrawIndex];
					}

					// draw main line
					vgui::surface()->DrawSetColor( Color( drawColor.r(), drawColor.g(), drawColor.b(), alpha ) );
					start.Init( Vector2D( x, y ), Vector2D( 0, 0 ) );
					end.Init( Vector2D( x2, y2 ), Vector2D( 1, 1 ) );
					SoftLine::DrawPolygonLine( start, end );

					// draw translucent ones around it to give it some softness	
					vgui::surface()->DrawSetColor( Color( drawColor.r(), drawColor.g(), drawColor.b(), 0.5f * alpha ) );

					start.Init( Vector2D( x - 0.50f, y - 0.50f ), Vector2D( 0, 0 ) );
					end.Init( Vector2D( x2 - 0.50f, y2 - 0.50f ), Vector2D( 1, 1 ) );
					SoftLine::DrawPolygonLine( start, end );

					start.Init( Vector2D( x + 0.50f, y - 0.50f ), Vector2D( 0, 0 ) );
					end.Init( Vector2D( x2 + 0.50f, y2 - 0.50f ), Vector2D( 1, 1 ) );
					SoftLine::DrawPolygonLine( start, end );

					start.Init( Vector2D( x - 0.50f, y + 0.50f ), Vector2D( 0, 0 ) );
					end.Init( Vector2D( x2 - 0.50f, y2 + 0.50f ), Vector2D( 1, 1 ) );
					SoftLine::DrawPolygonLine( start, end );

					start.Init( Vector2D( x + 0.50f, y + 0.50f ), Vector2D( 0, 0 ) );
					end.Init( Vector2D( x2 + 0.50f, y2 + 0.50f ), Vector2D( 1, 1 ) );
					SoftLine::DrawPolygonLine( start, end );
				}
			}
		}
	}
}

void CDrawingPanel::OnMousePressed( vgui::MouseCode code )
{
	if ( m_iPanelType == DRAWING_PANEL_TYPE_OBSERVER )
		return;

	if ( code != MOUSE_LEFT )
		return;

	SendMapLine( m_iMouseX, m_iMouseY, 0.0f, true );
	m_bDrawingLines = true;
}

void CDrawingPanel::OnMouseReleased( vgui::MouseCode code )
{
	if ( m_iPanelType == DRAWING_PANEL_TYPE_OBSERVER )
		return;
	
	if ( code != MOUSE_LEFT )
		return;

	m_bDrawingLines = false;
}

void CDrawingPanel::OnCursorExited()
{
	if ( m_iPanelType == DRAWING_PANEL_TYPE_OBSERVER )
		return;
	//Msg("CDrawingPanel::OnCursorExited\n");
	m_bDrawingLines = false;
}

void CDrawingPanel::OnCursorMoved( int x, int y )
{
	if ( m_iPanelType == DRAWING_PANEL_TYPE_OBSERVER )
		return;

	//Msg("CDrawingPanel::OnCursorMoved %d,%d\n", x, y);
	m_iMouseX = x;
	m_iMouseY = y;

	const float flLineInterval = 1.f / 60.f;

	if ( m_bDrawingLines && gpGlobals->curtime >= m_fLastMapLine + flLineInterval )
	{
		SendMapLine( x, y, 0.0f, false );
	}
}

void CDrawingPanel::OnThink()
{
	const int iLocalIndex = GetLocalPlayerIndex();
	const bool bLocalIndexValid = IsIndexIntoPlayerArrayValid( iLocalIndex );
	if ( m_iPanelType == DRAWING_PANEL_TYPE_OBSERVER )
	{
		if ( cl_spec_hud_draw_fade.GetBool() && !m_bFading && bLocalIndexValid )
		{
			m_bFading = true;

			cl_spec_hud_draw.SetValue( 0 );
			
			// make sure every line starts fading now
			FOR_EACH_VEC_BACK( m_vecDrawnLines[iLocalIndex], i )
			{
				if ( gpGlobals->curtime - m_vecDrawnLines[iLocalIndex][i].created_time < DRAWN_LINE_SOLID_TIME )
				{
					m_vecDrawnLines[iLocalIndex][i].created_time = gpGlobals->curtime - DRAWN_LINE_SOLID_TIME;
				}
			}
		}
	}
	
	if ( m_iPanelType == DRAWING_PANEL_TYPE_MATCH_SUMMARY || m_iPanelType == DRAWING_PANEL_TYPE_OBSERVER )
	{
		// clean up any segments that have faded out
		
		for ( int iIndex = 0; iIndex < ARRAYSIZE( m_vecDrawnLines ); iIndex++ )
		{
			FOR_EACH_VEC_BACK( m_vecDrawnLines[iIndex], i )
			{
				if ( gpGlobals->curtime - m_vecDrawnLines[iIndex][i].created_time > DRAWN_LINE_SOLID_TIME + DRAWN_LINE_FADE_TIME )
				{
					m_vecDrawnLines[iIndex].Remove( i );
				}
			}
		}
	}

	if ( m_iPanelType == DRAWING_PANEL_TYPE_OBSERVER )
	{
		// we've stopped fading
		if ( m_bFading && bLocalIndexValid && m_vecDrawnLines[iLocalIndex].IsEmpty() )
		{
			cl_spec_hud_draw_fade.SetValue( 0 );
			m_bFading = false;
		}
		const float flLineInterval = 1.f / 60.f;
		if ( cl_spec_hud_draw.GetBool() && cl_spec_hud_draw_show.GetBool() && !m_bFading )
		{
			if ( gpGlobals->curtime >= m_fLastMapLine + flLineInterval )
			{
				C_TFPlayer* pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( pLocalTFPlayer )
				{
					Vector vecEye = pLocalTFPlayer->EyePosition();
					Vector vecForward;
					pLocalTFPlayer->EyeVectors( &vecForward );

					trace_t tr;
					UTIL_TraceLine( vecEye, vecEye + ( vecForward * cl_spec_hud_draw_dist.GetFloat() ), MASK_PLAYERSOLID_BRUSHONLY, pLocalTFPlayer, COLLISION_GROUP_NONE, &tr );
					SendMapLine( tr.endpos.x, tr.endpos.y, tr.endpos.z, !m_bDrawingLines );
					m_bDrawingLines = true;
				}
			}
		}
		else if ( m_bDrawingLines )
		{
			m_bDrawingLines = false;
		}
	}
}

void CDrawingPanel::SendMapLine( float x, float y, float z, bool bInitial )
{
	if ( engine->IsPlayingDemo() )
		return;

	int iIndex = GetLocalPlayerIndex();
	
	if (!IsIndexIntoPlayerArrayValid(iIndex))
		return;
	
	int nMaxLines = 900; // 15 seconds of drawing at 60fps
	if ( m_iPanelType == DRAWING_PANEL_TYPE_MATCH_SUMMARY )
	{
		nMaxLines = 150; // 2.5 seconds of drawing at 60fps
	}

	// Stop adding lines after this much
	if ( m_vecDrawnLines[iIndex].Count() >= nMaxLines )
		return;

	int linetype = bInitial ? 0 : 1;

	m_fLastMapLine = gpGlobals->curtime;

	// short circuit add it to your own list
	MapLine line;
	line.worldpos.x = x;
	line.worldpos.y = y;
	line.worldpos.z = z;
	line.created_time = gpGlobals->curtime;
	if ( linetype == 1 )		// links to a previous
	{
		line.bLink = true;
	}

	if ( m_iPanelType == DRAWING_PANEL_TYPE_OBSERVER )
	{
		line.iColorIndex = cl_spec_hud_draw_color.GetInt();
	}

	m_vecDrawnLines[iIndex].AddToTail( line );

	if ( m_iPanelType == DRAWING_PANEL_TYPE_MATCH_SUMMARY )
	{
		// notify the server of this!
		KeyValues *kv = new KeyValues( "cl_drawline" );
		kv->SetInt( "panel", m_iPanelType );
		kv->SetInt( "line", linetype );
		kv->SetFloat( "x", x / (float)GetWide() );
		kv->SetFloat( "y", y / (float)GetTall() );
		engine->ServerCmdKeyValues( kv );
	}
}

void CDrawingPanel::ClearLines( int iIndex )
{
	if (!IsIndexIntoPlayerArrayValid(iIndex))
		return;

	m_vecDrawnLines[iIndex].Purge();
}

void CDrawingPanel::ClearAllLines()
{
	for ( int iIndex = 0; iIndex < ARRAYSIZE( m_vecDrawnLines ); iIndex++ )
	{
		m_vecDrawnLines[iIndex].Purge();
	}
}

void CDrawingPanel::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "cl_drawline" ) )
	{
		int iIndex = event->GetInt( "player" );
		
		// if this is NOT the local player (we've already stored our own data)
		if ( ( iIndex != GetLocalPlayerIndex() ) || engine->IsPlayingDemo() )
		{
			if ( !cl_enable_text_chat.GetBool() )
			{
				ClearLines( iIndex );
				return;
			}

			// If a player is muted for voice, also mute them for lines because jerks gonna jerk.
			if ( cl_mute_all_comms.GetBool() && ( iIndex != 0 ) )
			{
				if ( GetClientVoiceMgr() && GetClientVoiceMgr()->IsPlayerBlocked( iIndex ) )
				{
					ClearLines( iIndex );
					return;
				}
			}

			int iPanelType = event->GetInt( "panel" );

			// if this message is about our panel type
			if ( iPanelType == m_iPanelType )
			{
				int iLineType = event->GetInt( "line" );
				float x = event->GetFloat( "x" );
				float y = event->GetFloat( "y" );

				MapLine line;
				line.worldpos.x = (int)( x * GetWide() );
				line.worldpos.y = (int)( y * GetTall() );
				line.created_time = gpGlobals->curtime;
				if ( iLineType == 1 )		// links to a previous
				{
					line.bLink = true;
				}
				
				if (!IsIndexIntoPlayerArrayValid(iIndex))
					return;

				m_vecDrawnLines[iIndex].AddToTail( line );
			}
		}
	}
}