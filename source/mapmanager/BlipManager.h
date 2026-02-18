/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/mapmanager/BlipManager.h
 *****************************************************************************/

#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <string>
#include "CRadar.h"
#include "RenderWare.h"
#include "BlipTypes.h"

class MoreIconsManager;

class BlipManager
{
public:
    static const int          MAX_BLIP_ID = 63;
    static const unsigned int UPDATE_INTERVAL = 50;

    BlipManager(LPDIRECT3DDEVICE9 pDevice);
    ~BlipManager();

    bool LoadTextures();
    void CleanupTextures();
    void UpdateFromGame();

    const std::vector<Blip>& GetBlips() const { return m_blips; }
    LPDIRECT3DTEXTURE9       GetBlipTexture(int spriteId) const;  // 0-63 txd, 64-69 more icons (PNG)

    enum MoreIconId
    {
        MORE_ICON_STORE = 64,
        MORE_ICON_DONUTS = 65,
        MORE_ICON_INTRACK = 66,
        MORE_ICON_CASINO = 67,
        MORE_ICON_DATENUDE = 68,
        MORE_ICON_TRAIN = 69,
    };
    LPDIRECT3DTEXTURE9       LoadTextureFromTxd(const char* texName) const;
    const char*              GetIconPath(int spriteId) const;

    static bool                 IsLegendSprite(unsigned char spriteId);
    static bool                 IsMissionCheckpointSprite(unsigned char spriteId);
    static eHeightIndicatorType GetHeightIndicatorType(float blipZ, float playerZ, float threshold = 2.0f);
    static DWORD                TraceColorToD3D(unsigned int blipColour, bool bright, bool friendly);

private:
    void  ParseRadarBlipSprites();
    void  LoadMoreIconTextures();
    void  CleanupMoreIconTextures();

    DWORD ConvertBlipColorToDWORD(unsigned int blipColour, bool bright, bool friendly) const;

    LPDIRECT3DDEVICE9   m_pDevice;
    RwTexDictionary*    m_pBlipTxd;
    LPDIRECT3DTEXTURE9  m_textures[MAX_BLIP_ID + 1];
    LPDIRECT3DTEXTURE9  m_moreIconTextures[6];  // store, donuts, intrack, casino, dateNude, train
    std::string         m_iconPaths[RADAR_SPRITE_COUNT];
    std::vector<Blip>   m_blips;
    unsigned int        m_lastUpdateTime;
    MoreIconsManager*   m_pMoreIconsManager;
};
