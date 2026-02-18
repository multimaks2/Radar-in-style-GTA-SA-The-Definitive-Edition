/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/mapmanager/legends/LegendRenderer.cpp
 *****************************************************************************/

#include "LegendRenderer.h"

LegendRenderer::LegendRenderer(LPDIRECT3DDEVICE9 pDevice)
    : m_pDevice(pDevice)
{
}

LegendRenderer::~LegendRenderer()
{
    Shutdown();
}

bool LegendRenderer::Initialize()
{
    if (!m_pDevice)
        return false;
    return true;
}

void LegendRenderer::Shutdown()
{
}

void LegendRenderer::Render()
{
}
