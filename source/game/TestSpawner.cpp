/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/game/TestSpawner.cpp
 *****************************************************************************/

#ifdef _DEBUG

#include "TestSpawner.h"
#include "plugin.h"
#include "common.h"
#include "CCamera.h"
#include "CGeneral.h"
#include "CStreaming.h"
#include "CWorld.h"
#include "CMenuManager.h"
#include "CModelInfo.h"
#include "CTheScripts.h"
#include "CAutomobile.h"
#include "CHeli.h"
#include "CPlane.h"
#include "eCarMission.h"
#include "CStreamingInfo.h"
#include "CRadar.h"
#include "CPlayerPed.h"
#include "CPad.h"
#include <Windows.h>
#include <cmath>
#include <cstdint>

static constexpr int SPAWNABLE_MODELS[] = { 429, 425, 520 };
static constexpr int SPAWNABLE_COUNT = 3;

static bool s_key1WasDown = false;
static bool s_key2WasDown = false;
static bool s_key3WasDown = false;
static bool s_key1NoCtrlWasDown = false;  // Key 1 without Ctrl = teleport
static bool s_keyJWasDown = false;         // J = toggle jetpack
static bool s_modelsPreloaded = false;

static void TeleportToWaypoint()
{
    CPed* ped = FindPlayerPed(0);
    if (!ped)
        return;
    if (!CRadar::ms_RadarTrace)
        return;

    for (unsigned int i = 0; i < MAX_RADAR_TRACES; i++)
    {
        const tRadarTrace& trace = CRadar::ms_RadarTrace[i];
        if (!trace.m_bInUse || trace.m_nRadarSprite != RADAR_SPRITE_WAYPOINT)
            continue;
        if (trace.m_nBlipDisplay == BLIP_DISPLAY_NEITHER)
            continue;

        float wx = trace.m_vecPos.x;
        float wy = trace.m_vecPos.y;
        float groundZ = CWorld::FindGroundZForCoord(wx, wy);
        if (groundZ < -99.0f)
            groundZ = ped->GetPosition().z;
        float wz = groundZ + 1.0f;

        CVector dest(wx, wy, wz);
        if (ped->m_pVehicle)
        {
            ped->m_pVehicle->SetPosn(dest);
            ped->m_pVehicle->m_vecMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
            ped->m_pVehicle->m_vecTurnSpeed = CVector(0.0f, 0.0f, 0.0f);
        }
        else
        {
            ped->SetPosn(dest);
            ped->m_vecMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
        }
        return;
    }
}

static void ToggleJetpack()
{
    // Make sure game is in a safe state
    if (CPad::GetPad(0)->DisablePlayerControls)
        return;
    
    CPed* ped = FindPlayerPed(0);
    if (!ped)
        return;

    // Check if ped is in a vehicle - can't use jetpack in vehicle
    if (ped->m_pVehicle)
        return;


    // Use cheat function which handles both adding and removing jetpack
    // CCheat::JetpackCheat() at 0x439600 (GTA SA 1.0 US)
    // This function internally checks if player already has jetpack and toggles it
    using JetpackCheatFn = void(__stdcall*)();
    static const JetpackCheatFn JetpackCheat = reinterpret_cast<JetpackCheatFn>(0x439600);
    
    JetpackCheat();
}

static CVehicle* SpawnVehicle(int modelIndex, const CVector& position, float heading)
{
    unsigned char oldFlags = CStreaming::ms_aInfoForModel[modelIndex].m_nFlags;
    CStreaming::RequestModel(modelIndex, GAME_REQUIRED);
    CStreaming::LoadAllRequestedModels(false);

    if (CStreaming::ms_aInfoForModel[modelIndex].m_nLoadState != LOADSTATE_LOADED)
        return nullptr;

    if (!(oldFlags & GAME_REQUIRED))
    {
        CStreaming::SetModelIsDeletable(modelIndex);
        CStreaming::SetModelTxdIsDeletable(modelIndex);
    }

    CVehicle* vehicle = nullptr;
    auto* modelInfo = reinterpret_cast<CVehicleModelInfo*>(CModelInfo::ms_modelInfoPtrs[modelIndex]);
    switch (modelInfo->m_nVehicleType)
    {
    case VEHICLE_HELI:
        vehicle = new CHeli(modelIndex, 1);
        break;
    case VEHICLE_PLANE:
        vehicle = new CPlane(modelIndex, 1);
        break;
    default:
        vehicle = new CAutomobile(modelIndex, 1, true);
        break;
    }

    if (!vehicle)
        return nullptr;

    vehicle->SetPosn(position);
    vehicle->SetHeading(heading);
    vehicle->m_nStatus = STATUS_ABANDONED;
    vehicle->m_eDoorLock = DOORLOCK_UNLOCKED;
    vehicle->bEngineOn = false;
    vehicle->m_autoPilot.m_nCarMission = MISSION_NONE;
    vehicle->m_vecMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
    vehicle->m_vecTurnSpeed = CVector(0.0f, 0.0f, 0.0f);

    CWorld::Add(vehicle);
    CTheScripts::ClearSpaceForMissionEntity(position, vehicle);

    if (modelInfo->m_nVehicleType == VEHICLE_HELI || modelInfo->m_nVehicleType == VEHICLE_PLANE)
    {
        vehicle->SetPosn(position);
    }
    else if (modelInfo->m_nVehicleType != VEHICLE_BOAT)
    {
        reinterpret_cast<CAutomobile*>(vehicle)->PlaceOnRoadProperly();
    }

    return vehicle;
}

static void SpawnVehicleInFront(int modelId)
{
    CPed* ped = FindPlayerPed(0);
    if (!ped)
        return;

    CVector camPos = TheCamera.GetPosition();
    CVector camFwd = TheCamera.GetForward();
    float dist = 10.0f;
    CVector pos(camPos.x + camFwd.x * dist, camPos.y + camFwd.y * dist, camPos.z + camFwd.z * dist);
    float groundZ = CWorld::FindGroundZForCoord(pos.x, pos.y);
    if (groundZ > -99.0f)
        pos.z = (pos.z > groundZ + 0.5f) ? pos.z : (groundZ + 0.5f);
    float heading = atan2f(-camFwd.x, camFwd.y);
    int orient = CGeneral::GetRandomNumberInRange(0, 2);
    heading += (orient + 1) * (3.14159265f / 2.0f);

    SpawnVehicle(modelId, pos, heading);
}

void TestSpawner::Process()
{
    if (FrontEndMenuManager.m_bMenuActive)
        return;

    if (!s_modelsPreloaded)
    {
        for (int i = 0; i < SPAWNABLE_COUNT; ++i)
            CStreaming::RequestModel(SPAWNABLE_MODELS[i], 0);
        s_modelsPreloaded = true;
    }

    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool key1 = ctrl && (GetAsyncKeyState('1') & 0x8000) != 0;
    bool key2 = ctrl && (GetAsyncKeyState('2') & 0x8000) != 0;
    bool key3 = ctrl && (GetAsyncKeyState('3') & 0x8000) != 0;
    bool key1NoCtrl = !ctrl && (GetAsyncKeyState('1') & 0x8000) != 0;
    bool keyJ = (GetAsyncKeyState('J') & 0x8000) != 0;

    if (key1NoCtrl && !s_key1NoCtrlWasDown)
    {
        s_key1NoCtrlWasDown = true;
        TeleportToWaypoint();
    }
    else if (!key1NoCtrl)
        s_key1NoCtrlWasDown = false;

    if (key1 && !s_key1WasDown)
    {
        s_key1WasDown = true;
        SpawnVehicleInFront(429);  // Banshee
    }
    else if (!key1)
        s_key1WasDown = false;

    if (key2 && !s_key2WasDown)
    {
        s_key2WasDown = true;
        SpawnVehicleInFront(425);  // Hunter
    }
    else if (!key2)
        s_key2WasDown = false;

    if (key3 && !s_key3WasDown)
    {
        s_key3WasDown = true;
        SpawnVehicleInFront(520);  // Hydra
    }
    else if (!key3)
        s_key3WasDown = false;

    if (keyJ && !s_keyJWasDown)
    {
        s_keyJWasDown = true;
        ToggleJetpack();
    }
    else if (!keyJ)
        s_keyJWasDown = false;
}

#endif // _DEBUG
