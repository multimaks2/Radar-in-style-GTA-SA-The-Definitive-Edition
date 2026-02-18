/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/render/RadarViewContext.h
 *****************************************************************************/

#pragma once

struct RadarViewContext
{
    float circleX, circleY;
    float sizeX, sizeY;
    float centerX, centerY;
    float halfX, halfY;

    float rtWidth, rtHeight;
    int   screenWidth, screenHeight;
    float screenAspect;

    bool shapeCircle;
    bool borderShapeCircle;

    float nearPlane;
    float farPlane;

    bool  isInAircraft;
    bool  isInPlane;
    float rollAngle;
    float pitchAngle;

    void* player;  // CPed* — avoid game include in header

    // Blip size helper: scale base size by screen width
    float CalculateBlipSize(float baseBlipSize = 24.0f, float baseWidth = 1920.0f) const;
};
