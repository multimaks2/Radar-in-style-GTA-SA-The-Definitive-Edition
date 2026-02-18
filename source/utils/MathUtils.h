/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/utils/MathUtils.h
 *****************************************************************************/

#pragma once

#include <d3d9.h>
#include <d3dx9.h>

class MathUtils
{
public:
    // projectionAspect: if > 0 use for projection (e.g. screen height/width); if <= 0 use screenHeight/screenWidth
    static bool  WorldToScreen(const D3DXVECTOR3& worldPos, const D3DXVECTOR3& cameraPos, const D3DXVECTOR3& cameraRot, float fov, float nearPlane,
                               float farPlane, float screenWidth, float screenHeight, float& screenX, float& screenY, float projectionAspect = 0.0f);
    static void  CalculateRadarPosition(float& circleX, float& circleY, float& circleSize);
    // With configurable base sizes and shape; outputs sizeX, sizeY (for circle sizeX==sizeY)
    // baseOffsetX = offset from left edge (Full HD: 85), baseOffsetY = offset from bottom edge (Full HD: 55)
    static void  CalculateRadarPosition(float& circleX, float& circleY, float& sizeX, float& sizeY,
                                       float baseSizeCircle, float baseSizeSquareX, float baseSizeSquareY, bool shapeCircle,
                                       float baseOffsetX, float baseOffsetY);
    static float CalculateDistance2D(const D3DXVECTOR3& a, const D3DXVECTOR3& b);
    static float DistanceSq2D(const D3DXVECTOR3& a, const D3DXVECTOR3& b);
    static float DistanceSq2D(float ax, float ay, float bx, float by);
    // Direction from A to B, normalised; returns orbit angle for PointOnOrbitEdge (or false if too close)
    static bool  DirectionToOrbitAngle(const D3DXVECTOR3& fromPos, const D3DXVECTOR3& toPos, float yaw, float& outAngle);
};
