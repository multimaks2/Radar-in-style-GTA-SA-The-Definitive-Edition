/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/render/RenderTarget.cpp
 *****************************************************************************/

#include "RenderTarget.h"
#include "Config.h"

RenderTarget::RenderTarget(LPDIRECT3DDEVICE9 pDevice)
    : m_pDevice(pDevice)
    , m_pRenderTargetTexture(nullptr)
    , m_pRenderTargetSurface(nullptr)
    , m_pOldRenderTarget(nullptr)
    , m_pOldDepthStencil(nullptr)
    , m_bViewportSaved(false)
    , m_width(0)
    , m_height(0)
{
}

RenderTarget::~RenderTarget()
{
    dxSetRenderTarget(nullptr);

    if (m_pRenderTargetSurface)
    {
        m_pRenderTargetSurface->Release();
        m_pRenderTargetSurface = nullptr;
    }
    if (m_pRenderTargetTexture)
    {
        m_pRenderTargetTexture->Release();
        m_pRenderTargetTexture = nullptr;
    }
}

bool RenderTarget::dxCreateRenderTarget(int width, int height)
{
    if (!m_pDevice)
        return false;

    if (m_pRenderTargetSurface)
    {
        m_pRenderTargetSurface->Release();
        m_pRenderTargetSurface = nullptr;
    }
    if (m_pRenderTargetTexture)
    {
        m_pRenderTargetTexture->Release();
        m_pRenderTargetTexture = nullptr;
    }

    HRESULT hr = m_pDevice->CreateTexture(
        width, height,
        1,
        D3DUSAGE_RENDERTARGET,
        D3DFMT_A8R8G8B8,
        D3DPOOL_DEFAULT,
        &m_pRenderTargetTexture,
        nullptr
    );

    if (FAILED(hr))
        return false;

    hr = m_pRenderTargetTexture->GetSurfaceLevel(0, &m_pRenderTargetSurface);
    if (FAILED(hr))
    {
        m_pRenderTargetTexture->Release();
        m_pRenderTargetTexture = nullptr;
        return false;
    }

    m_width  = width;
    m_height = height;

    return true;
}

void RenderTarget::dxSetRenderTarget(LPDIRECT3DSURFACE9 surface)
{
    if (!m_pDevice)
        return;

    HRESULT hr = m_pDevice->TestCooperativeLevel();
    if (FAILED(hr) && hr != D3DERR_DEVICENOTRESET)
        return;

    if (surface == nullptr)
    {
        if (m_pOldRenderTarget)
        {
            m_pDevice->SetRenderTarget(0, m_pOldRenderTarget);
            m_pOldRenderTarget->Release();
            m_pOldRenderTarget = nullptr;
        }
        if (m_pOldDepthStencil)
        {
            m_pDevice->SetDepthStencilSurface(m_pOldDepthStencil);
            m_pOldDepthStencil->Release();
            m_pOldDepthStencil = nullptr;
        }
        if (m_bViewportSaved)
        {
            m_pDevice->SetViewport(&m_oldViewport);
            m_bViewportSaved = false;
        }
    }
    else
    {
        m_pDevice->GetRenderTarget(0, &m_pOldRenderTarget);
        m_pDevice->GetDepthStencilSurface(&m_pOldDepthStencil);

        m_pDevice->GetViewport(&m_oldViewport);
        m_bViewportSaved = true;

        m_pDevice->SetRenderTarget(0, surface);

        D3DVIEWPORT9 vp;
        vp.X       = 0;
        vp.Y       = 0;
        vp.Width   = m_width;
        vp.Height  = m_height;
        vp.MinZ    = 0.0f;
        vp.MaxZ    = 1.0f;
        m_pDevice->SetViewport(&vp);

        // Get background color from config (RGBA)
        int bgR, bgG, bgB, bgA;
        RadarConfig::GetBackgroundColor(bgR, bgG, bgB, bgA);
        m_pDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(bgA, bgR, bgG, bgB), 1.0f, 0);
    }
}
