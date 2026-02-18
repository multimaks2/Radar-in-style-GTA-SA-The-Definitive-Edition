/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/render/GpsRender.cpp
 *****************************************************************************/

#include "GpsRender.h"
#include "RadarGeometry.h"
#include "DxDrawPrimitives.h"
#include "ColorUtils.h"
#include "plugin.h"
#include "common.h"
#include "RenderWare.h"
#include "CPlayerPed.h"
#include "CVehicle.h"
#include "CMenuManager.h"
#include "CRadar.h"
#include "CPathFind.h"
#include "CNodeAddress.h"
#include "CPathNode.h"

#define GPS_LINE_WIDTH       6.0f
#define GPS_LINE_COLOR_A     255
#define GPS_LINE_R           255
#define GPS_LINE_G           220
#define GPS_LINE_B           0
#define RADAR_ROUTE_Z        0.02f
#define MAX_NODE_POINTS      2000

GpsRenderer::GpsRenderer(LPDIRECT3DDEVICE9 pDevice)
    : m_pDevice(pDevice)
    , m_bInitialized(false)
{
}

GpsRenderer::~GpsRenderer()
{
    Shutdown();
}

bool GpsRenderer::Initialize()
{
    if (!m_pDevice)
        return false;
    m_bInitialized = true;
    return true;
}

void GpsRenderer::Shutdown()
{
    m_bInitialized = false;
}

void GpsRenderer::Render(DxDrawPrimitives* pDraw,
    float centerX, float centerY, float sizeX, float sizeY,
    const D3DXVECTOR3& cameraPos, const D3DXVECTOR3& cameraRot,
    float fov, float nearPlane, float farPlane,
    float rtWidth, float rtHeight, float projectionAspect,
    bool shapeCircle, float halfX, float halfY)
{
    (void)centerX; (void)centerY; (void)sizeX; (void)sizeY;
    (void)shapeCircle; (void)halfX; (void)halfY;

    if (!m_bInitialized || !pDraw)
        return;

    CPed* playa = FindPlayerPed(0);
    if (!playa || !playa->m_pVehicle || !playa->bInVehicle)
        return;
    if (playa->m_pVehicle->m_nVehicleSubClass == VEHICLE_PLANE ||
        playa->m_pVehicle->m_nVehicleSubClass == VEHICLE_HELI ||
        playa->m_pVehicle->m_nVehicleSubClass == VEHICLE_BMX)
        return;
    if (!FrontEndMenuManager.m_nTargetBlipIndex)
        return;

    int blipArrId = LOWORD(FrontEndMenuManager.m_nTargetBlipIndex);
    int blipCounter = HIWORD(FrontEndMenuManager.m_nTargetBlipIndex);
    if (blipArrId < 0 || blipArrId >= (int)MAX_RADAR_TRACES)
        return;
    if (CRadar::ms_RadarTrace[blipArrId].m_nCounter != (unsigned short)blipCounter)
        return;
    if (!CRadar::ms_RadarTrace[blipArrId].m_nBlipDisplay)
        return;

    CVector playerPos = FindPlayerCoors(0);
    CVector destPosn = CRadar::ms_RadarTrace[blipArrId].m_vecPos;
    destPosn.z = playerPos.z;

    short nodesCount = 0;
    float gpsDistance = 0.0f;
    static CNodeAddress resultNodes[MAX_NODE_POINTS];

    ThePaths.DoPathSearch(0, FindPlayerCoors(0), CNodeAddress(), destPosn,
        resultNodes, &nodesCount, MAX_NODE_POINTS, &gpsDistance,
        999999.0f, NULL, 999999.0f, false, CNodeAddress(), false,
        playa->m_pVehicle->m_nVehicleSubClass == VEHICLE_BOAT);

    if (nodesCount <= 0)
        return;

    D3DXVECTOR3 playerPosRadar(playerPos.x + RadarGeometry::RADAR_OFFSET_X,
        playerPos.y + RadarGeometry::RADAR_OFFSET_Y, RADAR_ROUTE_Z);

    DWORD lineColor = tocolor(GPS_LINE_R, GPS_LINE_G, GPS_LINE_B, GPS_LINE_COLOR_A);
    std::vector<RoutePoint3D> route;

    int startSegment = 0;
    D3DXVECTOR3 adjustedStartPos = playerPosRadar;

    for (short i = 0; i < nodesCount - 1; i++)
    {
        CVector nodePos1 = ThePaths.GetPathNode(resultNodes[i])->GetNodeCoors();
        CVector nodePos2 = ThePaths.GetPathNode(resultNodes[i + 1])->GetNodeCoors();
        D3DXVECTOR3 segStart(nodePos1.x + RadarGeometry::RADAR_OFFSET_X,
            nodePos1.y + RadarGeometry::RADAR_OFFSET_Y, RADAR_ROUTE_Z);
        D3DXVECTOR3 segEnd(nodePos2.x + RadarGeometry::RADAR_OFFSET_X,
            nodePos2.y + RadarGeometry::RADAR_OFFSET_Y, RADAR_ROUTE_Z);

        D3DXVECTOR3 segDir = segEnd - segStart;
        float segLength = D3DXVec3Length(&segDir);
        if (segLength < 0.01f) continue;
        D3DXVec3Normalize(&segDir, &segDir);

        D3DXVECTOR3 toPlayer = playerPosRadar - segStart;
        float proj = D3DXVec3Dot(&toPlayer, &segDir);

        if (proj >= segLength) { startSegment = i + 1; continue; }
        if (proj > 0.0f && proj < segLength)
        {
            adjustedStartPos = segStart + segDir * proj;
            startSegment = i;
            break;
        }
        D3DXVECTOR3 toSeg = segStart - playerPosRadar;
        if (proj <= 0.0f && D3DXVec3Length(&toSeg) < 50.0f)
        {
            adjustedStartPos = playerPosRadar;
            startSegment = i;
            break;
        }
    }

    route.push_back({ adjustedStartPos.x, adjustedStartPos.y, adjustedStartPos.z, lineColor });
    for (short i = startSegment + 1; i < nodesCount; i++)
    {
        CVector nodePos = ThePaths.GetPathNode(resultNodes[i])->GetNodeCoors();
        route.push_back({
            nodePos.x + RadarGeometry::RADAR_OFFSET_X,
            nodePos.y + RadarGeometry::RADAR_OFFSET_Y,
            RADAR_ROUTE_Z,
            lineColor
        });
    }
    route.push_back({
        destPosn.x + RadarGeometry::RADAR_OFFSET_X,
        destPosn.y + RadarGeometry::RADAR_OFFSET_Y,
        RADAR_ROUTE_Z,
        lineColor
    });

    if (route.size() < 2)
        return;

    float lineWidth = GPS_LINE_WIDTH * RadarGeometry::GetRadarScale();

    if (reinterpret_cast<D3DCAPS9 const*>(RwD3D9GetCaps())->RasterCaps & D3DPRASTERCAPS_SCISSORTEST)
    {
        RECT rect;
        rect.left = static_cast<LONG>(centerX - sizeX * 0.5f + 2.0f);
        rect.top = static_cast<LONG>(centerY - sizeY * 0.5f + 2.0f);
        rect.right = static_cast<LONG>(centerX + sizeX * 0.5f - 2.0f);
        rect.bottom = static_cast<LONG>(centerY + sizeY * 0.5f - 2.0f);
        m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
        m_pDevice->SetScissorRect(&rect);
    }

    pDraw->dxDrawRoute3D(route, cameraPos, cameraRot, fov, nearPlane, farPlane,
        projectionAspect, lineWidth);

    if (reinterpret_cast<D3DCAPS9 const*>(RwD3D9GetCaps())->RasterCaps & D3DPRASTERCAPS_SCISSORTEST)
    {
        m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    }
}
