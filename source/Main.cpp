/*****************************************************************************
 *
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/Main.cpp
 *
 *****************************************************************************/

#include "plugin.h"
#include "common.h"
#include "RenderWare.h"
#include "Patch.h"
#include "RadarRenderer.h"
#include "Config.h"
#include "CMenuManager.h"
#ifdef _DEBUG
#include "TestSpawner.h"
#endif

using namespace plugin;

// Address of CHud::DrawRadar - we replace it with JMP to our DrawRadar
static constexpr uintptr_t RADAR_DRAW_ADDRESS = 0x58A330;

class RadarTrilogy
{
public:
    static RadarRenderer* s_pRadarRenderer;

    static void InitRadar()
    {
        auto* device = reinterpret_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
        if (!device)
            return;

        if (s_pRadarRenderer)
            return;

        RadarConfig::Load();

        s_pRadarRenderer = new RadarRenderer();
        if (!s_pRadarRenderer->Initialize(device))
        {
            delete s_pRadarRenderer;
            s_pRadarRenderer = nullptr;
            return;
        }

        bool shapeCircle = RadarConfig::GetShapeCircle();
        s_pRadarRenderer->SetRadarShapeCircle(shapeCircle);
        s_pRadarRenderer->SetBorderShapeCircle(shapeCircle);
        s_pRadarRenderer->SetShowGangZones(RadarConfig::GetShowGangZones());
    }

    static void DestroyRadar()
    {
        if (!s_pRadarRenderer)
            return;

        s_pRadarRenderer->Shutdown();
        delete s_pRadarRenderer;
        s_pRadarRenderer = nullptr;
    }

    static void DrawRadar()
    {
#ifndef _DEBUG
        patch::ReplaceFunction(RADAR_DRAW_ADDRESS, DrawRadar);
#endif

        if (!s_pRadarRenderer)
            return;

        auto* device = reinterpret_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
        if (!device)
            return;

        HRESULT hr = device->TestCooperativeLevel();
        if (FAILED(hr) && hr != D3DERR_DEVICENOTRESET)
            return;

        try
        {
            s_pRadarRenderer->Render();
        }
        catch (...)
        {
        }
    }

    static void OnCheckMenu()
    {
        // Arizona RP logic: restore our radar patch when menu is closed
        static bool s_prevMenuState = false;
        static int s_restoreTimer = 0;
        
        bool currMenuState = FrontEndMenuManager.m_bMenuActive;
        
        // If menu was open and now closed - schedule patch restore
        if (s_prevMenuState && !currMenuState)
        {
            s_restoreTimer = 2; // 2 frames delay
        }
        
        // Count down and restore patch
        if (s_restoreTimer > 0)
        {
            s_restoreTimer--;
            if (s_restoreTimer == 0)
            {
                patch::ReplaceFunction(RADAR_DRAW_ADDRESS, DrawRadar);
            }
        }
        
        s_prevMenuState = currMenuState;
    }

    static void restoreRender()
    {
        patch::ReplaceFunction(RADAR_DRAW_ADDRESS, DrawRadar);
    }

    RadarTrilogy()
    {
        s_pRadarRenderer = nullptr;

        #ifdef _DEBUG
            Events::drawRadarEvent += DrawRadar;
            Events::processScriptsEvent += []
            {
                TestSpawner::Process();
            };
        #else
            Events::initGameEvent += []
            {
                patch::ReplaceFunction(RADAR_DRAW_ADDRESS, DrawRadar);
            };
            // Track menu state to restore radar after ESC
            Events::processScriptsEvent += OnCheckMenu;
        #endif


        Events::initRwEvent += InitRadar;
        Events::shutdownRwEvent += DestroyRadar;
        Events::d3dLostEvent += DestroyRadar;
        Events::d3dResetEvent += InitRadar;
    }

} g_radarTrilogy;

RadarRenderer* RadarTrilogy::s_pRadarRenderer = nullptr;
