/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/mapmanager/BlipRenderer.cpp
 *****************************************************************************/

#include "BlipRenderer.h"
#include "BlipManager.h"
#include "CameraController.h"
#include "DxDrawPrimitives.h"

BlipRenderer::BlipRenderer(LPDIRECT3DDEVICE9 pDevice)
    : m_pDevice(pDevice)
{
}

BlipRenderer::~BlipRenderer()
{
}

void BlipRenderer::RenderBlips(BlipManager*, CameraController*, DxDrawPrimitives*,
                               bool, CPed*, float, float)
{
}

void BlipRenderer::RenderBlips2D(BlipManager*, CameraController*, DxDrawPrimitives*,
                                 float, float, float, float,
                                 bool, float, float,
                                 int, int)
{
}
