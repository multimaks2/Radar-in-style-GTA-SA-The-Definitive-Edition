/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/mapmanager/MapChunkManager.h
 *****************************************************************************/

#pragma once

#include <cmath>
#include <d3d9.h>
#include <d3dx9.h>
#include "RenderWare.h"
#include "MathUtils.h"

class MapChunkManager
{
public:
    static const int   MAP_CHUNKS_COUNT = 144;
    static const int   MAP_CHUNKS_PER_ROW = 12;
    static const float MAP_WIDTH;
    static const float MAP_HEIGHT;
    static const float MAP_CENTER_X;
    static const float MAP_CENTER_Y;

    struct FrustumParams
    {
        const D3DXVECTOR3* cameraPos;
        const D3DXVECTOR3* cameraRot;
        float fov;
        float nearPlane;
        float farPlane;
        float screenWidth;
        float screenHeight;
        float projectionAspect;
    };

    MapChunkManager(LPDIRECT3DDEVICE9 pDevice);
    ~MapChunkManager();

    bool Initialize();
    void LoadAllChunks();
    void Cleanup();

    // Distance culling (radius) + Frustum culling. Pass frustumParams=nullptr to skip frustum culling.
    template<typename F>
    void ForEachChunkInRadius(const D3DXVECTOR3& cameraPos, float visibleRadius,
                             const FrustumParams* frustumParams, F&& callback) const;

    LPDIRECT3DTEXTURE9 GetChunk(int index) const;
    bool               IsChunkLoaded(int index) const;
    int                GetLoadedChunksCount() const;

    static float ComputeVisibleRadius(float cameraZ, float fov, float offsetY, bool isInAircraft);

private:
    LPDIRECT3DDEVICE9   m_pDevice;
    RwTexDictionary*    m_pMapTxd;
    LPDIRECT3DTEXTURE9  m_chunks[MAP_CHUNKS_COUNT];
    bool                m_loaded[MAP_CHUNKS_COUNT];
    bool                m_initialized;
};

template<typename F>
void MapChunkManager::ForEachChunkInRadius(const D3DXVECTOR3& cameraPos, float visibleRadius,
                                          const FrustumParams* frustumParams, F&& callback) const
{
    const float chunkWorldWidth  = MAP_WIDTH / MAP_CHUNKS_PER_ROW;
    const float chunkWorldHeight = MAP_HEIGHT / MAP_CHUNKS_PER_ROW;
    const float halfW = chunkWorldWidth * 0.5f;
    const float halfH = chunkWorldHeight * 0.5f;
    const float mapLeft = MAP_CENTER_X - MAP_WIDTH * 0.5f;
    const float mapTop  = MAP_CENTER_Y + MAP_HEIGHT * 0.5f;

    // Chunk visible if ANY part is in view: expand radius so chunk rect can intersect
    const float chunkHalfDiag = sqrtf(halfW * halfW + halfH * halfH);
    const float effectiveRadius = visibleRadius + chunkHalfDiag;
    const float radiusSq = effectiveRadius * effectiveRadius;

    for (int index = 0; index < MAP_CHUNKS_COUNT; ++index)
    {
        if (!m_loaded[index])
            continue;

        LPDIRECT3DTEXTURE9 chunkTex = m_chunks[index];
        if (!chunkTex)
            continue;

        int row = index / MAP_CHUNKS_PER_ROW;
        int col = index % MAP_CHUNKS_PER_ROW;
        float chunkCenterX = mapLeft + (col + 0.5f) * chunkWorldWidth;
        float chunkCenterY = mapTop - (row + 0.5f) * chunkWorldHeight;

        // Distance: chunk rect intersects visibility circle (center within effectiveRadius)
        if (MathUtils::DistanceSq2D(chunkCenterX, chunkCenterY, cameraPos.x, cameraPos.y) > radiusSq)
            continue;

        // Frustum: at least one of 4 corners must project into view (even a tiny piece visible)
        if (frustumParams && frustumParams->cameraPos && frustumParams->cameraRot)
        {
            const D3DXVECTOR3& cp = *frustumParams->cameraPos;
            const D3DXVECTOR3& cr = *frustumParams->cameraRot;
            float fov = frustumParams->fov;
            float np = frustumParams->nearPlane;
            float fp = frustumParams->farPlane;
            float sw = frustumParams->screenWidth;
            float sh = frustumParams->screenHeight;
            float pa = frustumParams->projectionAspect;

            float cx = chunkCenterX, cy = chunkCenterY;
            D3DXVECTOR3 corners[4] = {
                { cx - halfW, cy - halfH, 0.1f },
                { cx + halfW, cy - halfH, 0.1f },
                { cx + halfW, cy + halfH, 0.1f },
                { cx - halfW, cy + halfH, 0.1f }
            };

            bool anyInFrustum = false;
            for (int i = 0; i < 4; ++i)
            {
                float sx, sy;
                if (MathUtils::WorldToScreen(corners[i], cp, cr, fov, np, fp, sw, sh, sx, sy, pa))
                {
                    anyInFrustum = true;
                    break;
                }
            }
            if (!anyInFrustum)
                continue;
        }

        D3DXVECTOR3 elementPos(chunkCenterX, chunkCenterY, 0.0f);
        D3DXVECTOR3 elementRot(0.0f, 0.0f, 0.0f);
        D3DXVECTOR2 elementSize(chunkWorldWidth, chunkWorldHeight);

        callback(index, elementPos, elementRot, elementSize, chunkTex);
    }
}
