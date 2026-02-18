/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/mapmanager/legends/LegendRenderer.h
 *****************************************************************************/

#pragma once

#include <d3d9.h>

class LegendRenderer
{
public:
    LegendRenderer(LPDIRECT3DDEVICE9 pDevice);
    ~LegendRenderer();

    bool Initialize();
    void Shutdown();
    void Render();

private:
    LPDIRECT3DDEVICE9 m_pDevice;
};
