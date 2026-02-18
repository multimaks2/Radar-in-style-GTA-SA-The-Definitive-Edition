/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/mapmanager/airstrips/AirstripRenderer.h
 *****************************************************************************/

#pragma once

#include <d3d9.h>

class AirstripRenderer
{
public:
    AirstripRenderer(LPDIRECT3DDEVICE9 pDevice);
    ~AirstripRenderer();

    bool Initialize();
    void Shutdown();
    void Render();

private:
    LPDIRECT3DDEVICE9 m_pDevice;
};
