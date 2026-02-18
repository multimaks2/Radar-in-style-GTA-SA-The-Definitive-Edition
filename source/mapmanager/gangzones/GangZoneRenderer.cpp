/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/mapmanager/gangzones/GangZoneRenderer.cpp
 *****************************************************************************/

#include "GangZoneRenderer.h"
#include "CTheZones.h"
#include "CZone.h"
#include "CZoneInfo.h"
#include "CGangWars.h"
#include "CVector.h"
#include "CTimer.h"
#include "CRGBA.h"
#include "ColorUtils.h"
#include <cmath>

const float GangZoneRenderer::MAX_RENDER_DISTANCE = 4000.0f;
const float GangZoneRenderer::MAX_ZONE_SIZE = 600.0f;
const float GangZoneRenderer::ZONE_OVERLAP = 0.001f;  // overlap at edges to avoid seams

GangZoneRenderer::GangZoneRenderer(LPDIRECT3DDEVICE9 pDevice)
    : m_pDevice(pDevice)
    , m_lastUpdateTime(0)
    , m_enabled(false)
{
}

void GangZoneRenderer::UpdateCache()
{
    if (!m_enabled)
        return;

    // Check if gang wars are active in original game - match vanilla behavior
    // Gang zones are only visible when gang wars are enabled
    if (!CGangWars::bGangWarsActive)
    {
        m_cachedZones.clear();
        return;
    }

    m_cachedZones.clear();

    try
    {
        CTheZones::FillZonesWithGangColours(false);

        short numZones = CTheZones::TotalNumberOfInfoZones;
        if (numZones <= 0) numZones = CTheZones::TotalNumberOfMapZones;
        if (numZones <= 0) return;

        for (short i = 0; i < numZones; i++)
        {
            CZone* zone = CTheZones::GetInfoZone(i);
            if (!zone) zone = CTheZones::GetMapZone(i);
            if (!zone)
                continue;

            CVector center(
                ((float)zone->m_fX1 + (float)zone->m_fX2) * 0.5f,
                ((float)zone->m_fY1 + (float)zone->m_fY2) * 0.5f,
                0.0f
            );
            CZone* zoneAtCenter = nullptr;
            CZoneInfo* zoneInfo = CTheZones::GetZoneInfo(&center, &zoneAtCenter);
            if (!zoneInfo)
                continue;

            if (!CGangWars::CanPlayerStartAGangWarHere(zoneInfo) && !CGangWars::DoesPlayerControlThisZone(zoneInfo))
                continue;

            unsigned int c = zoneInfo->m_ZoneColor.ToInt();
            if ((c & 0x00FFFFFF) == 0)
                continue;

            float w = fabsf((float)(zone->m_fX2 - zone->m_fX1));
            float h = fabsf((float)(zone->m_fY2 - zone->m_fY1));
            if (w < 5.0f || h < 5.0f || w > MAX_ZONE_SIZE || h > MAX_ZONE_SIZE)
                continue;

            GangZone gangZone;
            gangZone.x1 = (float)zone->m_fX1;
            gangZone.y1 = (float)zone->m_fY1;
            gangZone.x2 = (float)zone->m_fX2;
            gangZone.y2 = (float)zone->m_fY2;

            unsigned char alpha = 140;
            if (CGangWars::DoesPlayerControlThisZone(zoneInfo))
                gangZone.color = tocolor(0, 200, 0, alpha);
            else
            {
                int ballas = (unsigned char)zoneInfo->m_nGangDensity[0];
                int vagos = (unsigned char)zoneInfo->m_nGangDensity[2];
                if (ballas >= vagos)
                    gangZone.color = tocolor(200, 0, 200, alpha);
                else
                    gangZone.color = tocolor(255, 220, 0, alpha);
            }

            m_cachedZones.push_back(gangZone);
        }
    }
    catch (...) {}
}

void GangZoneRenderer::Render(const D3DXVECTOR3& cameraPos, const D3DXVECTOR3& cameraRot,
                              float fov, float nearPlane, float farPlane,
                              float cameraPosX, float cameraPosY,
                              LPDIRECT3DTEXTURE9 lineTexture,
                              dxDrawImage3DCallback drawCallback)
{
    if (!m_enabled || !m_pDevice || !lineTexture)
        return;

    unsigned int currentTime = CTimer::m_snTimeInMilliseconds;
    if (currentTime - m_lastUpdateTime > UPDATE_INTERVAL || m_cachedZones.empty())
    {
        UpdateCache();
        m_lastUpdateTime = currentTime;
    }

    if (m_cachedZones.empty())
        return;

    try
    {
        for (size_t i = 0; i < m_cachedZones.size(); i++)
        {
            const GangZone& gangZone = m_cachedZones[i];

            float minX = (gangZone.x1 < gangZone.x2) ? gangZone.x1 : gangZone.x2;
            float maxX = (gangZone.x1 > gangZone.x2) ? gangZone.x1 : gangZone.x2;
            float minY = (gangZone.y1 < gangZone.y2) ? gangZone.y1 : gangZone.y2;
            float maxY = (gangZone.y1 > gangZone.y2) ? gangZone.y1 : gangZone.y2;

            float centerWorldX = (minX + maxX) * 0.5f;
            float centerWorldY = (minY + maxY) * 0.5f;
            float zoneMapX = centerWorldX + 3000.0f;
            float zoneMapY = centerWorldY - 3000.0f;
            float dx = zoneMapX - cameraPosX;
            float dy = zoneMapY - cameraPosY;
            float distance = sqrtf(dx * dx + dy * dy);
            if (distance > MAX_RENDER_DISTANCE)
                continue;

            float width  = maxX - minX;
            float height = maxY - minY;
            if (width < 1.0f || height < 1.0f)
                continue;

            // Add overlap to eliminate seams/flickering at zone boundaries
            width  += ZONE_OVERLAP;
            height += ZONE_OVERLAP;

            // Use constant Z for all zones to avoid z-fighting at junctions
            D3DXVECTOR3 zonePos(centerWorldX + 3000.0f, centerWorldY - 3000.0f, 1.0f);
            D3DXVECTOR3 zoneRot(0.0f, 0.0f, 0.0f);
            D3DXVECTOR2 zoneSize(width, height);

            if (drawCallback)
            {
                drawCallback(zonePos, zoneRot, zoneSize, cameraPos, cameraRot,
                             fov, nearPlane, farPlane, lineTexture, gangZone.color);
            }
        }
    }
    catch (...) {}
}
