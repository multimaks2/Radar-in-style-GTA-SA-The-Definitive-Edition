/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/mapmanager/airstrips/AirstripRenderer.cpp
 *****************************************************************************/

#include "AirstripRenderer.h"

AirstripRenderer::AirstripRenderer(LPDIRECT3DDEVICE9 pDevice)
    : m_pDevice(pDevice)
{
}

AirstripRenderer::~AirstripRenderer()
{
    Shutdown();
}

bool AirstripRenderer::Initialize()
{
    if (!m_pDevice)
        return false;
    return true;
}

void AirstripRenderer::Shutdown()
{
}

void AirstripRenderer::Render()
{
}
