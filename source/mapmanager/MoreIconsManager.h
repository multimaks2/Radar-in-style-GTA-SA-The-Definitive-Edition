/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/mapmanager/MoreIconsManager.h
 *****************************************************************************/

#pragma once

#include "BlipTypes.h"
#include <vector>

class MoreIconsManager
{
public:
    MoreIconsManager();
    ~MoreIconsManager();

    void GetBlips(std::vector<Blip>& outBlips) const;

private:
    bool FindEnexPosition(const char* enexName, float& outX, float& outY) const;
    int AddAllEnexMatching(const char* const* names, int nameCount, int iconId, std::vector<Blip>& outBlips) const;
    void AddRespawnBlips(int iconId, bool isHospital, std::vector<Blip>& outBlips) const;
    void AddBlip(std::vector<Blip>& outBlips, float x, float y, int iconId) const;
};
