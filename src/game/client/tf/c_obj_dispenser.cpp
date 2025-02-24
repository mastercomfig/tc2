//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's CObjectDispenser with improved clarity and performance
//
//=============================================================================//

#include "cbase.h"
#include "c_baseobject.h"
#include "c_tf_player.h"
#include "vgui/ILocalize.h"
#include "c_obj_dispenser.h"

// NVNT haptics system interface
#include "c_tf_haptics.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Team's player UtlVector to entindexes
//-----------------------------------------------------------------------------
void RecvProxy_HealingList(const CRecvProxyData* pData, void* pStruct, void* pOut)
{
    auto* pDispenser = static_cast<C_ObjectDispenser*>(pStruct);
    // Get pointer to the element we want to update.
    CBaseHandle* pHandle = &pDispenser->m_hHealingTargets[pData->m_iElement];
    RecvProxy_IntToEHandle(pData, pStruct, pHandle);

    // Flag that healing targets need an update.
    pDispenser->m_bUpdateHealingTargets = true;
}

void RecvProxyArrayLength_HealingArray(void* pStruct, int objectID, int currentArrayLength)
{
    auto* pDispenser = static_cast<C_ObjectDispenser*>(pStruct);
    if (pDispenser->m_hHealingTargets.Size() != currentArrayLength)
    {
        pDispenser->m_hHealingTargets.SetSize(currentArrayLength);
    }

    // Flag that healing targets need an update.
    pDispenser->m_bUpdateHealingTargets = true;
}

//-----------------------------------------------------------------------------
// Purpose: Dispenser object client class
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT(C_ObjectDispenser, DT_ObjectDispenser, CObjectDispenser)
RecvPropInt(RECVINFO(m_iState)),
RecvPropInt(RECVINFO(m_iAmmoMetal)),
RecvPropInt(RECVINFO(m_iMiniBombCounter)),
RecvPropArray2(
    RecvProxyArrayLength_HealingArray,
    RecvPropInt("healing_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_HealingList),
    MAX_PLAYERS,
    0,
    "healing_array"
)
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
C_ObjectDispenser::C_ObjectDispenser()
    : m_bUpdateHealingTargets(false),
    m_bPlayingSound(false)
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
C_ObjectDispenser::~C_ObjectDispenser()
{
    StopSound("Building_Dispenser.Heal");

    // If a healing sound was active, adjust the haptics system count.
    if (m_bPlayingSound && tfHaptics.healingDispenserCount > 0)
    {
        --tfHaptics.healingDispenserCount;
        if (tfHaptics.healingDispenserCount == 0 && !tfHaptics.wasBeingHealedMedic)
        {
            tfHaptics.isBeingHealed = false;
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: Called when data is updated; refresh healing effects if needed.
//-----------------------------------------------------------------------------
void C_ObjectDispenser::OnDataChanged(DataUpdateType_t updateType)
{
    BaseClass::OnDataChanged(updateType);

    if (m_bUpdateHealingTargets)
    {
        UpdateEffects();
        m_bUpdateHealingTargets = false;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Perform regular client think processing.
//-----------------------------------------------------------------------------
void C_ObjectDispenser::ClientThink()
{
    BaseClass::ClientThink();
}

//-----------------------------------------------------------------------------
// Purpose: Adjust invisibility and update effects when entering or exiting invisibility.
//-----------------------------------------------------------------------------
void C_ObjectDispenser::SetInvisibilityLevel(float flValue)
{
    if (IsEnteringOrExitingFullyInvisible(flValue))
    {
        UpdateEffects();
    }

    BaseClass::SetInvisibilityLevel(flValue);
}

//-----------------------------------------------------------------------------
// Purpose: Create or update healing beam effects for valid targets.
//-----------------------------------------------------------------------------
void C_ObjectDispenser::UpdateEffects(void)
{
    C_TFPlayer* pOwner = GetOwner();

    // If the dispenser or its owner is fully invisible, remove all effects.
    if (GetInvisibilityLevel() == 1.f || (pOwner && pOwner->m_Shared.IsFullyInvisible()))
    {
        StopEffects(true);
        return;
    }

    // First, remove any effects that no longer apply.
    StopEffects();

    // Process each healing target.
    for (int i = 0; i < m_hHealingTargets.Count(); ++i)
    {
        C_BaseEntity* pTarget = m_hHealingTargets[i].Get();
        if (!pTarget)
            continue;

        // Skip targets that are stealthed (for spies).
        if (auto* pPlayer = dynamic_cast<C_TFPlayer*>(pTarget))
        {
            if (pPlayer->m_Shared.IsStealthed() || pPlayer->m_Shared.InCond(TF_COND_STEALTHED_BLINK))
                continue;
        }

        // Check if an effect already exists for this target.
        bool bHaveEffect = false;
        for (int j = 0; j < m_hHealingTargetEffects.Count(); ++j)
        {
            if (m_hHealingTargetEffects[j].pTarget == pTarget)
            {
                bHaveEffect = true;
                break;
            }
        }
        if (bHaveEffect)
            continue;

        // Notify the haptics system if the local player is being healed.
        if (pTarget == C_BasePlayer::GetLocalPlayer())
        {
            ++tfHaptics.healingDispenserCount;
            if (!tfHaptics.wasBeingHealedMedic)
            {
                tfHaptics.isBeingHealed = true;
            }
        }

        // Select the appropriate effect based on team.
        const char* pszEffectName = (GetTeamNumber() == TF_TEAM_RED) ? "dispenser_heal_red" : "dispenser_heal_blue";
        CNewParticleEffect* pEffect = nullptr;

        // Determine attachment type based on whether a model exists.
        if (FBitSet(GetObjectFlags(), OF_DOESNT_HAVE_A_MODEL))
        {
            if (FBitSet(GetObjectFlags(), OF_PLAYER_DESTRUCTION))
            {
                pEffect = ParticleProp()->Create(pszEffectName, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector(0, 0, 50));
            }
            else
            {
                pEffect = ParticleProp()->Create(pszEffectName, PATTACH_ABSORIGIN_FOLLOW);
            }
        }
        else
        {
            pEffect = ParticleProp()->Create(pszEffectName, PATTACH_POINT_FOLLOW, "heal_origin");
        }

        ParticleProp()->AddControlPoint(pEffect, 1, pTarget, PATTACH_ABSORIGIN_FOLLOW, nullptr, Vector(0, 0, 50));

        int effectIndex = m_hHealingTargetEffects.AddToTail();
        m_hHealingTargetEffects[effectIndex].pTarget = pTarget;
        m_hHealingTargetEffects[effectIndex].pEffect = pEffect;

        // Restart the healing sound for each new beam.
        StopSound("Building_Dispenser.Heal");
        CLocalPlayerFilter filter;
        EmitSound(filter, entindex(), "Building_Dispenser.Heal");
        m_bPlayingSound = true;
    }

    // If no healing targets remain, ensure the sound is stopped.
    if (m_bPlayingSound && m_hHealingTargets.Count() == 0)
    {
        m_bPlayingSound = false;
        StopSound("Building_Dispenser.Heal");
    }
}

//-----------------------------------------------------------------------------
// Purpose: Stop healing beam effects, removing those for targets no longer being healed.
//-----------------------------------------------------------------------------
void C_ObjectDispenser::StopEffects(bool bRemoveAll /* = false */)
{
    // Loop backwards to safely remove effects.
    for (int i = m_hHealingTargetEffects.Count() - 1; i >= 0; --i)
    {
        bool bStillHealing = false;
        if (!bRemoveAll)
        {
            for (int j = 0; j < m_hHealingTargets.Count(); ++j)
            {
                if (m_hHealingTargets[j] && m_hHealingTargetEffects[i].pTarget == m_hHealingTargets[j])
                {
                    bStillHealing = true;
                    break;
                }
            }
        }

        if (!bStillHealing)
        {
            // Notify haptics system if the local player is no longer being healed.
            if (m_hHealingTargetEffects[i].pTarget == C_BasePlayer::GetLocalPlayer())
            {
                if (tfHaptics.healingDispenserCount > 0)
                {
                    --tfHaptics.healingDispenserCount;
                    if (tfHaptics.healingDispenserCount == 0 && !tfHaptics.wasBeingHealedMedic)
                    {
                        tfHaptics.isBeingHealed = false;
                    }
                }
            }

            ParticleProp()->StopEmission(m_hHealingTargetEffects[i].pEffect);
            m_hHealingTargetEffects.Remove(i);
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: Update damage effects based on current damage level.
//-----------------------------------------------------------------------------
void C_ObjectDispenser::UpdateDamageEffects(BuildingDamageLevel_t damageLevel)
{
    if (m_hDamageEffects)
    {
        m_hDamageEffects->StopEmission(false, false);
        m_hDamageEffects = nullptr;
    }

    const char* pszEffect = "";
    switch (damageLevel)
    {
    case BUILDING_DAMAGE_LEVEL_LIGHT:    pszEffect = "dispenserdamage_1"; break;
    case BUILDING_DAMAGE_LEVEL_MEDIUM:   pszEffect = "dispenserdamage_2"; break;
    case BUILDING_DAMAGE_LEVEL_HEAVY:     pszEffect = "dispenserdamage_3"; break;
    case BUILDING_DAMAGE_LEVEL_CRITICAL:  pszEffect = "dispenserdamage_4"; break;
    default: break;
    }

    if (Q_strlen(pszEffect) > 0)
    {
        m_hDamageEffects = ParticleProp()->Create(pszEffect, PATTACH_ABSORIGIN);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Return maximum metal ammo for the dispenser.
//-----------------------------------------------------------------------------
int C_ObjectDispenser::GetMaxMetal(void)
{
    return DISPENSER_MAX_METAL_AMMO;
}

//-----------------------------------------------------------------------------
// Control Screen Factories for Dispenser Panels
//-----------------------------------------------------------------------------
DECLARE_VGUI_SCREEN_FACTORY(CDispenserControlPanel, "screen_obj_dispenser_blue");
DECLARE_VGUI_SCREEN_FACTORY(CDispenserControlPanel_Red, "screen_obj_dispenser_red");

//-----------------------------------------------------------------------------
// Constructor: Dispenser Control Panel
//-----------------------------------------------------------------------------
CDispenserControlPanel::CDispenserControlPanel(vgui::Panel* parent, const char* panelName)
    : BaseClass(parent, "CDispenserControlPanel")
{
    m_pAmmoProgress = new RotatingProgressBar(this, "MeterArrow");
}

//-----------------------------------------------------------------------------
// Purpose: Update the panel; disable buttons if ammo is insufficient.
//-----------------------------------------------------------------------------
void CDispenserControlPanel::OnTickActive(C_BaseObject* pObj, C_TFPlayer* pLocalPlayer)
{
    BaseClass::OnTickActive(pObj, pLocalPlayer);

    Assert(dynamic_cast<C_ObjectDispenser*>(pObj));
    m_hDispenser = static_cast<C_ObjectDispenser*>(pObj);

    float flProgress = (m_hDispenser) ? m_hDispenser->GetMetalAmmoCount() / static_cast<float>(m_hDispenser->GetMaxMetal()) : 0.f;
    m_pAmmoProgress->SetProgress(flProgress);
}

//-----------------------------------------------------------------------------
// Purpose: Only show the panel if the dispenser is not fully invisible.
//-----------------------------------------------------------------------------
bool CDispenserControlPanel::IsVisible(void)
{
    if (m_hDispenser && m_hDispenser->GetInvisibilityLevel() == 1.f)
    {
        return false;
    }
    return BaseClass::IsVisible();
}

IMPLEMENT_CLIENTCLASS_DT(C_ObjectCartDispenser, DT_ObjectCartDispenser, CObjectCartDispenser)
END_RECV_TABLE()
