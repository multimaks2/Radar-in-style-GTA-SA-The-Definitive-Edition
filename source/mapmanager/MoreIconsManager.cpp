/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/mapmanager/MoreIconsManager.cpp
 *****************************************************************************/

#include "MoreIconsManager.h"
#include "BlipManager.h"
#include "CRadar.h"
#include "CRect.h"
#include "CEntryExitManager.h"
#include "CEntryExit.h"
#include "CPool.h"
#include "CRestart.h"
#include "CVector.h"
#include <cstring>

namespace
{
    // More icon PNGs: store, donuts, intrack, casino, dateNude, train
    constexpr int ICON_STORE = BlipManager::MORE_ICON_STORE;
    constexpr int ICON_DONUTS = BlipManager::MORE_ICON_DONUTS;
    constexpr int ICON_INTRACK = BlipManager::MORE_ICON_INTRACK;
    constexpr int ICON_CASINO = BlipManager::MORE_ICON_CASINO;
    constexpr int ICON_STRIP = BlipManager::MORE_ICON_DATENUDE;
    constexpr int ICON_TRAIN = BlipManager::MORE_ICON_TRAIN;
    // Standard RADAR_SPRITE for others (ICON_REST = 50 removed - conflicts with game sprite)
    constexpr int ICON_CLUB = 48;
    constexpr int ICON_BAR = 49;
    constexpr int ICON_BARBER = 7;
    constexpr int ICON_PIZZA = 29;
    constexpr int ICON_AIRPORT = 5;
    constexpr int ICON_SHIP = 9;
    constexpr int ICON_FIRE = 20;
    constexpr int ICON_RACE = 33;
    constexpr int ICON_GYM = 54;
    constexpr int ICON_POLICE = 30;
    constexpr int ICON_HOSP = 22;

    struct CoordEntry
    {
        float x, y;
        int iconId;
    };

    static const char* DONUTS_NAMES[] = {"FDDONUT", "FDDONUT2", nullptr};

    static const char* INTRACK_NAMES[] = {"GENOTB", "GENOTB2", nullptr};
    static const CoordEntry INTRACK_COORDS[] = {
        {1260.0f, -803.0f, ICON_INTRACK},
        {1477.0f, 2284.0f, ICON_INTRACK},
        {0, 0, 0}
    };

    static const char* CLUB_NAMES[] = {"BAR1", nullptr};
    static const CoordEntry CLUB_COORDS[] = {
        {1836.0f, -1682.0f, ICON_CLUB},
        {0, 0, 0}
    };

    static const char* BAR_NAMES[] = {"BAR2", "UFOBAR", "TSDINER", nullptr};
    static const CoordEntry BAR_COORDS[] = {
        {495.0f, -75.0f, ICON_BAR},
        {-89.0f, 1373.0f, ICON_BAR},
        {452.0f, -20.0f, ICON_BAR},
        {-77.0f, 1220.0f, ICON_BAR},
        {0, 0, 0}
    };

    static const char* STRIP_NAMES[] = {"LASTRIP", "STRIP1", "STRIP2", "PDOMES2", nullptr};
    static const CoordEntry STRIP_COORDS[] = {
        {2351.0f, -1182.0f, ICON_STRIP},
        {2561.0f, 1026.0f, ICON_STRIP},
        {-89.0f, 1372.0f, ICON_STRIP},
        {2560.0f, 1100.0f, ICON_STRIP},
        {0, 0, 0}
    };

    // static const char* REST_NAMES[] = {"DINER1", "DINER2", "FDREST1", "REST2", nullptr};  // Removed - ICON_REST = 50 conflicts with game sprite
    static const char* CASINO_NAMES[] = {"MAFCAS", "CASINO2", "TRICAS", nullptr};

    static const CoordEntry BARBER[] = {
        {-1450.6199f, 2592.1501f, ICON_BARBER},
        {0, 0, 0}
    };

    static const CoordEntry PIZZA[] = {
        {1366.7700f, 250.3880f, ICON_PIZZA},
        {0, 0, 0}
    };

    static const CoordEntry AIRPORT[] = {
        {1685.649f, -2238.861f, ICON_AIRPORT},
        {-1422.066f, -288.3433f, ICON_AIRPORT},
        {1663.977f, 1424.857f, ICON_AIRPORT},
        {0, 0, 0}
    };

    static const CoordEntry SHIP[] = {
        {-1574.002808f, 133.471802f, ICON_SHIP},
        {0, 0, 0}
    };

    static const CoordEntry FIRE[] = {
        {1753.736f, -1457.757f, ICON_FIRE},
        {-2025.462f, 82.1209f, ICON_FIRE},
        {0, 0, 0}
    };

    static const CoordEntry RACE[] = {
        {2139.691f, -54.2869f, ICON_RACE},
        {-2297.395f, -1679.625f, ICON_RACE},
        {0, 0, 0}
    };

    static const CoordEntry GYM[] = {
        {665.33f, -1865.615f, ICON_GYM},
        {0, 0, 0}
    };

    static const CoordEntry TRAIN[] = {
        {1434.145f, 2624.496f, ICON_TRAIN},
        {-1983.391f, 138.4992f, ICON_TRAIN},
        {1759.01f, -1944.441f, ICON_TRAIN},
        {822.3628f, -1364.624f, ICON_TRAIN},
        {2859.9597f, 1290.1663f, ICON_TRAIN},
        {0, 0, 0}
    };
}  // namespace

MoreIconsManager::MoreIconsManager()
{
}

MoreIconsManager::~MoreIconsManager()
{
}

void MoreIconsManager::AddBlip(std::vector<Blip>& outBlips, float x, float y, int iconId) const
{
    Blip blip;
    blip.position = D3DXVECTOR3(x + 3000.0f, y - 3000.0f, 0.1f);
    blip.iconId = iconId;
    blip.size = 16.0f;
    blip.color = D3DCOLOR_ARGB(255, 255, 255, 255);
    blip.enabled = true;
    blip.shortRange = false;
    outBlips.push_back(blip);
}

// True = exterior entrance (leads INTO interior). Render icon.
// False = interior exit (leads OUT to street). Do not render.
// m_nArea = TargetInterior: 0 = exterior world, non-zero = interior ID.
static bool IsExteriorEnex(const CEntryExit* enex)
{
    if (!enex)
        return false;
    return enex->m_nArea == 0;
}

bool MoreIconsManager::FindEnexPosition(const char* enexName, float& outX, float& outY) const
{
    int idx = CEntryExitManager::GetEntryExitIndex(enexName, 0, 0);
    if (idx < 0)
        return false;

    CEntryExit* enex = CEntryExitManager::GetEntryExit(idx);
    if (!enex || !IsExteriorEnex(enex))
        return false;

    float cx, cy;
    enex->m_recEntrance.GetCenter(&cx, &cy);
    outX = cx;
    outY = cy;
    return true;
}

int MoreIconsManager::AddAllEnexMatching(const char* const* names, int nameCount, int iconId, std::vector<Blip>& outBlips) const
{
    CPool<CEntryExit>* pool = CEntryExitManager::mp_poolEntryExits;
    int added = 0;
    if (!pool || !pool->m_pObjects)
        return 0;

    for (int i = 0; i < pool->m_nSize; ++i)
    {
        CEntryExit* enex = pool->GetAt(i);
        if (!enex)
            continue;

        if (!IsExteriorEnex(enex))
            continue;

        for (int n = 0; n < nameCount; ++n)
        {
            if (names[n] && strncmp(enex->m_szName, names[n], 8) == 0)
            {
                float cx, cy;
                enex->m_recEntrance.GetCenter(&cx, &cy);
                AddBlip(outBlips, cx, cy, iconId);
                ++added;
                break;
            }
        }
    }
    return added;
}

void MoreIconsManager::AddRespawnBlips(int iconId, bool isHospital, std::vector<Blip>& outBlips) const
{
    short count = isHospital ? CRestart::NumberOfHospitalRestarts : CRestart::NumberOfPoliceRestarts;
    if (count <= 0)
        return;

    const CVector* points = isHospital ? CRestart::HospitalRestartPoints : CRestart::PoliceRestartPoints;
    if (!points)
        return;

    for (int i = 0; i < count && i < 10; i++)
    {
        AddBlip(outBlips, points[i].x, points[i].y, iconId);
    }
}

void MoreIconsManager::GetBlips(std::vector<Blip>& outBlips) const
{
    auto addCoordList = [this, &outBlips](const CoordEntry* list) {
        for (; list->iconId != 0; list++)
            AddBlip(outBlips, list->x, list->y, list->iconId);
    };

    // Enabled sections from example config
    AddAllEnexMatching(DONUTS_NAMES, 2, ICON_DONUTS, outBlips);
    if (AddAllEnexMatching(INTRACK_NAMES, 2, ICON_INTRACK, outBlips) == 0)
        addCoordList(INTRACK_COORDS);
    if (AddAllEnexMatching(CLUB_NAMES, 1, ICON_CLUB, outBlips) == 0)
        addCoordList(CLUB_COORDS);
    if (AddAllEnexMatching(BAR_NAMES, 3, ICON_BAR, outBlips) == 0)
        addCoordList(BAR_COORDS);
    if (AddAllEnexMatching(STRIP_NAMES, 4, ICON_STRIP, outBlips) == 0)
        addCoordList(STRIP_COORDS);
    // Skip restaurants (ICON_REST = 50) - uses standard blip.txd sprite, not More Icon
    // CLEO More Radar Icons loads restaurants from INI file, not hardcoded
    // AddAllEnexMatching(REST_NAMES, 4, ICON_REST, outBlips);
    AddAllEnexMatching(CASINO_NAMES, 3, ICON_CASINO, outBlips);
    addCoordList(BARBER);
    addCoordList(PIZZA);
    addCoordList(AIRPORT);
    addCoordList(SHIP);
    addCoordList(FIRE);
    addCoordList(RACE);
    addCoordList(GYM);
    addCoordList(TRAIN);

    AddRespawnBlips(ICON_POLICE, false, outBlips);
    AddRespawnBlips(ICON_HOSP, true, outBlips);
}
