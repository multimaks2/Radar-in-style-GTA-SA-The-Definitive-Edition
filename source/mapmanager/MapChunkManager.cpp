/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/mapmanager/MapChunkManager.cpp
 *****************************************************************************/

#include "MapChunkManager.h"
#include "plugin.h"
#include "CFileLoader.h"
#include "RenderWare.h"
#include <d3dx9.h>
#include <cstring>
#include <cmath>

const float MapChunkManager::MAP_WIDTH     = 6000.0f;
const float MapChunkManager::MAP_HEIGHT    = 6000.0f;
const float MapChunkManager::MAP_CENTER_X  = 3000.0f;
const float MapChunkManager::MAP_CENTER_Y  = -3000.0f;

static LPDIRECT3DTEXTURE9 RwTextureToD3D9(LPDIRECT3DDEVICE9 pDevice, RwTexture* rwTex)
{
    if (!pDevice || !rwTex)
        return nullptr;

    RwRaster* raster = RwTextureGetRaster(rwTex);
    if (!raster)
        return nullptr;

    int w = RwRasterGetWidth(raster);
    int h = RwRasterGetHeight(raster);
    if (w <= 0 || h <= 0)
        return nullptr;

    RwImage* img = RwImageCreate(w, h, 32);
    if (!img)
        return nullptr;

    if (!RwImageAllocatePixels(img))
    {
        RwImageDestroy(img);
        return nullptr;
    }

    if (!RwImageSetFromRaster(img, raster))
    {
        RwImageFreePixels(img);
        RwImageDestroy(img);
        return nullptr;
    }

    LPDIRECT3DTEXTURE9 d3dTex = nullptr;
    HRESULT hr = pDevice->CreateTexture((UINT)w, (UINT)h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &d3dTex, nullptr);
    if (FAILED(hr) || !d3dTex)
    {
        RwImageFreePixels(img);
        RwImageDestroy(img);
        return nullptr;
    }

    D3DLOCKED_RECT locked;
    if (SUCCEEDED(d3dTex->LockRect(0, &locked, nullptr, 0)))
    {
        RwUInt8* src = RwImageGetPixels(img);
        int srcStride = RwImageGetStride(img);
        int dstStride = locked.Pitch;
        for (int y = 0; y < h; y++)
        {
            RwUInt8* rowSrc = src + y * srcStride;
            RwUInt8* rowDst = (RwUInt8*)locked.pBits + y * dstStride;
            for (int x = 0; x < w; x++)
            {
                int off = x * 4;
                rowDst[off + 0] = rowSrc[off + 2];
                rowDst[off + 1] = rowSrc[off + 1];
                rowDst[off + 2] = rowSrc[off + 0];
                rowDst[off + 3] = rowSrc[off + 3];
            }
        }
        d3dTex->UnlockRect(0);
    }

    RwImageFreePixels(img);
    RwImageDestroy(img);
    return d3dTex;
}

MapChunkManager::MapChunkManager(LPDIRECT3DDEVICE9 pDevice)
    : m_pDevice(pDevice)
    , m_pMapTxd(nullptr)
    , m_initialized(false)
{
    ZeroMemory(m_chunks, sizeof(m_chunks));
    ZeroMemory(m_loaded, sizeof(m_loaded));
}

MapChunkManager::~MapChunkManager()
{
    Cleanup();
}

bool MapChunkManager::Initialize()
{
    if (m_initialized)
        return true;

    const char* path = PLUGIN_PATH("radar/map.txd");
    m_pMapTxd = CFileLoader::LoadTexDictionary(path);
    if (!m_pMapTxd)
        return false;

    m_initialized = true;
    LoadAllChunks();
    return true;
}

void MapChunkManager::LoadAllChunks()
{
    if (!m_initialized && !Initialize())
        return;

    for (int index = 0; index < MAP_CHUNKS_COUNT; ++index)
    {
        if (m_chunks[index])
            continue;

        char texName[32];
        sprintf_s(texName, "radar%02d", index);

        RwTexture* rwTex = RwTexDictionaryFindNamedTexture(m_pMapTxd, texName);
        if (!rwTex)
            continue;

        m_chunks[index] = RwTextureToD3D9(m_pDevice, rwTex);
        m_loaded[index] = (m_chunks[index] != nullptr);
    }
}

float MapChunkManager::ComputeVisibleRadius(float cameraZ, float fov, float offsetY, bool isInAircraft)
{
    float visibleRadius = cameraZ * tanf(fov * 0.5f) * 2.0f;
    visibleRadius += fabsf(offsetY) * 1.5f;
    if (isInAircraft)
        visibleRadius *= 1.7f;
    visibleRadius *= 1.4f;  // extra margin to avoid pop-in at edges
    return visibleRadius;
}

void MapChunkManager::Cleanup()
{
    for (int i = 0; i < MAP_CHUNKS_COUNT; ++i)
    {
        if (m_chunks[i])
        {
            m_chunks[i]->Release();
            m_chunks[i] = nullptr;
        }
        m_loaded[i] = false;
    }

    if (m_pMapTxd)
    {
        RwTexDictionaryDestroy(m_pMapTxd);
        m_pMapTxd = nullptr;
    }
    m_initialized = false;
}

LPDIRECT3DTEXTURE9 MapChunkManager::GetChunk(int index) const
{
    if (index < 0 || index >= MAP_CHUNKS_COUNT)
        return nullptr;
    return m_chunks[index];
}

bool MapChunkManager::IsChunkLoaded(int index) const
{
    if (index < 0 || index >= MAP_CHUNKS_COUNT)
        return false;
    return m_loaded[index];
}

int MapChunkManager::GetLoadedChunksCount() const
{
    int count = 0;
    for (int i = 0; i < MAP_CHUNKS_COUNT; ++i)
        if (m_loaded[i])
            ++count;
    return count;
}
