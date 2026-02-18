/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/render/RadarTypes.h
 *****************************************************************************/

#pragma once

#include <d3d9.h>
#include <d3dx9.h>

struct ScreenVertex
{
    float x, y, z;
    DWORD color;
    float u, v;
};

struct RenderStates
{
    DWORD                       fvf;
    IDirect3DBaseTexture9*      texture;
    IDirect3DVertexDeclaration9* vertexDecl;
    DWORD                       samplerStateU;
    DWORD                       samplerStateV;
    DWORD                       samplerMinFilter;
    DWORD                       samplerMagFilter;
    IDirect3DVertexShader9*     vertexShader;
    IDirect3DPixelShader9*      pixelShader;
    DWORD                       alphaBlendEnable;
    DWORD                       srcBlend;
    DWORD                       destBlend;
    DWORD                       zEnable;
    DWORD                       zWriteEnable;
    DWORD                       lighting;
    DWORD                       cullMode;
    D3DXMATRIX                  worldMatrix;
    D3DXMATRIX                  viewMatrix;
    D3DXMATRIX                  projMatrix;
};
