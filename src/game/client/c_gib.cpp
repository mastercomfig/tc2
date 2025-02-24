//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-side gib implementation
//
//=============================================================================

#include "cbase.h"
#include "vcollide_parse.h"
#include "c_gib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// NOTENOTE: This is not yet coupled with the server-side implementation of CGib
//           This is only a client-side version of gibs at the moment

//-----------------------------------------------------------------------------
// Purpose: Destructor - cleans up the physics object.
//-----------------------------------------------------------------------------
C_Gib::~C_Gib(void)
{
    VPhysicsDestroyObject();
}

//-----------------------------------------------------------------------------
// Purpose: Factory function to create a client-side gib.
//-----------------------------------------------------------------------------
C_Gib* C_Gib::CreateClientsideGib(const char* pszModelName,
    const Vector& vecOrigin,
    const Vector& vecForceDir,
    AngularImpulse vecAngularImp,
    float flLifetime)
{
    C_Gib* pGib = new C_Gib();
    if (!pGib)
        return nullptr;

    if (!pGib->InitializeGib(pszModelName, vecOrigin, vecForceDir, vecAngularImp, flLifetime))
    {
        pGib->Release();
        return nullptr;
    }

    return pGib;
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the gib as a client entity and sets up physics.
//-----------------------------------------------------------------------------
bool C_Gib::InitializeGib(const char* pszModelName,
    const Vector& vecOrigin,
    const Vector& vecForceDir,
    AngularImpulse vecAngularImp,
    float flLifetime)
{
    // Initialize as a client entity; if this fails, clean up.
    if (!InitializeAsClientEntity(pszModelName, RENDER_GROUP_OPAQUE_ENTITY))
    {
        Release();
        return false;
    }

    SetAbsOrigin(vecOrigin);
    SetCollisionGroup(COLLISION_GROUP_DEBRIS);

    // Parse the physics model.
    solid_t tmpSolid;
    PhysModelParseSolid(tmpSolid, this, GetModelIndex());

    // Create and initialize the physics object.
    m_pPhysicsObject = VPhysicsInitNormal(SOLID_VPHYSICS, 0, false, &tmpSolid);
    if (m_pPhysicsObject)
    {
        // Scale the force direction by the mass of the physics object.
        float flForce = m_pPhysicsObject->GetMass();
        Vector scaledForce = vecForceDir * flForce;

        // Cache the absolute origin.
        const Vector absOrigin = GetAbsOrigin();
        m_pPhysicsObject->ApplyForceOffset(scaledForce, absOrigin);

        // Enable global touch callbacks.
        m_pPhysicsObject->SetCallbackFlags(m_pPhysicsObject->GetCallbackFlags() |
            CALLBACK_GLOBAL_TOUCH | CALLBACK_GLOBAL_TOUCH_STATIC);
    }
    else
    {
        // Failed to create a physics object; clean up.
        Release();
        return false;
    }

    // Schedule the gib for removal after its lifetime expires.
    SetNextClientThink(gpGlobals->curtime + flLifetime);

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Handles client-side thinking for fading out the gib.
//-----------------------------------------------------------------------------
void C_Gib::ClientThink(void)
{
    SetRenderMode(kRenderTransAlpha);
    m_nRenderFX = kRenderFxFadeFast;

    // If the gib is fully faded out, remove it.
    if (m_clrRender->a == 0)
    {
#ifdef HL2_CLIENT_DLL
        s_AntlionGibManager.RemoveGib(this);
#endif
        Release();
        return;
    }

    // Cache current time to avoid multiple global accesses.
    const float curTime = gpGlobals->curtime;
    SetNextClientThink(curTime + 1.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Processes a collision with another entity, with a delay to prevent rapid repeats.
//-----------------------------------------------------------------------------
void C_Gib::StartTouch(C_BaseEntity* pOther)
{
    // Cache current time.
    const float curTime = gpGlobals->curtime;
    constexpr float TOUCH_DELAY = 0.1f;
    if (m_flTouchDelta < curTime)
    {
        HitSurface(pOther);
        m_flTouchDelta = curTime + TOUCH_DELAY;
    }

    BaseClass::StartTouch(pOther);
}

//-----------------------------------------------------------------------------
// Purpose: Handles effects when the gib hits a surface.
//          (Child classes can implement splatter effects or similar.)
//-----------------------------------------------------------------------------
void C_Gib::HitSurface(C_BaseEntity* pOther)
{
    // TODO: Implement splatter or other effects in derived classes.
}
