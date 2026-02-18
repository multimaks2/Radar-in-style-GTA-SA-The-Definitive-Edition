/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/render/RadarViewContext.cpp
 *****************************************************************************/

#include "RadarViewContext.h"

float RadarViewContext::CalculateBlipSize(float baseBlipSize, float baseWidth) const
{
    float screenW = (float)screenWidth;
    float scaleX  = screenW / baseWidth;
    return baseBlipSize * scaleX;
}
