/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/mapmanager/BlipRenderer.h
 *****************************************************************************/

#pragma once

#include <d3d9.h>
#include <d3dx9.h>

class BlipManager;
class CPed;
class CameraController;
class DxDrawPrimitives;

class BlipRenderer
{
public:
    BlipRenderer(LPDIRECT3DDEVICE9 pDevice);
    ~BlipRenderer();

    void RenderBlips(BlipManager* pBlipManager, CameraController* pCamera, DxDrawPrimitives* pDraw,
                    bool inAircraft, CPed* player, float nearPlane, float farPlane);
    void RenderBlips2D(BlipManager* pBlipManager, CameraController* pCamera, DxDrawPrimitives* pDraw,
                      float circleX, float circleY, float sizeX, float sizeY,
                      bool shapeCircle, float rtWidth, float rtHeight,
                      int screenWidth, int screenHeight);

private:
    LPDIRECT3DDEVICE9 m_pDevice;
};
