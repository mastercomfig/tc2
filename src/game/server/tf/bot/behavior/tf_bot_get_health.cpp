//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_get_health.h
// Pick up any nearby health kit
// Michael Booth, May 2009

#include "cbase.h"
#include "tf_gamerules.h"
#include "tf_obj.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_get_health.h"

extern ConVar tf_bot_path_lookahead_range;

ConVar tf_bot_health_critical_ratio( "tf_bot_health_critical_ratio", "0.3", FCVAR_CHEAT );
ConVar tf_bot_health_ok_ratio( "tf_bot_health_ok_ratio", "0.8", FCVAR_CHEAT );
ConVar tf_bot_health_search_near_range( "tf_bot_health_search_near_range", "1000", FCVAR_CHEAT );
ConVar tf_bot_health_search_far_range( "tf_bot_health_search_far_range", "2000", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
class CHealthFilter : public INextBotFilter
{
public:
	CHealthFilter( CTFBot *me )
	{
		m_me = me;
		m_healthArea = NULL;
	}

	bool IsSelected( const CBaseEntity *constCandidate ) const
	{
		if ( !constCandidate )
			return false;

		CBaseEntity *candidate = const_cast< CBaseEntity * >( constCandidate );

		m_healthArea = (CTFNavArea *)TheNavMesh->GetNearestNavArea( candidate->WorldSpaceCenter() );
		if ( !m_healthArea )
			return false;

		CClosestTFPlayer close( candidate );
		ForEachPlayer( close );

		// if the closest player to this candidate object is an enemy, don't use it
		if ( close.m_closePlayer && !m_me->InSameTeam( close.m_closePlayer ) )
			return false;

		// resupply cabinets (not assigned a team)
		if ( candidate->ClassMatches( "func_regenerate" ) )
		{
			if ( !m_healthArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE | TF_NAV_SPAWN_ROOM_RED ) )
			{
				// Assume any resupply cabinets not in a teamed spawn room are inaccessible.
				// Ex: pl_upward has forward spawn rooms that neither team can use until 
				// certain checkpoints are reached.
				return false;
			}

			if ( ( m_me->GetTeamNumber() == TF_TEAM_RED && m_healthArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_RED ) ) ||
			     ( m_me->GetTeamNumber() == TF_TEAM_BLUE && m_healthArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE ) ) )
			{
				// the supply cabinet is in my spawn room
				return true;
			}

			return false;
		}

		// ignore non-existent ammo to ensure we collect nearby existing ammo
		if ( candidate->IsEffectActive( EF_NODRAW ) )
			return false;

		if ( candidate->ClassMatches( "item_healthkit*" ) )
			return true;

		if ( m_me->InSameTeam( candidate ) )
		{
			// friendly engineer's dispenser
			if ( candidate->ClassMatches( "obj_dispenser*" ) )
			{
				CBaseObject	*dispenser = (CBaseObject *)candidate;
				if ( !dispenser->IsBuilding() && !dispenser->IsPlacing() && !dispenser->IsDisabled() )
				{
					return true;
				}
			}
		}

		return false;
	}

	CTFBot *m_me;
	mutable CTFNavArea *m_healthArea;
};


//---------------------------------------------------------------------------------------------
static CTFBot *s_possibleBot = NULL;
static CHandle< CBaseEntity > s_possibleHealth = NULL;
static int s_possibleFrame = 0;


//---------------------------------------------------------------------------------------------
/** 
 * Return true if this Action has what it needs to perform right now
 */
bool CTFBotGetHealth::IsPossible( CTFBot *me )
{
	VPROF_BUDGET( "CTFBotGetHealth::IsPossible", "NextBot" );

	// don't move to fetch health if we have a healer
	if ( me->m_Shared.GetNumHealers() > 0 )
		return false;

#ifdef TF_RAID_MODE
	// mobs don't heal
	if ( TFGameRules()->IsRaidMode() && me->HasAttribute( CTFBot::AGGRESSIVE ) )
	{
		return false;
	}
#endif // TF_RAID_MODE

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		return false;
	}

	float healthRatio = ( float )me->GetHealth() / ( float )me->GetMaxHealth();

	// even if i'm burning, if i'm overhealed then don't be so stingy.
	if ( healthRatio > 1.0f )
	{
		return false;
	}

	float t = ( healthRatio - tf_bot_health_critical_ratio.GetFloat() ) / ( tf_bot_health_ok_ratio.GetFloat() - tf_bot_health_critical_ratio.GetFloat() );
	t = clamp( t, 0.0f, 1.0f );

	if ( me->m_Shared.InCond( TF_COND_BURNING ) || me->m_Shared.InCond( TF_COND_BLEEDING ) )
	{
		// taking DoT - get health now
		t = 0.0f;
	}

	if ( t >= 1.0f - FLT_EPSILON )
	{
		return false;
	}

	// the more we are hurt, the farther we'll travel to get health
	float searchRange = tf_bot_health_search_far_range.GetFloat() + t * ( tf_bot_health_search_near_range.GetFloat() - tf_bot_health_search_far_range.GetFloat() );

	CUtlVector<CNavArea*> nearbyAreaVector;
	CollectSurroundingAreas( &nearbyAreaVector, me->GetLastKnownArea(), searchRange, me->GetLocomotionInterface()->GetStepHeight(), me->GetLocomotionInterface()->GetDeathDropHeight() );

	CHealthFilter healthFilter( me );
	
	const CUtlVector<CHandle<CBaseEntity>>& staticHealthVector = TFGameRules()->GetHealthEntityVector();
	CBaseEntity* closestHealth = NULL;
	float closestHealthTravelDistance = FLT_MAX;

	if ( staticHealthVector.Count() == 0 )
	{
		if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
		{
			Warning( "%3.2f: No health nearby\n", gpGlobals->curtime );
		}
		return false;
	}

	for ( int i = 0; i < staticHealthVector.Count(); ++i )
	{
		CBaseEntity* health = staticHealthVector[i];
		if ( health )
		{
			if ( healthFilter.IsSelected( health ) )
			{
				if ( healthFilter.m_healthArea && healthFilter.m_healthArea->IsMarked() )
				{
					// "cost so far" was computed during the breadth first search within CollectSurroundingAreas()
					// and is the travel distance from to this area
					if ( healthFilter.m_healthArea->GetCostSoFar() < closestHealthTravelDistance )
					{
						closestHealth = health;
						closestHealthTravelDistance = healthFilter.m_healthArea->GetCostSoFar();
					}
				}
			}
		}
	}

	if ( !closestHealth )
	{
		if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
		{
			Warning( "%3.2f: No health nearby\n", gpGlobals->curtime );
		}
		return false;
	}

	s_possibleBot = me;
	s_possibleHealth = closestHealth;
	s_possibleFrame = gpGlobals->framecount;

	return true;
}

//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotGetHealth::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	VPROF_BUDGET( "CTFBotGetHealth::OnStart", "NextBot" );

	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	// if IsPossible() has already been called, use its cached data
	if ( s_possibleFrame != gpGlobals->framecount || s_possibleBot != me )
	{
		if ( !IsPossible( me ) || s_possibleHealth == NULL )
		{
			return Done( "Can't get health" );
		}
	}

	m_healthKit = s_possibleHealth;
	m_isGoalDispenser = m_healthKit->ClassMatches( "obj_dispenser*" );

	CTFBotPathCost cost( me, SAFEST_ROUTE );
	if ( !m_path.Compute( me, m_healthKit->WorldSpaceCenter(), cost ) )
	{
		return Done( "No path to health!" );
	}

	// if I'm a spy, cloak and disguise
	if ( me->IsPlayerClass( TF_CLASS_SPY ) )
	{
		if ( !me->m_Shared.IsStealthed() )
		{
			me->PressAltFireButton();
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotGetHealth::Update( CTFBot *me, float interval )
{
	if ( m_healthKit == NULL || ( m_healthKit->IsEffectActive( EF_NODRAW ) && !FClassnameIs( m_healthKit, "func_regenerate" ) ) )
	{
		return Done( "Health kit I was going for has been taken" );
	}

	// if a medic is healing us, give up on getting a kit
	int i;
	for( i=0; i<me->m_Shared.GetNumHealers(); ++i )
	{
		if ( !me->m_Shared.HealerIsDispenser( i ) )
			break;
	}

	if ( i < me->m_Shared.GetNumHealers() )
	{
		return Done( "A Medic is healing me" );
	}

	if ( me->m_Shared.GetNumHealers() )
	{
		// a dispenser is healing me, don't wait if I'm in combat
		const CKnownEntity *known = me->GetVisionInterface()->GetPrimaryKnownThreat();
		if ( known && known->IsVisibleInFOVNow() )
		{
			return Done( "No time to wait for health, I must fight" );
		}
	}

	if ( me->GetHealth() >= me->GetMaxHealth() )
	{
		return Done( "I've been healed" );
	}

/* TODO: Rethink this. Currently creates zombie behavior loop.
	// if the closest player to the item we're after is an enemy, give up
	CClosestTFPlayer close( m_healthKit );
	ForEachPlayer( close );
	if ( close.m_closePlayer && !me->InSameTeam( close.m_closePlayer ) )
		return Done( "An enemy is closer to it" );
*/

	if ( !m_path.IsValid() )
	{
		// this can occur if we overshoot the health kit's location
		// because it is momentarily gone
		CTFBotPathCost cost( me, SAFEST_ROUTE );
		if ( !m_path.Compute( me, m_healthKit->WorldSpaceCenter(), cost ) )
		{
			return Done( "No path to health!" );
		}
	}

	// un-zoom
	CTFWeaponBase *myWeapon = me->m_Shared.GetActiveTFWeapon();
	if ( myWeapon && myWeapon->IsWeapon( TF_WEAPON_SNIPERRIFLE ) && me->m_Shared.InCond( TF_COND_ZOOMED ) )
		me->PressAltFireButton();

	// may need to switch weapons (ie: engineer holding toolbox now needs to heal and defend himself)
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	m_path.Update( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotGetHealth::OnStuck( CTFBot *me )
{
	return TryDone( RESULT_CRITICAL, "Stuck trying to reach health kit" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotGetHealth::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotGetHealth::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType reason )
{
	return TryDone( RESULT_CRITICAL, "Failed to reach health kit" );
}


//---------------------------------------------------------------------------------------------
// We are always hurrying if we need to collect health
QueryResultType CTFBotGetHealth::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}
