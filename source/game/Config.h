/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/game/Config.h
 *****************************************************************************/

#pragma once

#include <string>

namespace RadarConfig
{
    void Load();
    void Save();
    const char* GetConfigPath();

    bool GetShapeCircle();
    bool GetShowGangZones();
    bool GetModeMoreIcon();
    int  GetCircleSize();
    int  GetSquareSizeX();
    int  GetSquareSizeY();
    int  GetBorderThickness();
    int  GetOffsetX();
    int  GetOffsetY();
    void GetBackgroundColor(int& outR, int& outG, int& outB);
    void GetBackgroundColor(int& outR, int& outG, int& outB, int& outA);
    void GetCircleColor(int& outR, int& outG, int& outB, int& outA);
    void GetBorderColor(int& outR, int& outG, int& outB, int& outA);

    void SetShapeCircle(bool useCircle);
    void SetShowGangZones(bool value);
    void SetModeMoreIcon(bool value);
    void SetCircleSize(int value);
    void SetSquareSizeX(int value);
    void SetSquareSizeY(int value);
    void SetBorderThickness(int value);
    void SetOffsetX(int value);
    void SetOffsetY(int value);
}
