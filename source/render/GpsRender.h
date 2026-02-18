/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/render/GpsRender.h
 *****************************************************************************/

#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <vector>

struct D3DXVECTOR2;
struct D3DXVECTOR3;

class DxDrawPrimitives;

class GpsRenderer
{
public:
    GpsRenderer(LPDIRECT3DDEVICE9 pDevice);
    ~GpsRenderer();

    bool Initialize();
    void Shutdown();

    // Render GPS route on radar (trilogy-style 3D lines). Call when radar RT is active.
    void Render(DxDrawPrimitives* pDraw,
        float centerX, float centerY, float sizeX, float sizeY,
        const D3DXVECTOR3& cameraPos, const D3DXVECTOR3& cameraRot,
        float fov, float nearPlane, float farPlane,
        float rtWidth, float rtHeight, float projectionAspect,
        bool shapeCircle, float halfX, float halfY);

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    bool m_bInitialized;
};
