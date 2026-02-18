/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/mapmanager/BlipManager.cpp
 *****************************************************************************/

#include "BlipManager.h"
#include "MoreIconsManager.h"
#include "Config.h"
#include "GameState.h"
#include "plugin.h"
#include "CTimer.h"
#include "CFileLoader.h"
#include "CPools.h"
#include "CVehicle.h"
#include "CEntryExit.h"
#include "CRadar.h"
#include <cstring>

static LPDIRECT3DTEXTURE9 RwTextureToD3D9(LPDIRECT3DDEVICE9 pDevice, RwTexture* rwTex)
{
    if (!pDevice || !rwTex)
        return nullptr;

    RwRaster* raster = RwTextureGetRaster(rwTex);
    if (!raster)
        return nullptr;

    int w = RwRasterGetWidth(raster);
    int h = RwRasterGetHeight(raster);
    if (w <= 0 || h <= 0)
        return nullptr;

    RwImage* img = RwImageCreate(w, h, 32);
    if (!img)
        return nullptr;

    if (!RwImageAllocatePixels(img))
    {
        RwImageDestroy(img);
        return nullptr;
    }

    if (!RwImageSetFromRaster(img, raster))
    {
        RwImageFreePixels(img);
        RwImageDestroy(img);
        return nullptr;
    }

    LPDIRECT3DTEXTURE9 d3dTex = nullptr;
    HRESULT hr = pDevice->CreateTexture((UINT)w, (UINT)h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &d3dTex, nullptr);
    if (FAILED(hr) || !d3dTex)
    {
        RwImageFreePixels(img);
        RwImageDestroy(img);
        return nullptr;
    }

    D3DLOCKED_RECT locked;
    if (SUCCEEDED(d3dTex->LockRect(0, &locked, nullptr, 0)))
    {
        RwUInt8* src = RwImageGetPixels(img);
        int srcStride = RwImageGetStride(img);
        int dstStride = locked.Pitch;
        for (int y = 0; y < h; y++)
        {
            RwUInt8* rowSrc = src + y * srcStride;
            RwUInt8* rowDst = (RwUInt8*)locked.pBits + y * dstStride;
            for (int x = 0; x < w; x++)
            {
                int off = x * 4;
                rowDst[off + 0] = rowSrc[off + 2];
                rowDst[off + 1] = rowSrc[off + 1];
                rowDst[off + 2] = rowSrc[off + 0];
                rowDst[off + 3] = rowSrc[off + 3];
            }
        }
        d3dTex->UnlockRect(0);
    }

    RwImageFreePixels(img);
    RwImageDestroy(img);
    return d3dTex;
}

BlipManager::BlipManager(LPDIRECT3DDEVICE9 pDevice)
    : m_pDevice(pDevice)
    , m_pBlipTxd(nullptr)
    , m_lastUpdateTime(0)
    , m_pMoreIconsManager(nullptr)
{
    ZeroMemory(m_textures, sizeof(m_textures));
    ZeroMemory(m_moreIconTextures, sizeof(m_moreIconTextures));

    for (int i = 0; i < RADAR_SPRITE_COUNT; ++i)
    {
        if (i <= 1)
            m_iconPaths[i].clear();
        else
        {
            char path[64];
            sprintf_s(path, "radar/blip/%d", i);
            m_iconPaths[i] = path;
        }
    }
}

BlipManager::~BlipManager()
{
    if (m_pMoreIconsManager)
    {
        delete m_pMoreIconsManager;
        m_pMoreIconsManager = nullptr;
    }
    CleanupTextures();
}

bool BlipManager::LoadTextures()
{
    if (!m_pDevice)
        return false;

    if (!m_pMoreIconsManager)
        m_pMoreIconsManager = new MoreIconsManager();

    if (m_pBlipTxd)
    {
        RwTexDictionaryDestroy(m_pBlipTxd);
        m_pBlipTxd = nullptr;
    }

    CleanupTextures();

    const char* path = PLUGIN_PATH("radar/blip.txd");
    m_pBlipTxd = CFileLoader::LoadTexDictionary(path);
    if (!m_pBlipTxd)
        return false;

    bool allLoaded = true;
    for (int i = 2; i <= MAX_BLIP_ID; ++i)
    {
        char texName[16];
        sprintf_s(texName, "%d", i);

        RwTexture* rwTex = RwTexDictionaryFindNamedTexture(m_pBlipTxd, texName);
        if (!rwTex)
        {
            m_textures[i] = nullptr;
            allLoaded = false;
            continue;
        }

        m_textures[i] = RwTextureToD3D9(m_pDevice, rwTex);
        if (!m_textures[i])
            allLoaded = false;
    }

    LoadMoreIconTextures();

    if (!m_textures[48])
        m_textures[48] = LoadTextureFromTxd("dateDisco");
    if (!m_textures[49])
        m_textures[49] = LoadTextureFromTxd("dateDrink");

    return allLoaded;
}

LPDIRECT3DTEXTURE9 BlipManager::LoadTextureFromTxd(const char* texName) const
{
    if (!m_pBlipTxd || !m_pDevice || !texName)
        return nullptr;

    RwTexture* rwTex = RwTexDictionaryFindNamedTexture(m_pBlipTxd, texName);
    if (!rwTex)
        return nullptr;

    return RwTextureToD3D9(m_pDevice, rwTex);
}

void BlipManager::LoadMoreIconTextures()
{
    struct { const char* texName; int index; } texs[] = {
        { "store", 0 },
        { "donuts", 1 },
        { "intrack", 2 },
        { "casino", 3 },
        { "dateNude", 4 },
        { "train", 5 },
    };
    for (const auto& t : texs)
        m_moreIconTextures[t.index] = LoadTextureFromTxd(t.texName);
}

void BlipManager::CleanupMoreIconTextures()
{
    for (int i = 0; i < 6; i++)
    {
        if (m_moreIconTextures[i])
        {
            m_moreIconTextures[i]->Release();
            m_moreIconTextures[i] = nullptr;
        }
    }
}

void BlipManager::CleanupTextures()
{
    CleanupMoreIconTextures();
    for (int i = 0; i <= MAX_BLIP_ID; ++i)
    {
        if (m_textures[i])
        {
            m_textures[i]->Release();
            m_textures[i] = nullptr;
        }
    }

    if (m_pBlipTxd)
    {
        RwTexDictionaryDestroy(m_pBlipTxd);
        m_pBlipTxd = nullptr;
    }
}

void BlipManager::UpdateFromGame()
{
    unsigned int currentTime = CTimer::m_snTimeInMilliseconds;
    if (currentTime - m_lastUpdateTime < UPDATE_INTERVAL)
        return;

    m_lastUpdateTime = currentTime;
    m_blips.clear();
    ParseRadarBlipSprites();
}

void BlipManager::ParseRadarBlipSprites()
{
    tRadarTrace* ms_RadarTrace = CRadar::ms_RadarTrace;
    CSprite2d* RadarBlipSprites = CRadar::RadarBlipSprites;
    if (!ms_RadarTrace || !RadarBlipSprites)
        return;

    auto spriteValid = [this, RadarBlipSprites](unsigned char id, CSprite2d*& outSprite) -> bool {
        if (id >= MAX_RADAR_SPRITES)
            return false;
        CSprite2d* s = &RadarBlipSprites[id];
        if (!s || !s->m_pTexture || !s->m_pTexture->raster || !m_textures[id])
            return false;
        outSprite = s;
        return true;
    };

    for (unsigned int i = 0; i < MAX_RADAR_TRACES; i++)
    {
        tRadarTrace& gameBlip = ms_RadarTrace[i];
        if (!gameBlip.m_bInUse
            || gameBlip.m_nRadarSprite == RADAR_SPRITE_NORTH
            || gameBlip.m_nRadarSprite == RADAR_SPRITE_CJ
            || gameBlip.m_nBlipDisplay == BLIP_DISPLAY_NEITHER)
            continue;

        eBlipType blipType = (eBlipType)gameBlip.m_nBlipType;
        if (blipType == BLIP_OBJECT)
            continue;

        // Skip BLIP_CHAR unless it's a legend sprite (story characters like Sweet, Ryder, etc.)
        if (blipType == BLIP_CHAR && !IsLegendSprite(gameBlip.m_nRadarSprite))
            continue;

        bool isMissionMarker = (blipType == BLIP_COORD || blipType == BLIP_CONTACTPOINT || blipType == BLIP_SPOTLIGHT);
        unsigned char spriteId = gameBlip.m_nRadarSprite;
        if (spriteId >= MAX_RADAR_SPRITES && isMissionMarker)
            spriteId = RADAR_SPRITE_WAYPOINT;
        if (spriteId >= MAX_RADAR_SPRITES)
            continue;

        CSprite2d* sprite = nullptr;
        if (!spriteValid(spriteId, sprite) && isMissionMarker)
        {
            spriteId = RADAR_SPRITE_WAYPOINT;
            if (!spriteValid(spriteId, sprite))
                continue;
        }
        else if (!sprite)
            continue;

        Blip blip;
        CVector blipPos = gameBlip.m_vecPos;

        if (gameBlip.m_nEntityHandle != 0 && blipType == BLIP_CAR)
        {
            try
            {
                CVehicle* vehicle = CPools::GetVehicle(gameBlip.m_nEntityHandle);
                if (vehicle)
                {
                    blipPos = vehicle->GetPosition();
                    spriteId = 0;
                    if (!spriteValid(spriteId, sprite))
                        continue;
                }
            }
            catch (...) {}
        }

        // Skip blips associated with interior exits - only show exterior entrances
        if (gameBlip.m_pEntryExit)
        {
            try
            {
                CEntryExit* enex = gameBlip.m_pEntryExit;
                // Skip interior exits - only show exterior entrances (like More Radar Icons CLEO script)
                if (enex->m_nArea != 0)
                    continue;
                blipPos.x = (enex->m_recEntrance.left + enex->m_recEntrance.right) * 0.5f;
                blipPos.y = (enex->m_recEntrance.top + enex->m_recEntrance.bottom) * 0.5f;
            }
            catch (...) {}
        }

        // Skip sprite 50 (restaurant fork&knife) - it's added by MoreIconsManager only when Enex found
        // Game creates this sprite for various locations but CLEO More Radar Icons doesn't show them
        //if (spriteId == 50)
        //    continue;

        int w = RwRasterGetWidth(sprite->m_pTexture->raster);
        int h = RwRasterGetHeight(sprite->m_pTexture->raster);
        blip.position = D3DXVECTOR3(blipPos.x + 3000.0f, blipPos.y - 3000.0f, 0.1f);
        blip.iconId = spriteId;
        blip.size = (w + h) > 0 ? (float)((w + h) / 2) : 16.0f;
        blip.color = ConvertBlipColorToDWORD(gameBlip.m_nColour, gameBlip.m_bBright, gameBlip.m_bFriendly);
        blip.enabled = true;
        blip.shortRange = gameBlip.m_bShortRange ? true : false;
        m_blips.push_back(blip);
    }

    if (RadarConfig::GetModeMoreIcon() && m_pMoreIconsManager)
    {
        m_pMoreIconsManager->GetBlips(m_blips);
    }
}

LPDIRECT3DTEXTURE9 BlipManager::GetBlipTexture(int spriteId) const
{
    if (spriteId >= MORE_ICON_STORE && spriteId <= MORE_ICON_TRAIN)
        return m_moreIconTextures[spriteId - MORE_ICON_STORE];
    if (spriteId < 0 || spriteId > MAX_BLIP_ID)
        return nullptr;
    return m_textures[spriteId];
}

DWORD BlipManager::ConvertBlipColorToDWORD(unsigned int blipColour, bool bright, bool friendly) const
{
    return TraceColorToD3D(blipColour, bright, friendly);
}

DWORD BlipManager::TraceColorToD3D(unsigned int blipColour, bool bright, bool friendly)
{
    CRGBA color = CRadar::GetRadarTraceColour(blipColour, bright ? 1 : 0, friendly ? 1 : 0);
    return D3DCOLOR_ARGB(color.a, color.r, color.g, color.b);
}

const char* BlipManager::GetIconPath(int spriteId) const
{
    if (spriteId < 0 || spriteId >= RADAR_SPRITE_COUNT)
        return nullptr;
    return m_iconPaths[spriteId].c_str();
}

bool BlipManager::IsLegendSprite(unsigned char spriteId)
{
    switch (spriteId)
    {
    case RADAR_SPRITE_BIGSMOKE:
    case RADAR_SPRITE_CATALINAPINK:
    case RADAR_SPRITE_CESARVIAPANDO:
    case RADAR_SPRITE_CJ:
    case RADAR_SPRITE_CRASH1:
    case RADAR_SPRITE_MCSTRAP:
    case RADAR_SPRITE_OGLOC:
    case RADAR_SPRITE_RYDER:
    case RADAR_SPRITE_SWEET:
    case RADAR_SPRITE_THETRUTH:
    case RADAR_SPRITE_TORENORANCH:
    case RADAR_SPRITE_WOOZIE:
    case RADAR_SPRITE_ZERO:
        return true;
    default:
        return false;
    }
}

bool BlipManager::IsMissionCheckpointSprite(unsigned char spriteId)
{
    switch (spriteId)
    {
    case RADAR_SPRITE_NONE:
    case RADAR_SPRITE_QMARK:
        return true;
    default:
        return false;
    }
}

eHeightIndicatorType BlipManager::GetHeightIndicatorType(float blipZ, float playerZ, float threshold)
{
    float diff = blipZ - playerZ;
    if (diff > threshold)
        return HEIGHT_INDICATOR_ABOVE;
    if (diff < -threshold)
        return HEIGHT_INDICATOR_BELOW;
    return HEIGHT_INDICATOR_SAME;
}
