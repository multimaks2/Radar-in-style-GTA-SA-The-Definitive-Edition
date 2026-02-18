/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/render/RenderTarget.h
 *****************************************************************************/

#pragma once

#include <d3d9.h>

class RenderTarget
{
public:
    RenderTarget(LPDIRECT3DDEVICE9 pDevice);
    ~RenderTarget();

    bool dxCreateRenderTarget(int width, int height);
    void dxSetRenderTarget(LPDIRECT3DSURFACE9 surface = nullptr);

    LPDIRECT3DTEXTURE9 GetRenderTargetTexture() const { return m_pRenderTargetTexture; }
    LPDIRECT3DSURFACE9 GetSurface() const { return m_pRenderTargetSurface; }
    int                GetWidth() const { return m_width; }
    int                GetHeight() const { return m_height; }

private:
    LPDIRECT3DDEVICE9    m_pDevice;
    LPDIRECT3DTEXTURE9  m_pRenderTargetTexture;
    LPDIRECT3DSURFACE9  m_pRenderTargetSurface;
    LPDIRECT3DSURFACE9  m_pOldRenderTarget;
    LPDIRECT3DSURFACE9  m_pOldDepthStencil;
    D3DVIEWPORT9        m_oldViewport;
    bool                m_bViewportSaved;
    int                 m_width;
    int                 m_height;
};
