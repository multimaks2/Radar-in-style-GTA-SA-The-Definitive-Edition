/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/render/RadarRenderer.cpp
 *****************************************************************************/

#include "RadarRenderer.h"
#include "RadarGeometry.h"
#include "RadarViewContext.h"
#include "GameState.h"
#include "Config.h"
#include "DxDrawPrimitives.h"
#include "RenderTarget.h"
#include "ShaderCode.h"
#include "ShaderManager.h"
#include "CameraController.h"
#include "MapChunkManager.h"
#include "BlipManager.h"
#include "GangZoneRenderer.h"
#include "GpsRender.h"
#include "RenderRadio.h"
#include "MathUtils.h"
#include <cstring>
#include <cmath>
#include <cfloat>
#include <vector>
#include "plugin.h"
#include "common.h"
#include "RenderWare.h"
#include "CWorld.h"
#include "CPlayerPed.h"
#include "CModelInfo.h"
#include "CTimer.h"
#include "CPools.h"
#include "CTimer.h"
#include "Base64Image.h"
#include "CPed.h"
#include "CVehicle.h"
#include "CProjectileInfo.h"
#include "CProjectile.h"
#include "eWeaponType.h"
#include "CRadar.h"
#include "CAERadioTrackManager.h"
#ifdef _DEBUG
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")
#endif


RadarRenderer::RadarRenderer()
    : m_pd3dDevice(nullptr)
    , m_pShaderManager(nullptr)
    , m_pCameraController(nullptr)
    , m_pMapChunkManager(nullptr)
    , m_pBlipManager(nullptr)
    , m_pGangZoneRenderer(nullptr)
    , m_pGpsRenderer(nullptr)
    , m_pRenderRadio(nullptr)
    , m_pFont(nullptr)
    , m_pFontSprite(nullptr)
    , m_pRadioFont(nullptr)
    , m_pRadioFontSprite(nullptr)
    , m_pTriangleVB(nullptr)
    , m_pTriangleEffect(nullptr)
    , m_pBorderEffect(nullptr)
    , m_pImage3DEffect(nullptr)
    , m_pLineEffect(nullptr)
    , m_pLineSmoothEffect(nullptr)
    , m_pGreenSquareEffect(nullptr)
    , m_pCircleTexture(nullptr)
    , m_pNorthTexture(nullptr)
    , m_pLineTexture(nullptr)
    , m_pRadarRingPlaneTexture(nullptr)
    , m_pDraw(nullptr)
    , m_pRenderTarget(nullptr)
    , m_width(0)
    , m_height(0)
    , m_cachedPlayer(nullptr)
    , m_cachedIsInAircraft(false)
    , m_cachedIsInPlane(false)
    , m_cachedRollAngle(0.0f)
    , m_cachedPitchAngle(0.0f)
    , m_nearPlane(0.3f)
    , m_farPlane(10000.0f)
    , m_initialAircraftAltitude(0.0f)
    , m_bWasInAircraft(false)
    , m_bWasInInterior(false)
    , m_exitInteriorImageStartTime(0)
    , m_pExitInteriorTexture(nullptr)
    , m_bShowGangZones(true)
    , m_bRadarShapeCircle(true)
    , m_bBorderShapeCircle(true)
    , m_bInitialized(false)
#ifdef _DEBUG
    , m_debugChunksRenderedLastFrame(0)
#endif
{
}

RadarRenderer::~RadarRenderer()
{
    Shutdown();
}

bool RadarRenderer::Initialize(LPDIRECT3DDEVICE9 pDevice)
{
    if (!pDevice)
        return false;

    m_pd3dDevice = pDevice;
    
    m_width = RsGlobal.maximumWidth;
    m_height = RsGlobal.maximumHeight;

    m_pShaderManager = new ShaderManager(m_pd3dDevice);
    if (!m_pShaderManager->Initialize())
    {
        return false;
    }

    // Initialize helper classes
    m_pCameraController = new CameraController();
    m_pMapChunkManager = new MapChunkManager(m_pd3dDevice);
    m_pMapChunkManager->Initialize();
    m_pBlipManager = new BlipManager(m_pd3dDevice);
    m_pGangZoneRenderer = new GangZoneRenderer(m_pd3dDevice);
    
    if (!m_pBlipManager->LoadTextures())
    {
        // Non-critical, continue anyway
    }
    
    if (m_pGangZoneRenderer)
        m_pGangZoneRenderer->SetEnabled(m_bShowGangZones);

    m_pGpsRenderer = new GpsRenderer(m_pd3dDevice);
    if (!m_pGpsRenderer->Initialize())
    {
        delete m_pGpsRenderer;
        m_pGpsRenderer = nullptr;
    }

    if (!InitializeResources())
    {
        return false;
    }

    m_pRenderTarget = new RenderTarget(m_pd3dDevice);
    UpdateDrawResources();
    m_pDraw = new DxDrawPrimitives(m_pd3dDevice);
    if (!m_pDraw->Initialize(&m_drawResources))
    {
        delete m_pDraw;
        m_pDraw = nullptr;
        delete m_pRenderTarget;
        m_pRenderTarget = nullptr;
        return false;
    }

    m_pRenderRadio = new RenderRadio(m_pDraw);
    if (m_pRadioFont)
    {
        m_pRenderRadio->SetFont(m_pRadioFont);
    }

    m_bInitialized = true;
    return true;
}

void RadarRenderer::Shutdown()
{
    if (m_pDraw)
    {
        m_pDraw->Shutdown();
        delete m_pDraw;
        m_pDraw = nullptr;
    }
    if (m_pRenderTarget)
    {
        delete m_pRenderTarget;
        m_pRenderTarget = nullptr;
    }

    CleanupResources();
    
    // Cleanup helper classes
    if (m_pGangZoneRenderer)
    {
        delete m_pGangZoneRenderer;
        m_pGangZoneRenderer = nullptr;
    }
    if (m_pGpsRenderer)
    {
        m_pGpsRenderer->Shutdown();
        delete m_pGpsRenderer;
        m_pGpsRenderer = nullptr;
    }
    if (m_pRenderRadio)
    {
        delete m_pRenderRadio;
        m_pRenderRadio = nullptr;
    }
    if (m_pBlipManager)
    {
        m_pBlipManager->CleanupTextures();
        delete m_pBlipManager;
        m_pBlipManager = nullptr;
    }
    
    if (m_pMapChunkManager)
    {
        m_pMapChunkManager->Cleanup();
        delete m_pMapChunkManager;
        m_pMapChunkManager = nullptr;
    }
    
    if (m_pCameraController)
    {
        delete m_pCameraController;
        m_pCameraController = nullptr;
    }
    
    // m_pNorthTexture from BlipManager (GetBlipTexture(4)), do not Release
    m_pNorthTexture = nullptr;
    
    if (m_pFont)
    {
        m_pFont->Release();
        m_pFont = nullptr;
    }
    if (m_pFontSprite)
    {
        m_pFontSprite->Release();
        m_pFontSprite = nullptr;
    }
    if (m_pRadioFont)
    {
        m_pRadioFont->Release();
        m_pRadioFont = nullptr;
    }
    if (m_pRadioFontSprite)
    {
        m_pRadioFontSprite->Release();
        m_pRadioFontSprite = nullptr;
    }

    if (m_pShaderManager)
    {
        m_pShaderManager->Shutdown();
        delete m_pShaderManager;
        m_pShaderManager = nullptr;
    }

    m_bInitialized = false;
}

void RadarRenderer::SetShowGangZones(bool value)
{
    m_bShowGangZones = value;
    if (m_pGangZoneRenderer)
        m_pGangZoneRenderer->SetEnabled(value);
}

bool RadarRenderer::InitializeResources()
{
    if (!m_pd3dDevice || !m_pShaderManager)
    {
        return false;
    }

    const char* circleShaderCode = GetCircleShaderCode();
    m_pTriangleEffect = m_pShaderManager->LoadEffectFromString(circleShaderCode, "CircleShader");
    if (!m_pTriangleEffect)
    {
        return false;
    }

    const char* borderShaderCode = GetBorderShaderCode();
    m_pBorderEffect = m_pShaderManager->LoadEffectFromString(borderShaderCode, "BorderShader");
    if (!m_pBorderEffect)
    {
        return false;
    }

    const char* image3DShaderCode = GetImage3DShaderCode();
    m_pImage3DEffect = m_pShaderManager->LoadEffectFromString(image3DShaderCode, "Image3DShader");
    if (!m_pImage3DEffect)
    {
        return false;
    }

    const char* lineShaderCode = GetLineShaderCode();
    m_pLineEffect = m_pShaderManager->LoadEffectFromString(lineShaderCode, "LineShader");
    if (!m_pLineEffect)
        return false;

    const char* lineSmoothShaderCode = GetLineSmoothShaderCode();
    m_pLineSmoothEffect = m_pShaderManager->LoadEffectFromString(lineSmoothShaderCode, "LineSmoothShader");
    if (!m_pLineSmoothEffect)
        return false;

    const char* greenSquareShaderCode = GetGreenSquareShaderCode();
    m_pGreenSquareEffect = m_pShaderManager->LoadEffectFromString(greenSquareShaderCode, "GreenSquareShader");
    if (!m_pGreenSquareEffect)
    {
        return false;
    }

    // Render target is created in Render() with radar size (sizeX x sizeY) for correct aspect ratio

    struct CircleVertex
    {
        float x, y, z;
        DWORD color;
        float u, v;
    };
    
    CircleVertex circle[] = {
        { -0.5f,  0.5f, 0.5f, D3DCOLOR_XRGB(255, 255, 255), 0.0f, 0.0f },
        { -0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(255, 255, 255), 0.0f, 1.0f },
        {  0.5f,  0.5f, 0.5f, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0.0f },
        { -0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(255, 255, 255), 0.0f, 1.0f },
        {  0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 1.0f },
        {  0.5f,  0.5f, 0.5f, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0.0f }
    };

    if (FAILED(m_pd3dDevice->CreateVertexBuffer(
        6 * sizeof(CircleVertex),
        D3DUSAGE_WRITEONLY,
        D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1,
        D3DPOOL_DEFAULT,
        &m_pTriangleVB,
        nullptr)))
    {
        return false;
    }

    void* pVertices = nullptr;
    if (FAILED(m_pTriangleVB->Lock(0, sizeof(circle), &pVertices, 0)))
    {
        return false;
    }
    memcpy(pVertices, circle, sizeof(circle));
    m_pTriangleVB->Unlock();

    // Fallback for dxDrawCircleShader when render target is null (map is from chunks in render target)
    if (SUCCEEDED(D3DXCreateTexture(m_pd3dDevice, 1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_pCircleTexture)))
    {
        D3DLOCKED_RECT locked;
        if (SUCCEEDED(m_pCircleTexture->LockRect(0, &locked, nullptr, 0)))
        {
            *(DWORD*)locked.pBits = 0xFFFFFFFF;
            m_pCircleTexture->UnlockRect(0);
        }
    }

    
    if (m_pBlipManager)
        m_pNorthTexture = m_pBlipManager->GetBlipTexture(4);

    // Line from blip.txd
    if (m_pBlipManager)
    {
        m_pLineTexture = m_pBlipManager->LoadTextureFromTxd("line");
        if (!m_pLineTexture)
            m_pLineTexture = m_pBlipManager->LoadTextureFromTxd("radarLine");
    }
    if (!m_pLineTexture)
    {
        const char* fallback = PLUGIN_PATH("radar/blip/line.png");
        D3DXCreateTextureFromFileA(m_pd3dDevice, fallback, &m_pLineTexture);
    }

    // RingPlane from blip.txd
    if (m_pBlipManager)
    {
        m_pRadarRingPlaneTexture = m_pBlipManager->LoadTextureFromTxd("radarRingPlane");
        if (!m_pRadarRingPlaneTexture)
            m_pRadarRingPlaneTexture = m_pBlipManager->LoadTextureFromTxd("RingPlane");
    }
    if (!m_pRadarRingPlaneTexture)
    {
        const char* fallback = PLUGIN_PATH("radar/blip/RingPlane.png");
        D3DXCreateTextureFromFileA(m_pd3dDevice, fallback, &m_pRadarRingPlaneTexture);
    }
    
    // Create DirectX font for text rendering
    D3DXFONT_DESC fontDesc;
    ZeroMemory(&fontDesc, sizeof(fontDesc));
    fontDesc.Height = 15;
    fontDesc.Width = 0;
    fontDesc.Weight = FW_NORMAL;
    fontDesc.MipLevels = 1;
    fontDesc.Italic = FALSE;
    fontDesc.CharSet = DEFAULT_CHARSET;
    fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
    fontDesc.Quality = DEFAULT_QUALITY;
    fontDesc.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    strcpy_s(fontDesc.FaceName, "Arial");
    
    if (FAILED(D3DXCreateFontIndirect(m_pd3dDevice, &fontDesc, &m_pFont)))
    {
        return false;
    }
    if (FAILED(D3DXCreateSprite(m_pd3dDevice, &m_pFontSprite)))
    {
        m_pFontSprite = nullptr;
    }

    // Создаём шрифт для радио текста (большего размера)
    D3DXFONT_DESC radioFontDesc;
    ZeroMemory(&radioFontDesc, sizeof(radioFontDesc));
    radioFontDesc.Height = 32;  // Увеличенный размер шрифта
    radioFontDesc.Width = 0;
    radioFontDesc.Weight = FW_BOLD;  // Жирный шрифт для лучшей читаемости
    radioFontDesc.MipLevels = 1;
    radioFontDesc.Italic = FALSE;
    radioFontDesc.CharSet = DEFAULT_CHARSET;
    radioFontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
    radioFontDesc.Quality = DEFAULT_QUALITY;
    radioFontDesc.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    strcpy_s(radioFontDesc.FaceName, "Arial");

    if (FAILED(D3DXCreateFontIndirect(m_pd3dDevice, &radioFontDesc, &m_pRadioFont)))
    {
        m_pRadioFont = nullptr;
    }
    if (FAILED(D3DXCreateSprite(m_pd3dDevice, &m_pRadioFontSprite)))
    {
        m_pRadioFontSprite = nullptr;
    }

    int exitImgW = 0, exitImgH = 0;
    m_pExitInteriorTexture = (LPDIRECT3DTEXTURE9)Base64Image::CreateTextureFromBase64(
        m_pd3dDevice, Base64Image::GetEmbeddedImageBase64(), exitImgW, exitImgH);
    if (!m_pExitInteriorTexture)
    {
            // Not critical - exit-from-interior effect simply won't show the image
    }
    Base64Image::ClearEmbeddedImageData();

    return true;
}

void RadarRenderer::UpdateDrawResources()
{
    m_drawResources.pDevice             = m_pd3dDevice;
    m_drawResources.pTriangleEffect      = m_pTriangleEffect;
    m_drawResources.pBorderEffect        = m_pBorderEffect;
    m_drawResources.pImage3DEffect       = m_pImage3DEffect;
    m_drawResources.pLineEffect          = m_pLineEffect;
    m_drawResources.pLineSmoothEffect     = m_pLineSmoothEffect;
    m_drawResources.pGreenSquareEffect   = m_pGreenSquareEffect;
    m_drawResources.pCircleTexture       = m_pCircleTexture;
    m_drawResources.pNorthTexture        = m_pNorthTexture;
    m_drawResources.pLineTexture         = m_pLineTexture;
    m_drawResources.pRadarRingPlaneTexture = m_pRadarRingPlaneTexture;
    m_drawResources.pFont                = m_pFont;
    m_drawResources.pFontSprite          = m_pFontSprite;
    m_drawResources.screenWidth          = m_width;
    m_drawResources.screenHeight         = m_height;
    m_drawResources.radarShapeCircle     = m_bRadarShapeCircle;
    m_drawResources.borderShapeCircle    = m_bBorderShapeCircle;
    if (m_pRenderTarget)
    {
        m_drawResources.renderTargetWidth  = m_pRenderTarget->GetWidth();
        m_drawResources.renderTargetHeight = m_pRenderTarget->GetHeight();
    }
    else
    {
        m_drawResources.renderTargetWidth  = 0;
        m_drawResources.renderTargetHeight = 0;
    }
}

void RadarRenderer::CleanupResources()
{
    if (m_pCircleTexture)
    {
        m_pCircleTexture->Release();
        m_pCircleTexture = nullptr;
    }


    if (m_pLineTexture)
    {
        m_pLineTexture->Release();
        m_pLineTexture = nullptr;
    }

    if (m_pRadarRingPlaneTexture)
    {
        m_pRadarRingPlaneTexture->Release();
        m_pRadarRingPlaneTexture = nullptr;
    }

    if (m_pExitInteriorTexture)
    {
        m_pExitInteriorTexture->Release();
        m_pExitInteriorTexture = nullptr;
    }

    if (m_pTriangleVB)
    {
        m_pTriangleVB->Release();
        m_pTriangleVB = nullptr;
    }

    m_pTriangleEffect = nullptr;
    m_pBorderEffect = nullptr;
    m_pImage3DEffect = nullptr;
    m_pLineEffect = nullptr;
    m_pLineSmoothEffect = nullptr;
    m_pGreenSquareEffect = nullptr;
}


void RadarRenderer::CalculateRadarPosition(float& circleX, float& circleY, float& sizeX, float& sizeY)
{
    float baseCircle = (float)RadarConfig::GetCircleSize();
    float baseSqX = (float)RadarConfig::GetSquareSizeX();
    float baseSqY = (float)RadarConfig::GetSquareSizeY();
    float baseOffsetX = (float)RadarConfig::GetOffsetX();
    float baseOffsetY = (float)RadarConfig::GetOffsetY();
#ifdef _DEBUG
    // Как в trilogy: offset 85 + 2 размера радара (в базовых 1920p единицах)
    float baseRadarSize = m_bRadarShapeCircle ? baseCircle : baseSqX;
    baseOffsetX = 85.0f + baseRadarSize;
#endif
    MathUtils::CalculateRadarPosition(circleX, circleY, sizeX, sizeY, baseCircle, baseSqX, baseSqY, m_bRadarShapeCircle, baseOffsetX, baseOffsetY);
}

float RadarRenderer::CalculateBlipSize(float baseBlipSize, float baseWidth) const
{
    float screenWidth = (float)RsGlobal.maximumWidth;
    float scaleX = screenWidth / baseWidth;
    return baseBlipSize * scaleX;
}

void RadarRenderer::RenderRadarTargetContents(float circleX, float circleY, float sizeX, float sizeY)
{
    (void)circleX; (void)circleY; (void)sizeX; (void)sizeY;
    if (m_pImage3DEffect && m_pCameraController && m_pMapChunkManager)
    {
        float offsetWorldX, offsetWorldY;
        D3DXVECTOR3 cameraPos, cameraRot;
        m_pCameraController->GetCachedCalculations(offsetWorldX, offsetWorldY, cameraPos, cameraRot);
        const CameraController::CameraState& camState = m_pCameraController->GetState();
        float visibleRadius = MapChunkManager::ComputeVisibleRadius(cameraPos.z, camState.fov, camState.offsetY, m_cachedIsInAircraft);
        if (!m_bRadarShapeCircle)
            visibleRadius *= 1.65f;  // square corners extend beyond circle; extra margin for square radar

        MapChunkManager::FrustumParams frustum = {};
        frustum.cameraPos = &cameraPos;
        frustum.cameraRot = &cameraRot;
        frustum.fov = camState.fov;
        frustum.nearPlane = m_nearPlane;
        frustum.farPlane = m_farPlane;
        frustum.screenWidth = m_pRenderTarget ? (float)m_pRenderTarget->GetWidth() : 0.0f;
        frustum.screenHeight = m_pRenderTarget ? (float)m_pRenderTarget->GetHeight() : 0.0f;
        frustum.projectionAspect = (frustum.screenWidth > 0) ? (frustum.screenHeight / frustum.screenWidth) : 1.0f;

        m_pMapChunkManager->ForEachChunkInRadius(cameraPos, visibleRadius, &frustum, [this, &cameraPos, &cameraRot, &camState](int, const D3DXVECTOR3& elementPos, const D3DXVECTOR3& elementRot, const D3DXVECTOR2& elementSize, LPDIRECT3DTEXTURE9 chunkTex)
        {
            m_pDraw->dxDrawImage3D(elementPos, elementRot, elementSize, cameraPos, cameraRot, camState.fov, m_nearPlane, m_farPlane, chunkTex, tocolor(255, 255, 255, 255));
#ifdef _DEBUG
            ++m_debugChunksRenderedLastFrame;
#endif
        });
    }
    if (m_pGangZoneRenderer && m_pCameraController)
    {
        float offsetWorldX, offsetWorldY;
        D3DXVECTOR3 cameraPos, cameraRot;
        m_pCameraController->GetCachedCalculations(offsetWorldX, offsetWorldY, cameraPos, cameraRot);
        const CameraController::CameraState& camState = m_pCameraController->GetState();
        m_pGangZoneRenderer->Render(cameraPos, cameraRot, camState.fov, m_nearPlane, m_farPlane,
            camState.posX, camState.posY, m_pCircleTexture ? m_pCircleTexture : m_pLineTexture,
            [this](const D3DXVECTOR3& pos, const D3DXVECTOR3& rot, const D3DXVECTOR2& size,
                const D3DXVECTOR3& camPos, const D3DXVECTOR3& camRot,
                float fov, float nearPlane, float farPlane,
                LPDIRECT3DTEXTURE9 tex, DWORD color) {
            m_pDraw->dxDrawImage3D(pos, rot, size, camPos, camRot, fov, nearPlane, farPlane, tex, color);
        });
    }
    if (m_pBlipManager)
        m_pBlipManager->UpdateFromGame();

    // GPS line: render to RT BEFORE player blip so line is under icon
    if (m_pGpsRenderer && m_pCameraController && m_pRenderTarget)
    {
        float rtWidth = (float)m_pRenderTarget->GetWidth();
        float rtHeight = (float)m_pRenderTarget->GetHeight();
        if (rtWidth > 0 && rtHeight > 0)
        {
            float centerX = rtWidth * 0.5f;
            float centerY = rtHeight * 0.5f;
            float halfX, halfY;
            RadarGeometry::GetRadarHalfExtents(rtWidth, rtHeight, (float)RadarConfig::GetBorderThickness(), m_bRadarShapeCircle, halfX, halfY);
            float offsetWorldX, offsetWorldY;
            D3DXVECTOR3 cameraPos, cameraRot;
            m_pCameraController->GetCachedCalculations(offsetWorldX, offsetWorldY, cameraPos, cameraRot);
            const CameraController::CameraState& camState = m_pCameraController->GetState();
            float projectionAspect = (m_width > 0) ? ((float)m_height / (float)m_width) : (rtHeight / rtWidth);
            m_pGpsRenderer->Render(m_pDraw, centerX, centerY, rtWidth, rtHeight,
                cameraPos, cameraRot, camState.fov, m_nearPlane, m_farPlane,
                rtWidth, rtHeight, projectionAspect,
                m_bRadarShapeCircle, halfX, halfY);
        }
    }

    RenderBlips();
}

LPDIRECT3DTEXTURE9 RadarRenderer::GetRenderTargetTexture() const
{
    return m_pRenderTarget ? m_pRenderTarget->GetRenderTargetTexture() : nullptr;
}

void RadarRenderer::RenderRadarOverlays(float circleX, float circleY, float sizeX, float sizeY)
{
    LPDIRECT3DTEXTURE9 rtTex = GetRenderTargetTexture();
    LPDIRECT3DTEXTURE9 textureToUse = rtTex ? rtTex : m_pCircleTexture;
    float scale = RadarGeometry::GetRadarScale();

    DWORD oldAlphaBlend = 0, oldAlphaTest = 0;
    m_pd3dDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAlphaBlend);
    m_pd3dDevice->GetRenderState(D3DRS_ALPHATESTENABLE, &oldAlphaTest);
    m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    m_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

    int circleR, circleG, circleB, circleA;
    RadarConfig::GetCircleColor(circleR, circleG, circleB, circleA);
    m_pDraw->dxDrawCircleShader(circleX, circleY, sizeX, sizeY, textureToUse, tocolor(circleR, circleG, circleB, circleA));
    m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlend);
    m_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, oldAlphaTest);

    if (m_cachedIsInPlane && m_bRadarShapeCircle)
    {
        static LPDIRECT3DTEXTURE9 whiteTexture = nullptr;
        if (!whiteTexture && m_pd3dDevice)
        {
            if (SUCCEEDED(D3DXCreateTexture(m_pd3dDevice, 1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &whiteTexture)))
            {
                D3DLOCKED_RECT lockedRect;
                if (SUCCEEDED(whiteTexture->LockRect(0, &lockedRect, nullptr, 0)))
                {
                    DWORD* pData = (DWORD*)lockedRect.pBits;
                    *pData = 0xFFFFFFFF;
                    whiteTexture->UnlockRect(0);
                }
            }
        }
        if (m_pGreenSquareEffect && whiteTexture)
        {
            float pitchAngle = m_cachedPitchAngle;
            float fillLevel = (pitchAngle + D3DX_PI * 0.5f) / D3DX_PI;
            if (fillLevel < 0.0f) fillLevel = 0.0f;
            if (fillLevel > 1.0f) fillLevel = 1.0f;
            m_pDraw->dxDrawGreenSquareFill(circleX, circleY, sizeX, sizeY, whiteTexture, tocolor(0, 255, 0, 155), fillLevel);
        }
        if (m_pRadarRingPlaneTexture)
        {
            float rollAngle = m_cachedRollAngle;
            m_pDraw->dxDrawImage2DRotated(circleX, circleY, sizeX, sizeY, m_pRadarRingPlaneTexture, rollAngle, tocolor(255, 255, 255, 225));
        }
    }

    if (m_pExitInteriorTexture && m_exitInteriorImageStartTime != 0)
    {
        unsigned int now = CTimer::m_snTimeInMilliseconds;
        unsigned int elapsed = now - m_exitInteriorImageStartTime;
        if (elapsed >= 10000)
        {
            m_exitInteriorImageStartTime = 0;
            m_pExitInteriorTexture->Release();
            m_pExitInteriorTexture = nullptr;
        }
        else
        {
            float alpha = 1.0f - (elapsed / 10000.0f);
            DWORD oldAb = 0, oldSb = 0, oldDb = 0;
            m_pd3dDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAb);
            m_pd3dDevice->GetRenderState(D3DRS_SRCBLEND, &oldSb);
            m_pd3dDevice->GetRenderState(D3DRS_DESTBLEND, &oldDb);
            m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

            int circleR, circleG, circleB, circleA;
            RadarConfig::GetCircleColor(circleR, circleG, circleB, circleA);
            DWORD circleColor = tocolor(circleR, circleG, circleB, circleA);
            BYTE alphaByte = (BYTE)(alpha * 255.0f);
            circleColor = (circleColor & 0x00FFFFFF) | (alphaByte << 24);

            m_pDraw->dxDrawCircleShader(circleX, circleY, sizeX, sizeY, m_pExitInteriorTexture, circleColor);
            m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAb);
            m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, oldSb);
            m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, oldDb);
        }
    }

    float borderThicknessPx = (float)RadarConfig::GetBorderThickness();
    int borderR, borderG, borderB, borderA;
    RadarConfig::GetBorderColor(borderR, borderG, borderB, borderA);
    DWORD borderColor = tocolor(borderR, borderG, borderB, borderA);

    if (m_bBorderShapeCircle)
        m_pDraw->dxDrawBorderShader(circleX, circleY, sizeX, sizeY, borderColor, 1.0f, borderThicknessPx);
    else
    {
        float margin = borderThicknessPx;
        float borderSizeX = sizeX + 2.0f * margin;
        float borderSizeY = sizeY + 2.0f * margin;
        m_pDraw->dxDrawBorderShader(circleX - margin, circleY - margin, borderSizeX, borderSizeY, borderColor, 1.0f, margin);
    }

    bool bInAircraft = m_cachedIsInAircraft;
    if (bInAircraft && !m_bWasInAircraft)
    {
        try {
            if (m_cachedPlayer && m_cachedPlayer->bInVehicle && m_cachedPlayer->m_pVehicle)
            {
                CVector pos = m_cachedPlayer->GetPosition();
                m_initialAircraftAltitude = pos.z;
            }
        }
        catch (...) { m_initialAircraftAltitude = 0.0f; }
    }
    m_bWasInAircraft = bInAircraft;

    if (bInAircraft)
    {
        float stripX = circleX + sizeX, stripY = circleY;
        float stripWidth = 20.0f * scale, stripHeight = sizeY;
        float currentAltitude = 0.0f;
        try {
            if (m_cachedPlayer && m_cachedPlayer->bInVehicle && m_cachedPlayer->m_pVehicle)
                currentAltitude = m_cachedPlayer->GetPosition().z - m_initialAircraftAltitude;
        }
        catch (...) {}
        if (currentAltitude < 0.0f) currentAltitude = 0.0f;
        if (currentAltitude > 250.0f) currentAltitude = 250.0f;
        float progressBarY = stripY + sizeY - (sizeY * currentAltitude / 250.0f);
        m_pDraw->dxDrawRectangle(stripX + stripWidth, stripY, stripWidth, stripHeight, tocolor(0, 0, 0, 100));
        m_pDraw->dxDrawRectangle(stripX + stripWidth * 0.85f, progressBarY, stripWidth * 1.3f, stripWidth * 0.2f, tocolor(255, 255, 255, 225));
    }

    if (m_pNorthTexture && m_pCameraController)
    {
        float northMarkerSize = CalculateBlipSize(28.0f);
        float centerX = circleX + sizeX * 0.5f, centerY = circleY + sizeY * 0.5f;
        float halfX, halfY;
        RadarGeometry::GetRadarHalfExtents(sizeX, sizeY, (float)RadarConfig::GetBorderThickness(), m_bRadarShapeCircle, halfX, halfY);
        const CameraController::CameraState& camState = m_pCameraController->GetState();
        float northAngle = -camState.yaw - D3DX_PI * 0.5f;
        float northCenterX, northCenterY;
        RadarGeometry::PointOnOrbitEdge(centerX, centerY, halfX, halfY, cosf(northAngle), sinf(northAngle), !m_bRadarShapeCircle, northCenterX, northCenterY);
        m_pDraw->dxDrawImage2D(northCenterX - northMarkerSize * 0.5f, northCenterY - northMarkerSize * 0.5f, northMarkerSize, northMarkerSize, m_pNorthTexture, tocolor(255, 255, 255, 255));
    }

    RenderBlips2D();
    RenderAirstrips();
    RenderIndicatorBlips();
    RenderLegends();

    // Отрисовка текста радио
    RenderRadioText();

#ifdef _DEBUG
    dxDrawRenderDebugMemory();
#endif
}

void RadarRenderer::Render()
{
    if (!m_bInitialized || !m_pd3dDevice)
        return;
    
    bool inInterior = GameState::IsPlayerInInterior();
    if (m_bWasInInterior && !inInterior)
        m_exitInteriorImageStartTime = CTimer::m_snTimeInMilliseconds;
    m_bWasInInterior = inInterior;
    if (inInterior)
        return;
    

    HRESULT hr = m_pd3dDevice->TestCooperativeLevel();
    if (FAILED(hr) && hr != D3DERR_DEVICENOTRESET)
        return;
    
    if (!RwD3D9GetCurrentD3DDevice())
        return;
    
    if (RsGlobal.maximumWidth <= 0 || RsGlobal.maximumHeight <= 0)
        return;
#ifdef _DEBUG
    m_debugChunksRenderedLastFrame = 0;
#endif
    // Save all device states so as not to affect the game
    IDirect3DStateBlock9* pStateBlock = nullptr;
    if (m_pd3dDevice)
    {
        if (FAILED(m_pd3dDevice->CreateStateBlock(D3DSBT_ALL, &pStateBlock)))
        {
            pStateBlock = nullptr;
        }
        else if (pStateBlock)
        {
            pStateBlock->Capture();
        }
    }

    // Disable scissor test and z-testing so fire effects don't clip the radar
    // Radar must render above all scene objects
    DWORD oldScissorTest = 0;
    DWORD oldZEnable = 0;
    DWORD oldZWriteEnable = 0;
    if (m_pd3dDevice)
    {
        m_pd3dDevice->GetRenderState(D3DRS_SCISSORTESTENABLE, &oldScissorTest);
        m_pd3dDevice->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
        m_pd3dDevice->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWriteEnable);
        m_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    }

    try {
        m_cachedPlayer = FindPlayerPed();
        
        if (m_pCameraController)
        {
            m_pCameraController->UpdateFromGame(m_cachedPlayer);
            m_pCameraController->UpdateFromSpeed(m_cachedPlayer);
            
            m_cachedIsInAircraft = m_pCameraController->IsInAircraft(m_cachedPlayer);
            m_cachedIsInPlane = m_pCameraController->IsInPlane(m_cachedPlayer);

            if (m_cachedIsInAircraft)
            {
                m_cachedRollAngle = m_pCameraController->GetVehicleRollAngle(m_cachedPlayer);
                m_cachedPitchAngle = m_pCameraController->GetVehiclePitchAngle(m_cachedPlayer);
            }
        }

    float circleX, circleY, sizeX, sizeY;
    CalculateRadarPosition(circleX, circleY, sizeX, sizeY);

    float baseSize = (sizeX < sizeY) ? sizeX : sizeY;
    int rtSize = (int)(baseSize + 0.5f);
    if (rtSize < 1) rtSize = 1;
    if (m_pRenderTarget && (m_pRenderTarget->GetWidth() != rtSize || m_pRenderTarget->GetHeight() != rtSize))
        m_pRenderTarget->dxCreateRenderTarget(rtSize, rtSize);

    UpdateDrawResources();

    if (m_pRenderTarget && m_pRenderTarget->GetSurface())
    {
        m_pRenderTarget->dxSetRenderTarget(m_pRenderTarget->GetSurface());
        RenderRadarTargetContents(circleX, circleY, sizeX, sizeY);
        m_pRenderTarget->dxSetRenderTarget(nullptr);

        D3DVIEWPORT9 screenViewport;
        screenViewport.X = 0;
        screenViewport.Y = 0;
        screenViewport.Width = RsGlobal.maximumWidth;
        screenViewport.Height = RsGlobal.maximumHeight;
        screenViewport.MinZ = 0.0f;
        screenViewport.MaxZ = 1.0f;
        m_pd3dDevice->SetViewport(&screenViewport);
    }

    RenderRadarOverlays(circleX, circleY, sizeX, sizeY);
    }
    catch (...) {
    }

    // Restore scissor test and z-buffer states
    if (m_pd3dDevice)
    {
        m_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, oldScissorTest);
        m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, oldZEnable);
        m_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, oldZWriteEnable);
    }

    // Restore device states if they were saved
    if (pStateBlock)
    {
        pStateBlock->Apply();
        pStateBlock->Release();
        pStateBlock = nullptr;
    }
}

void RadarRenderer::dxDrawCircleShader(float x, float y, float width, float height, LPDIRECT3DTEXTURE9 texture, DWORD color, float alpha)
{
    if (m_pDraw) m_pDraw->dxDrawCircleShader(x, y, width, height, texture, color, alpha);
}

void RadarRenderer::dxDrawBorderShader(float x, float y, float width, float height, DWORD color, float alpha, float borderThicknessPixels)
{
    if (m_pDraw) m_pDraw->dxDrawBorderShader(x, y, width, height, color, alpha, borderThicknessPixels);
}

void RadarRenderer::dxDrawGreenSquareFill(float x, float y, float width, float height, LPDIRECT3DTEXTURE9 texture, DWORD color, float fillLevel)
{
    if (m_pDraw) m_pDraw->dxDrawGreenSquareFill(x, y, width, height, texture, color, fillLevel);
}

void RadarRenderer::dxDrawImage2D(float x, float y, float width, float height, LPDIRECT3DTEXTURE9 texture, DWORD color)
{
    if (m_pDraw) m_pDraw->dxDrawImage2D(x, y, width, height, texture, color);
}

void RadarRenderer::dxDrawImage2DRotated(float x, float y, float width, float height, LPDIRECT3DTEXTURE9 texture, float rotationAngle, DWORD color)
{
    if (m_pDraw) m_pDraw->dxDrawImage2DRotated(x, y, width, height, texture, rotationAngle, color);
}

void RadarRenderer::dxDrawRectangle(float x, float y, float width, float height, DWORD color)
{
    if (m_pDraw) m_pDraw->dxDrawRectangle(x, y, width, height, color);
}

void RadarRenderer::dxDrawGTAIndicatorBlip(float screenX, float screenY, float size, DWORD color, eHeightIndicatorType type)
{
    if (m_pDraw) m_pDraw->dxDrawGTAIndicatorBlip(screenX, screenY, size, color, type);
}

void RadarRenderer::dxDrawImage3D(const D3DXVECTOR3& elementPos, const D3DXVECTOR3& elementRot, const D3DXVECTOR2& elementSize,
                                 const D3DXVECTOR3& cameraPos, const D3DXVECTOR3& cameraRot,
                                 float fov, float nearPlane, float farPlane,
                                 LPDIRECT3DTEXTURE9 texture, DWORD color)
{
    if (m_pDraw) m_pDraw->dxDrawImage3D(elementPos, elementRot, elementSize, cameraPos, cameraRot, fov, nearPlane, farPlane, texture, color);
}

void RadarRenderer::dxDrawText(const char* text, float x, float y, float sx, float sy, float rotation, DWORD color)
{
    if (m_pDraw) m_pDraw->dxDrawText(text, x, y, sx, sy, rotation, color);
}

#ifdef _DEBUG
void RadarRenderer::dxDrawRenderDebugMemory()
{
    if (!m_pd3dDevice || !m_pFont)
        return;
    static const char s_targetModuleName[] = "radar-trilogy-sa.asi";
    DWORD cbNeeded = 0;
    EnumProcessModules(GetCurrentProcess(), nullptr, 0, &cbNeeded);
    if (cbNeeded == 0)
        return;
    std::vector<HMODULE> mods((cbNeeded + sizeof(HMODULE) - 1) / sizeof(HMODULE), nullptr);
    if (!EnumProcessModules(GetCurrentProcess(), mods.data(), (DWORD)(mods.size() * sizeof(HMODULE)), &cbNeeded))
        return;
    HMODULE hOurModule = nullptr;
    const DWORD nMods = cbNeeded / sizeof(HMODULE);
    for (DWORD i = 0; i < nMods; ++i)
    {
        char baseName[MAX_PATH] = {};
        if (GetModuleBaseNameA(GetCurrentProcess(), mods[i], baseName, _countof(baseName)) == 0)
            continue;
        if (_stricmp(baseName, s_targetModuleName) == 0)
        {
            hOurModule = mods[i];
            break;
        }
    }
    SIZE_T sizeMb = 0, sizeKb = 0;
    if (hOurModule)
    {
        MODULEINFO mi = {};
        if (GetModuleInformation(GetCurrentProcess(), hOurModule, &mi, sizeof(mi)))
        {
            sizeMb = mi.SizeOfImage / (1024 * 1024);
            sizeKb = (mi.SizeOfImage % (1024 * 1024)) / 1024;
        }
    }
    PROCESS_MEMORY_COUNTERS pmc = {};
    pmc.cb = sizeof(pmc);
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    const SIZE_T processMb = pmc.WorkingSetSize / (1024 * 1024);

    auto drawWithOutline = [this](const char* text, float x, float y, float sx, float sy, DWORD color) {
        const DWORD black = D3DCOLOR_XRGB(0, 0, 0);
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                if (dx != 0 || dy != 0)
                    m_pDraw->dxDrawText(text, x + (float)dx, y + (float)dy, sx, sy, 0.0f, black);
        m_pDraw->dxDrawText(text, x, y, sx, sy, 0.0f, color);
    };

    const DWORD color = tocolor(255, 255, 255, 255);
    char buf[160];
    float lineY = 10.0f;
    sprintf_s(buf, "DLL (%s): %u \xCC\xC1 %u \xCA\xC1", s_targetModuleName, (unsigned)sizeMb, (unsigned)sizeKb);
    drawWithOutline(buf, 10.0f, lineY, 420.0f, 22.0f, color);
    lineY += 24.0f;
    sprintf_s(buf, "\xCF\xF0\xEE\xF6\xE5\xF1\xF1: %u \xCC\xC1", (unsigned)processMb);
    drawWithOutline(buf, 10.0f, lineY, 420.0f, 22.0f, color);
    lineY += 24.0f;

    int chunksRendered = 0;
    int chunksLoaded = 0;
    int chunksUnloaded = MapChunkManager::MAP_CHUNKS_COUNT;
    size_t blipsTotal = 0;
    size_t blipsEnabled = 0;
    if (m_pMapChunkManager)
    {
        chunksLoaded = m_pMapChunkManager->GetLoadedChunksCount();
        chunksUnloaded = MapChunkManager::MAP_CHUNKS_COUNT - chunksLoaded;
        chunksRendered = m_debugChunksRenderedLastFrame;
    }
    if (m_pBlipManager)
    {
        const auto& blips = m_pBlipManager->GetBlips();
        blipsTotal = blips.size();
        for (const auto& b : blips)
            if (b.enabled) ++blipsEnabled;
    }
    sprintf_s(buf, "Chunks: %d rend., %d load., %d unload. (of %d)", chunksRendered, chunksLoaded, chunksUnloaded, MapChunkManager::MAP_CHUNKS_COUNT);
    drawWithOutline(buf, 10.0f, lineY, 560.0f, 22.0f, color);
    lineY += 24.0f;
    sprintf_s(buf, "\xC1\xEB\xE8\xEF\xFB: %zu \xE2\xF1\xE5\xE3\xEE, %zu \xE2\xEA\xEB.", blipsTotal, blipsEnabled);
    drawWithOutline(buf, 10.0f, lineY, 420.0f, 22.0f, color);
}

#endif // _DEBUG

void RadarRenderer::RenderBlips()
{
    if (!m_pd3dDevice || !m_pDraw || !m_pBlipManager || !m_pCameraController)
        return;
    
    HRESULT hr = m_pd3dDevice->TestCooperativeLevel();
    if (FAILED(hr) && hr != D3DERR_DEVICENOTRESET)
        return;
    
    try {
        float offsetWorldX, offsetWorldY;
        D3DXVECTOR3 cameraPos, cameraRot;
        m_pCameraController->GetCachedCalculations(offsetWorldX, offsetWorldY, cameraPos, cameraRot);
        const CameraController::CameraState& camState = m_pCameraController->GetState();
        
        const auto& blips = m_pBlipManager->GetBlips();
        D3DXVECTOR2 elementSize(6000.0f, 6000.0f);
        
        // Render player icon in 3D (hidden in plane for round radar; square radar always shows it)
        LPDIRECT3DTEXTURE9 playerTexture = m_pBlipManager->GetBlipTexture(2);
        if (playerTexture && (!m_cachedIsInPlane || !m_bRadarShapeCircle))
        {
            D3DXVECTOR3 playerPos(camState.posX, camState.posY, 0.1f + (float)(blips.size() + 1) * 0.01f);
            
            float playerRotation = 0.0f;
            if (m_cachedPlayer)
            {
                playerRotation = m_cachedPlayer->GetHeading();
            }
            
            D3DXVECTOR3 playerRot(0.0f, 0.0f, playerRotation);

            // Player icon size: default at height 445, grows with camera height
            const float defaultCameraHeight = 445.0f;
            const float baseSize = 17.0f;
            float heightMult = camState.height / defaultCameraHeight;
            float scaledSize = baseSize * heightMult;

            D3DXVECTOR2 playerSize(scaledSize, scaledSize);
            
            m_pDraw->dxDrawImage3D(playerPos, playerRot, playerSize, cameraPos, cameraRot,
                       camState.fov, m_nearPlane, m_farPlane,
                       playerTexture, tocolor(255, 255, 255, 255));
        }
        
        // Other blips are rendered in 2D only (not in 3D)
    }
    catch (...) {
    }
}

void RadarRenderer::RenderBlips2D()
{
    
    if (!m_pd3dDevice)
        return;
    
    HRESULT hr = m_pd3dDevice->TestCooperativeLevel();
    if (FAILED(hr) && hr != D3DERR_DEVICENOTRESET)
        return;
    
    try {

    float circleX, circleY, sizeX, sizeY;
    CalculateRadarPosition(circleX, circleY, sizeX, sizeY);
    float centerX = circleX + sizeX * 0.5f;
    float centerY = circleY + sizeY * 0.5f;
    float halfX, halfY;
    RadarGeometry::GetRadarHalfExtents(sizeX, sizeY, (float)RadarConfig::GetBorderThickness(), m_bRadarShapeCircle, halfX, halfY);
    if (!m_pBlipManager || !m_pCameraController)
        return;
    float offsetWorldX, offsetWorldY;
    D3DXVECTOR3 cameraPos, cameraRot;
    m_pCameraController->GetCachedCalculations(offsetWorldX, offsetWorldY, cameraPos, cameraRot);
    const CameraController::CameraState& camState = m_pCameraController->GetState();
    float rtWidth = m_pRenderTarget ? (float)m_pRenderTarget->GetWidth() : 0.0f;
    float rtHeight = m_pRenderTarget ? (float)m_pRenderTarget->GetHeight() : 0.0f;
    float screenAspect = (m_width > 0) ? ((float)m_height / (float)m_width) : 1.0f;

    // When mission marker is active (checkpoint indicator), don't show waypoint (41) вЂ” same as 2D-RADAR hideLegendsFromOrbit logic
    bool hasMissionCheckpoint = false;
    if (CRadar::ms_RadarTrace)
    {
        for (unsigned int t = 0; t < MAX_RADAR_TRACES; t++)
        {
            const tRadarTrace& tr = CRadar::ms_RadarTrace[t];
            if (!tr.m_bInUse) continue;
            bool isMissionCheckpoint = (tr.m_nBlipType == BLIP_COORD || tr.m_nBlipType == BLIP_CONTACTPOINT) && BlipManager::IsMissionCheckpointSprite(tr.m_nRadarSprite);
            if (isMissionCheckpoint) { hasMissionCheckpoint = true; break; }
        }
    }

    float radarRange = CRadar::m_radarRange;
    D3DXVECTOR3 playerPos(camState.posX, camState.posY, 0.0f);

    const std::vector<Blip>& blips = m_pBlipManager->GetBlips();
    for (size_t i = 0; i < blips.size(); ++i)
    {
        if (!blips[i].enabled)
            continue;

        if (blips[i].shortRange && !RadarGeometry::IsWithinRadarRange(blips[i].position, playerPos, radarRange))
            continue;
        
        int iconId = blips[i].iconId;
        LPDIRECT3DTEXTURE9 blipTexture = m_pBlipManager->GetBlipTexture(iconId);
        bool validIconRange = (iconId >= 0 && iconId <= BlipManager::MAX_BLIP_ID)
            || (iconId >= BlipManager::MORE_ICON_STORE && iconId <= BlipManager::MORE_ICON_TRAIN);
        if (!validIconRange || !blipTexture)
            continue;
        
        // Skip icon ID 2 (player icon) in 2D rendering - it's rendered in 3D
        if (iconId == 2)
            continue;

        // Don't show waypoint (41) when mission marker is active вЂ” like 2D-RADAR
        if (iconId == RADAR_SPRITE_WAYPOINT && hasMissionCheckpoint)
            continue;
        
        D3DXVECTOR3 blipWorldPos = blips[i].position;
        blipWorldPos.z += 0.1f;
        
        bool isVisible = false;
        
        float circleScreenX, circleScreenY;
        bool useSquareOrbit = !m_bRadarShapeCircle;
        if (RadarGeometry::WorldToCircleScreen(blipWorldPos, cameraPos, cameraRot, camState.fov, m_nearPlane, m_farPlane, rtWidth, rtHeight, sizeX, sizeY, centerX, centerY, circleScreenX, circleScreenY, screenAspect))
        {
            if (RadarGeometry::IsInsideOrbit(circleScreenX, circleScreenY, centerX, centerY, halfX, halfY, useSquareOrbit))
            {
                isVisible = true;
                float iconSize = CalculateBlipSize(24.0f);
                
                float iconX = circleScreenX - iconSize * 0.5f;
                float iconY = circleScreenY - iconSize * 0.5f;
                m_pDraw->dxDrawImage2D(iconX, iconY, iconSize, iconSize, blipTexture, tocolor(255, 255, 255, 255));
            }
        }
        
        // Waypoint should ALWAYS be visible on edge, even if behind camera or very far
        if (iconId == 41 && !isVisible)
        {
            float angle = 0.0f;
            bool angleCalculated = false;
            
            float screenX, screenY;
            if (MathUtils::WorldToScreen(blipWorldPos, cameraPos, cameraRot, camState.fov, m_nearPlane, m_farPlane,
                rtWidth, rtHeight, screenX, screenY, screenAspect))
            {
                // Convert from render target coordinates to normalized coordinates
                float normalizedX = screenX / rtWidth; // 0 to 1
                float normalizedY = screenY / rtHeight; // 0 to 1
                
                // Calculate direction from center of radar to waypoint in screen space
                float dirX = normalizedX - 0.5f; // -0.5 to 0.5
                float dirY = normalizedY - 0.5f; // -0.5 to 0.5
                
                // Normalize direction
                float dirLength = sqrtf(dirX * dirX + dirY * dirY);
                if (dirLength > 0.01f)
                {
                    dirX /= dirLength;
                    dirY /= dirLength;
                    
                    // Calculate angle from center to waypoint in screen space
                    angle = atan2f(dirY, dirX);
                    angleCalculated = true;
                }
            }
            
            if (!angleCalculated)
            {
                D3DXVECTOR3 waypointPos2D(blipWorldPos.x, blipWorldPos.y, 0.0f);
                angleCalculated = MathUtils::DirectionToOrbitAngle(playerPos, waypointPos2D, camState.yaw, angle);
            }
            
            if (angleCalculated)
            {
                float iconSize = CalculateBlipSize(24.0f);
                float edgeCenterX, edgeCenterY;
                RadarGeometry::PointOnOrbitEdge(centerX, centerY, halfX, halfY, cosf(angle), sinf(angle), useSquareOrbit, edgeCenterX, edgeCenterY);
                float edgeX = edgeCenterX - iconSize * 0.5f;
                float edgeY = edgeCenterY - iconSize * 0.5f;
                m_pDraw->dxDrawImage2D(edgeX, edgeY, iconSize, iconSize, blipTexture, tocolor(255, 255, 255, 255));
            }
        }
    }
    }
    catch (...) {
        // Ignore exceptions when rendering 2D blips
    }
}

// --- Airstrip (airport) indicator: 56 LIGHT / 57 RUNWAY (from 2D-RADAR) ---
struct AirstripInfo {
    float posX, posY;
    float direction; // degrees
    float radius;
};
static const AirstripInfo g_AirstripTable[] = {
    { +1750.0f,    -2494.0f,   180.0f,   1000.0f*  .75f },  // LS Airport
    {-1373.0f ,    +120.0f ,   225.0f,   1500.0f * .75f}, // SF Airport
    {+1478.0f ,    +1461.0f,   90.0f ,   1200.0f * .75f}, // LV Airport
    {+175.0f  ,    +2502.0f,   180.0f,   1000.0f * .75f}  // Verdant Meadows
};
static const int NUM_AIRSTRIPS = sizeof(g_AirstripTable) / sizeof(g_AirstripTable[0]);

static int GetAirstripIndexByPosition(float worldX, float worldY)
{
    float minDistSq = FLT_MAX;
    int index = -1;
    for (int i = 0; i < NUM_AIRSTRIPS; i++)
    {
        float distSq = MathUtils::DistanceSq2D(worldX, worldY, g_AirstripTable[i].posX, g_AirstripTable[i].posY);
        if (distSq < minDistSq) { minDistSq = distSq; index = i; }
    }
    return index;
}

static float GetAirstripDirectionByPosition(float worldX, float worldY)
{
    int i = GetAirstripIndexByPosition(worldX, worldY);
    return i >= 0 ? g_AirstripTable[i].direction : 0.0f;
}

static bool IsPlayerOnRunwaySegment(float point1X, float point1Y, float point2X, float point2Y, float playerX, float playerY, float runwayWidth)
{
    float vx = point2X - point1X, vy = point2Y - point1Y;
    float lenSq = vx * vx + vy * vy;
    if (lenSq < 1e-6f) return false;
    float wx = playerX - point1X, wy = playerY - point1Y;
    float t = (wx * vx + wy * vy) / lenSq;
    if (t < 0.0f || t > 1.0f) return false;
    float projX = point1X + t * vx, projY = point1Y + t * vy;
    return MathUtils::DistanceSq2D(playerX, playerY, projX, projY) <= runwayWidth * runwayWidth;
}

// World (x,y) -> circle-space offset from center. Returns true if WorldToScreen succeeded.
static bool WorldToCircleOffset(float worldX, float worldY, float worldZ,
    const D3DXVECTOR3& cameraPos, const D3DXVECTOR3& cameraRot, float fov, float nearPlane, float farPlane,
    float rtWidth, float rtHeight, float sizeX, float sizeY, float centerX, float centerY,
    float& outCircleX, float& outCircleY, float projectionAspect = 0.0f)
{
    D3DXVECTOR3 worldPos(worldX + RadarGeometry::RADAR_OFFSET_X, worldY + RadarGeometry::RADAR_OFFSET_Y, 0.1f);
    float screenX, screenY;
    if (!MathUtils::WorldToScreen(worldPos, cameraPos, cameraRot, fov, nearPlane, farPlane, rtWidth, rtHeight, screenX, screenY, projectionAspect))
        return false;
    float normalizedX = screenX / rtWidth, normalizedY = screenY / rtHeight;
    outCircleX = (normalizedX - 0.5f) * sizeX;
    outCircleY = (normalizedY - 0.5f) * sizeY;
    return true;
}

static bool GetAirstripOffsetRangeInsideCircle(float stripCenterX, float stripCenterY, float stripDirRad, float halfLen, float playerZ,
    float centerX, float centerY, float innerRadius,
    const D3DXVECTOR3& cameraPos, const D3DXVECTOR3& cameraRot, float fov, float nearPlane, float farPlane,
    float rtWidth, float rtHeight, float sizeX, float sizeY,
    float& outMinOffset, float& outMaxOffset, float projectionAspect = 0.0f)
{
    const float stepWorld = 100.0f;
    float cosDir = cosf(stripDirRad), sinDir = sinf(stripDirRad);
    float cx, cy, sx, sy;
    if (!WorldToCircleOffset(stripCenterX, stripCenterY, playerZ, cameraPos, cameraRot, fov, nearPlane, farPlane, rtWidth, rtHeight, sizeX, sizeY, centerX, centerY, cx, cy, projectionAspect))
        return false;
    if (!WorldToCircleOffset(stripCenterX + stepWorld * cosDir, stripCenterY + stepWorld * sinDir, playerZ, cameraPos, cameraRot, fov, nearPlane, farPlane, rtWidth, rtHeight, sizeX, sizeY, centerX, centerY, sx, sy, projectionAspect))
        return false;
    float Dx = (sx - cx) / stepWorld, Dy = (sy - cy) / stepWorld;
    float a = Dx * Dx + Dy * Dy;
    if (a < 1e-10f) return false;
    float b = 2.0f * (cx * Dx + cy * Dy);
    float c = cx * cx + cy * cy - innerRadius * innerRadius;
    float disc = b * b - 4.0f * a * c;
    if (disc < 0.0f)
    {
        if (c <= 0.0f) { outMinOffset = -halfLen; outMaxOffset = halfLen; return true; }
        return false;
    }
    float sqrtDisc = sqrtf(disc);
    float w1 = (-b - sqrtDisc) / (2.0f * a), w2 = (-b + sqrtDisc) / (2.0f * a);
    outMinOffset = (w1 < w2) ? w1 : w2;
    outMaxOffset = (w1 < w2) ? w2 : w1;
    if (outMinOffset < -halfLen) outMinOffset = -halfLen;
    if (outMaxOffset > halfLen) outMaxOffset = halfLen;
    if (outMinOffset > outMaxOffset) return false;
    return true;
}

// Helper: get world position from radar trace (entity or m_vecPos)
static void GetLegendTraceWorldPosition(const tRadarTrace& trace, float& outX, float& outY, float& outZ)
{
    if (trace.m_nBlipType == BLIP_CHAR && trace.m_nEntityHandle != 0)
    {
        CPed* p = CPools::GetPed(trace.m_nEntityHandle);
        if (p)
        {
            CVector pos = p->GetPosition();
            outX = pos.x; outY = pos.y; outZ = pos.z;
            return;
        }
    }
    if (trace.m_nBlipType == BLIP_CAR && trace.m_nEntityHandle != 0)
    {
        CVehicle* v = CPools::GetVehicle(trace.m_nEntityHandle);
        if (v)
        {
            CVector pos = v->GetPosition();
            outX = pos.x; outY = pos.y; outZ = pos.z;
            return;
        }
    }
    outX = trace.m_vecPos.x;
    outY = trace.m_vecPos.y;
    outZ = trace.m_vecPos.z;
}

void RadarRenderer::RenderAirstrips()
{
    if (!m_pd3dDevice || !m_pBlipManager || !m_pCameraController)
        return;
    HRESULT hr = m_pd3dDevice->TestCooperativeLevel();
    if (FAILED(hr) && hr != D3DERR_DEVICENOTRESET)
        return;

    float circleX, circleY, sizeX, sizeY;
    CalculateRadarPosition(circleX, circleY, sizeX, sizeY);
    float centerX = circleX + sizeX * 0.5f;
    float centerY = circleY + sizeY * 0.5f;
    float halfX, halfY;
    RadarGeometry::GetRadarHalfExtents(sizeX, sizeY, (float)RadarConfig::GetBorderThickness(), m_bRadarShapeCircle, halfX, halfY);
    float iconSize = CalculateBlipSize(24.0f);

    float offsetWorldX, offsetWorldY;
    D3DXVECTOR3 cameraPos, cameraRot;
    m_pCameraController->GetCachedCalculations(offsetWorldX, offsetWorldY, cameraPos, cameraRot);
    const CameraController::CameraState& camState = m_pCameraController->GetState();
    float rtWidth = m_pRenderTarget ? (float)m_pRenderTarget->GetWidth() : 0.0f;
    float rtHeight = m_pRenderTarget ? (float)m_pRenderTarget->GetHeight() : 0.0f;
    float screenAspect = (m_width > 0) ? ((float)m_height / (float)m_width) : 1.0f;

    float playerX = 0.0f, playerY = 0.0f, playerZ = 0.0f;
    if (m_cachedPlayer)
    {
        CVector ppos = m_cachedPlayer->GetPosition();
        playerX = ppos.x; playerY = ppos.y; playerZ = ppos.z;
    }

    // Show airstrip only when player is in plane (not helicopter) - avionics for planes only
    if (!m_cachedIsInPlane)
        return;

    int nearestIdx = 0;
    float minDist = FLT_MAX;
    D3DXVECTOR3 playerPos2D(playerX, playerY, 0.0f);
    for (int i = 0; i < NUM_AIRSTRIPS; i++)
    {
        D3DXVECTOR3 stripPos(g_AirstripTable[i].posX, g_AirstripTable[i].posY, 0.0f);
        float dist = MathUtils::CalculateDistance2D(playerPos2D, stripPos);
        if (dist < minDist)
        {
            minDist = dist;
            nearestIdx = i;
        }
    }

    {
        int airstripIdx = nearestIdx;
        const AirstripInfo& strip = g_AirstripTable[airstripIdx];
        float wx = strip.posX, wy = strip.posY, wz = playerZ;
        bool playerOnRunwaySegment = false;
        {
            float halfLen = strip.radius * 0.5f;
            float dirRad = strip.direction * (3.14159265f / 180.0f);
            float cosD = cosf(dirRad), sinD = sinf(dirRad);
            float point1X = strip.posX + halfLen * cosD, point1Y = strip.posY + halfLen * sinD;
            float point2X = strip.posX - halfLen * cosD, point2Y = strip.posY - halfLen * sinD;
            float runwayWidth = strip.radius * 0.25f;
            playerOnRunwaySegment = IsPlayerOnRunwaySegment(point1X, point1Y, point2X, point2Y, playerX, playerY, runwayWidth);
        }

        float airstripCenterCircleX, airstripCenterCircleY;
        bool airstripCenterVisible = WorldToCircleOffset(strip.posX, strip.posY, playerZ, cameraPos, cameraRot, camState.fov, m_nearPlane, m_farPlane, rtWidth, rtHeight, sizeX, sizeY, centerX, centerY, airstripCenterCircleX, airstripCenterCircleY, screenAspect);
        bool show56Light = airstripCenterVisible || playerOnRunwaySegment;
        bool isOnOrbit = !show56Light;

        D3DXVECTOR3 blipWorldPos(wx + RadarGeometry::RADAR_OFFSET_X, wy + RadarGeometry::RADAR_OFFSET_Y, 0.1f);
        float screenX, screenY;
        bool visible = MathUtils::WorldToScreen(blipWorldPos, cameraPos, cameraRot, camState.fov, m_nearPlane, m_farPlane, rtWidth, rtHeight, screenX, screenY, screenAspect);
        float circleScreenX, circleScreenY;
        float dx, dy;
        if (visible)
        {
            float normalizedX = screenX / rtWidth, normalizedY = screenY / rtHeight;
            circleScreenX = centerX + (normalizedX - 0.5f) * sizeX;
            circleScreenY = centerY + (normalizedY - 0.5f) * sizeY;
            dx = circleScreenX - centerX;
            dy = circleScreenY - centerY;
        }
        else
        {
            D3DXVECTOR3 playerPos(camState.posX, camState.posY, 0.0f);
            D3DXVECTOR3 airstripPos2D(wx + RadarGeometry::RADAR_OFFSET_X, wy + RadarGeometry::RADAR_OFFSET_Y, 0.0f);
            float angle;
            if (!MathUtils::DirectionToOrbitAngle(playerPos, airstripPos2D, camState.yaw, angle))
                return;
            dx = cosf(angle);
            dy = sinf(angle);
            bool useSquareOrbit = !m_bRadarShapeCircle;
            RadarGeometry::PointOnOrbitEdge(centerX, centerY, halfX, halfY, dx, dy, useSquareOrbit, circleScreenX, circleScreenY);
        }

        unsigned char airstripSprite = isOnOrbit ? RADAR_SPRITE_RUNWAY : RADAR_SPRITE_LIGHT;
        LPDIRECT3DTEXTURE9 iconTex = m_pBlipManager->GetBlipTexture(airstripSprite);
        if (!iconTex)
            return;

        float runwayDirectionDeg = GetAirstripDirectionByPosition(wx, wy);
        float runwayDirectionRad = runwayDirectionDeg * (3.14159265f / 180.0f);
        float rotation = -camState.yaw - runwayDirectionRad - (3.14159265f * 0.5f);

        if (isOnOrbit)
        {
            // 57 RUNWAY: position = airport coordinates (projected when visible, orbit when behind). No animation from 56.
            if (!visible)
            {
                float orbitInset = iconSize * 0.5f;
                float insetHalfX = (halfX > orbitInset) ? (halfX - orbitInset) : halfX;
                float insetHalfY = (halfY > orbitInset) ? (halfY - orbitInset) : halfY;
                float orbitAngle = atan2f(dy, dx);
                bool useSquareOrbit = !m_bRadarShapeCircle;
                RadarGeometry::PointOnOrbitEdge(centerX, centerY, insetHalfX, insetHalfY, cosf(orbitAngle), sinf(orbitAngle), useSquareOrbit, circleScreenX, circleScreenY);
            }
            float iconX = circleScreenX - iconSize * 0.5f;
            float iconY = circleScreenY - iconSize * 0.5f;
            m_pDraw->dxDrawImage2DRotated(iconX, iconY, iconSize, iconSize, iconTex, rotation, tocolor(255, 255, 255, 255));
        }
        else if (airstripIdx >= 0)
        {
            const AirstripInfo& stripInner = g_AirstripTable[airstripIdx];
            float halfLen = stripInner.radius * 0.5f;
            float rangeRadius = (halfX < halfY) ? halfX : halfY;
            float minOffset, maxOffset;
            bool hasRange = GetAirstripOffsetRangeInsideCircle(stripInner.posX, stripInner.posY, runwayDirectionRad, halfLen, playerZ,
                centerX, centerY, rangeRadius, cameraPos, cameraRot, camState.fov, m_nearPlane, m_farPlane, rtWidth, rtHeight, sizeX, sizeY, minOffset, maxOffset, screenAspect);
            float offset;
            if (!hasRange || maxOffset <= minOffset)
                offset = 0.0f;
            else
            {
                float range = maxOffset - minOffset;
                float animLen = range * 0.75f;
                float centerOffset = (minOffset + maxOffset) * 0.5f;
                float newMin = centerOffset - animLen * 0.5f;
                float newMax = centerOffset + animLen * 0.5f;
                unsigned int timeMs = CTimer::m_snTimeInMilliseconds;
                const float cycleMs = 350.0f;
                float t = static_cast<float>(timeMs) / cycleMs;
                float tSmooth = 0.5f + 0.5f * sinf(t * 6.283185307f);
                offset = newMin + (newMax - newMin) * tSmooth;
            }
            bool useSquareOrbit = !m_bRadarShapeCircle;

            float cosDir = cosf(runwayDirectionRad), sinDir = sinf(runwayDirectionRad);
            float animX = stripInner.posX + offset * cosDir, animY = stripInner.posY + offset * sinDir;
            float animCircleX, animCircleY;
            bool lightPosOk = WorldToCircleOffset(animX, animY, playerZ, cameraPos, cameraRot, camState.fov, m_nearPlane, m_farPlane, rtWidth, rtHeight, sizeX, sizeY, centerX, centerY, animCircleX, animCircleY, screenAspect);

            // 56 LIGHT: always show when on runway, never on orbit. Keep icon inside radar with margin.
            LPDIRECT3DTEXTURE9 lightTex = m_pBlipManager->GetBlipTexture(RADAR_SPRITE_LIGHT);
            if (lightTex)
            {
                if (airstripCenterVisible && lightPosOk)
                {
                    circleScreenX = centerX + animCircleX;
                    circleScreenY = centerY + animCircleY;
                }
                else if (airstripCenterVisible)
                {
                    circleScreenX = centerX + airstripCenterCircleX;
                    circleScreenY = centerY + airstripCenterCircleY;
                }
                else
                {
                    float orbitAngle = atan2f(dy, dx);
                    float innerRadius = ((halfX < halfY) ? halfX : halfY) - iconSize;
                    if (innerRadius < 10.0f) innerRadius = 10.0f;
                    circleScreenX = centerX + cosf(orbitAngle) * innerRadius;
                    circleScreenY = centerY + sinf(orbitAngle) * innerRadius;
                }
                float iconMargin = iconSize * 0.6f;
                float innerHalfX = (halfX > iconMargin) ? (halfX - iconMargin) : halfX;
                float innerHalfY = (halfY > iconMargin) ? (halfY - iconMargin) : halfY;
                bool neededClamp = !RadarGeometry::IsInsideOrbit(circleScreenX, circleScreenY, centerX, centerY, innerHalfX, innerHalfY, useSquareOrbit);
                bool trajectoryVisible = hasRange && (maxOffset > minOffset);
                bool switchTo57 = neededClamp && !trajectoryVisible;  // only 56→57 when trajectory not visible
                if (neededClamp)
                {
                    if (switchTo57)
                    {
                        circleScreenX = centerX + airstripCenterCircleX;
                        circleScreenY = centerY + airstripCenterCircleY;
                        RadarGeometry::ClampToOrbit(circleScreenX, circleScreenY, centerX, centerY, innerHalfX, innerHalfY, circleScreenX, circleScreenY, useSquareOrbit);
                    }
                    else
                        RadarGeometry::ClampToOrbit(circleScreenX, circleScreenY, centerX, centerY, innerHalfX, innerHalfY, circleScreenX, circleScreenY, useSquareOrbit);
                }
                LPDIRECT3DTEXTURE9 iconToDraw = switchTo57 ? m_pBlipManager->GetBlipTexture(RADAR_SPRITE_RUNWAY) : lightTex;
                float iconX = circleScreenX - iconSize * 0.5f;
                float iconY = circleScreenY - iconSize * 0.5f;
                m_pDraw->dxDrawImage2DRotated(iconX, iconY, iconSize, iconSize, iconToDraw, neededClamp ? rotation : 0.0f, tocolor(255, 255, 255, 255));
            }
        }
    }
}

void RadarRenderer::RenderIndicatorBlips()
{
    if (!m_pd3dDevice || !m_pBlipManager || !m_pCameraController)
        return;
    HRESULT hr = m_pd3dDevice->TestCooperativeLevel();
    if (FAILED(hr) && hr != D3DERR_DEVICENOTRESET)
        return;
    if (!CRadar::ms_RadarTrace)
        return;

    float circleX, circleY, sizeX, sizeY;
    CalculateRadarPosition(circleX, circleY, sizeX, sizeY);
    float centerX = circleX + sizeX * 0.5f;
    float centerY = circleY + sizeY * 0.5f;
    float halfX, halfY;
    RadarGeometry::GetRadarHalfExtents(sizeX, sizeY, (float)RadarConfig::GetBorderThickness(), m_bRadarShapeCircle, halfX, halfY);
    float indicatorSize = CalculateBlipSize(13.0f);
    bool useSquareOrbit = !m_bRadarShapeCircle;

    float offsetWorldX, offsetWorldY;
    D3DXVECTOR3 cameraPos, cameraRot;
    m_pCameraController->GetCachedCalculations(offsetWorldX, offsetWorldY, cameraPos, cameraRot);
    const CameraController::CameraState& camState = m_pCameraController->GetState();
    float rtWidth = m_pRenderTarget ? (float)m_pRenderTarget->GetWidth() : 0.0f;
    float rtHeight = m_pRenderTarget ? (float)m_pRenderTarget->GetHeight() : 0.0f;
    float screenAspect = (m_width > 0) ? ((float)m_height / (float)m_width) : 1.0f;

    float playerZ = 0.0f;
    if (m_cachedPlayer)
    {
        CVector ppos = m_cachedPlayer->GetPosition();
        playerZ = ppos.z;
    }

    for (unsigned int i = 0; i < MAX_RADAR_TRACES; i++)
    {
        tRadarTrace& trace = CRadar::ms_RadarTrace[i];
        if (!trace.m_bInUse)
            continue;
        if (trace.m_nBlipDisplay == BLIP_DISPLAY_NEITHER)
            continue;

        unsigned char blipType = trace.m_nBlipType;
        unsigned char spriteId = trace.m_nRadarSprite;
        bool isMissionCheckpoint = (blipType == BLIP_COORD || blipType == BLIP_CONTACTPOINT) && BlipManager::IsMissionCheckpointSprite(spriteId);
        bool needsIndicator = (blipType == BLIP_CHAR || blipType == BLIP_CAR || blipType == BLIP_SPOTLIGHT || isMissionCheckpoint);
        if (!needsIndicator)
            continue;

        float wx, wy, wz;
        GetLegendTraceWorldPosition(trace, wx, wy, wz);
        D3DXVECTOR3 blipWorldPos(wx + RadarGeometry::RADAR_OFFSET_X, wy + RadarGeometry::RADAR_OFFSET_Y, 0.1f);

        float screenX, screenY;
        if (!MathUtils::WorldToScreen(blipWorldPos, cameraPos, cameraRot, camState.fov, m_nearPlane, m_farPlane, rtWidth, rtHeight, screenX, screenY, screenAspect))
        {
            D3DXVECTOR3 playerPos(camState.posX, camState.posY, 0.0f);
            D3DXVECTOR3 blipPos2D(wx + RadarGeometry::RADAR_OFFSET_X, wy + RadarGeometry::RADAR_OFFSET_Y, 0.0f);
            float angle;
            if (!MathUtils::DirectionToOrbitAngle(playerPos, blipPos2D, camState.yaw, angle))
                continue;
            float iconX, iconY;
            RadarGeometry::PointOnOrbitEdge(centerX, centerY, halfX, halfY, cosf(angle), sinf(angle), useSquareOrbit, iconX, iconY);
            DWORD color = BlipManager::TraceColorToD3D(trace.m_nColour, trace.m_bBright != 0, trace.m_bFriendly != 0);
            m_pDraw->dxDrawGTAIndicatorBlip(iconX, iconY, indicatorSize, color, BlipManager::GetHeightIndicatorType(wz, playerZ, 2.5f));
            continue;
        }

        float normalizedX = screenX / rtWidth;
        float normalizedY = screenY / rtHeight;
        float circleScreenX = centerX + (normalizedX - 0.5f) * sizeX;
        float circleScreenY = centerY + (normalizedY - 0.5f) * sizeY;
        if (!RadarGeometry::IsInsideOrbit(circleScreenX, circleScreenY, centerX, centerY, halfX, halfY, useSquareOrbit))
            RadarGeometry::ClampToOrbit(circleScreenX, circleScreenY, centerX, centerY, halfX, halfY, circleScreenX, circleScreenY, useSquareOrbit);

        DWORD color = BlipManager::TraceColorToD3D(trace.m_nColour, trace.m_bBright != 0, trace.m_bFriendly != 0);
        m_pDraw->dxDrawGTAIndicatorBlip(circleScreenX, circleScreenY, indicatorSize, color, BlipManager::GetHeightIndicatorType(wz, playerZ, 2.5f));
    }

    // Enemy missiles/rockets (from 2D-RADAR): only show rockets not created by player or player vehicle
    extern unsigned int MAX_PROJECTILE_INFOS;
    if (CProjectileInfo::ms_apProjectile && gaProjectileInfo)
    {
        CPlayerPed* playerPed = FindPlayerPed();
        CVehicle* playerVeh = FindPlayerVehicle();
        CVector playerPosR = FindPlayerCoors();
        DWORD missileColor = tocolor(255, 0, 0, 255);
        float missileIndicatorSize = CalculateBlipSize(12.0f);

        for (unsigned int i = 0; i < MAX_PROJECTILE_INFOS; i++)
        {
            CProjectileInfo& info = gaProjectileInfo[i];
            CEntity* creator = info.m_pCreator;
            if (!info.m_bActive || !CProjectileInfo::ms_apProjectile[i])
                continue;
            if (creator == playerPed || creator == playerVeh)
                continue;
            unsigned int wtype = info.m_nWeaponType;
            if (wtype != WEAPONTYPE_ROCKET && wtype != WEAPONTYPE_ROCKET_HS && wtype != WEAPONTYPE_RLAUNCHER && wtype != WEAPONTYPE_RLAUNCHER_HS)
                continue;

            CVector worldPos = CProjectileInfo::ms_apProjectile[i]->GetPosition();
            D3DXVECTOR3 missileWorldPos(worldPos.x + RadarGeometry::RADAR_OFFSET_X, worldPos.y + RadarGeometry::RADAR_OFFSET_Y, 0.1f);

            float screenX, screenY;
            if (!MathUtils::WorldToScreen(missileWorldPos, cameraPos, cameraRot, camState.fov, m_nearPlane, m_farPlane, rtWidth, rtHeight, screenX, screenY, screenAspect))
            {
                D3DXVECTOR3 playerPos(camState.posX, camState.posY, 0.0f);
                D3DXVECTOR3 missilePos2D(worldPos.x + RadarGeometry::RADAR_OFFSET_X, worldPos.y + RadarGeometry::RADAR_OFFSET_Y, 0.0f);
                float angle;
                if (!MathUtils::DirectionToOrbitAngle(playerPos, missilePos2D, camState.yaw, angle))
                    continue;
                float iconX, iconY;
                RadarGeometry::PointOnOrbitEdge(centerX, centerY, halfX, halfY, cosf(angle), sinf(angle), useSquareOrbit, iconX, iconY);
                m_pDraw->dxDrawGTAIndicatorBlip(iconX, iconY, missileIndicatorSize, missileColor, BlipManager::GetHeightIndicatorType(worldPos.z, playerPosR.z, 2.5f));
                continue;
            }

            float normalizedX = screenX / rtWidth;
            float normalizedY = screenY / rtHeight;
            float circleScreenX = centerX + (normalizedX - 0.5f) * sizeX;
            float circleScreenY = centerY + (normalizedY - 0.5f) * sizeY;
            if (!RadarGeometry::IsInsideOrbit(circleScreenX, circleScreenY, centerX, centerY, halfX, halfY, useSquareOrbit))
                RadarGeometry::ClampToOrbit(circleScreenX, circleScreenY, centerX, centerY, halfX, halfY, circleScreenX, circleScreenY, useSquareOrbit);
            m_pDraw->dxDrawGTAIndicatorBlip(circleScreenX, circleScreenY, missileIndicatorSize, missileColor, BlipManager::GetHeightIndicatorType(worldPos.z, playerPosR.z, 2.5f));
        }
    }
}

void RadarRenderer::RenderLegends()
{
    if (!m_pd3dDevice || !m_pBlipManager || !m_pCameraController)
        return;

    HRESULT hr = m_pd3dDevice->TestCooperativeLevel();
    if (FAILED(hr) && hr != D3DERR_DEVICENOTRESET)
        return;

    if (!CRadar::ms_RadarTrace)
        return;

    float circleX, circleY, sizeX, sizeY;
    CalculateRadarPosition(circleX, circleY, sizeX, sizeY);
    float centerX = circleX + sizeX * 0.5f;
    float centerY = circleY + sizeY * 0.5f;
    float halfX, halfY;
    RadarGeometry::GetRadarHalfExtents(sizeX, sizeY, (float)RadarConfig::GetBorderThickness(), m_bRadarShapeCircle, halfX, halfY);
    bool useSquareOrbit = !m_bRadarShapeCircle;
    float offsetWorldX, offsetWorldY;
    D3DXVECTOR3 cameraPos, cameraRot;
    m_pCameraController->GetCachedCalculations(offsetWorldX, offsetWorldY, cameraPos, cameraRot);
    const CameraController::CameraState& camState = m_pCameraController->GetState();
    float rtWidth = m_pRenderTarget ? (float)m_pRenderTarget->GetWidth() : 0.0f;
    float rtHeight = m_pRenderTarget ? (float)m_pRenderTarget->GetHeight() : 0.0f;
    float screenAspect = (m_width > 0) ? ((float)m_height / (float)m_width) : 1.0f;

    bool hideLegendsFromOrbit = false;
    for (unsigned int i = 0; i < MAX_RADAR_TRACES; i++)
    {
        const tRadarTrace& t = CRadar::ms_RadarTrace[i];
        if (!t.m_bInUse) continue;
        bool hasIndicator = (t.m_nBlipType == BLIP_CHAR || t.m_nBlipType == BLIP_CAR ||
            (t.m_nBlipType == BLIP_COORD && BlipManager::IsMissionCheckpointSprite(t.m_nRadarSprite)) ||
            (t.m_nBlipType == BLIP_CONTACTPOINT && BlipManager::IsMissionCheckpointSprite(t.m_nRadarSprite)));
        if (hasIndicator) { hideLegendsFromOrbit = true; break; }
    }

    float iconSize = CalculateBlipSize(24.0f);
    DWORD iconColor = tocolor(255, 255, 255, 255);

    for (unsigned int i = 0; i < MAX_RADAR_TRACES; i++)
    {
        tRadarTrace& trace = CRadar::ms_RadarTrace[i];
        if (!trace.m_bInUse || trace.m_nRadarSprite == RADAR_SPRITE_NONE || !BlipManager::IsLegendSprite(trace.m_nRadarSprite))
            continue;

        LPDIRECT3DTEXTURE9 blipTexture = m_pBlipManager->GetBlipTexture(trace.m_nRadarSprite);
        if (!blipTexture)
            continue;

        float wx, wy, wz;
        GetLegendTraceWorldPosition(trace, wx, wy, wz);
        // Same height as regular blips: fixed 0.1f (radar plane), not world Z
        D3DXVECTOR3 blipWorldPos(wx + RadarGeometry::RADAR_OFFSET_X, wy + RadarGeometry::RADAR_OFFSET_Y, 0.1f);

        float screenX, screenY;
        if (!MathUtils::WorldToScreen(blipWorldPos, cameraPos, cameraRot, camState.fov, m_nearPlane, m_farPlane, rtWidth, rtHeight, screenX, screenY, screenAspect))
        {
            if (hideLegendsFromOrbit)
                continue;
            D3DXVECTOR3 playerPos(camState.posX, camState.posY, 0.0f);
            D3DXVECTOR3 legendPos2D(wx + RadarGeometry::RADAR_OFFSET_X, wy + RadarGeometry::RADAR_OFFSET_Y, 0.0f);
            float angle;
            if (!MathUtils::DirectionToOrbitAngle(playerPos, legendPos2D, camState.yaw, angle))
                continue;
            float edgeCenterX, edgeCenterY;
            RadarGeometry::PointOnOrbitEdge(centerX, centerY, halfX, halfY, cosf(angle), sinf(angle), useSquareOrbit, edgeCenterX, edgeCenterY);
            float edgeX = edgeCenterX - iconSize * 0.5f;
            float edgeY = edgeCenterY - iconSize * 0.5f;
            m_pDraw->dxDrawImage2D(edgeX, edgeY, iconSize, iconSize, blipTexture, iconColor);
            continue;
        }

        float normalizedX = screenX / rtWidth;
        float normalizedY = screenY / rtHeight;
        float circleScreenX = centerX + (normalizedX - 0.5f) * sizeX;
        float circleScreenY = centerY + (normalizedY - 0.5f) * sizeY;

        if (RadarGeometry::IsInsideOrbit(circleScreenX, circleScreenY, centerX, centerY, halfX, halfY, useSquareOrbit))
        {
            float iconX = circleScreenX - iconSize * 0.5f;
            float iconY = circleScreenY - iconSize * 0.5f;
            m_pDraw->dxDrawImage2D(iconX, iconY, iconSize, iconSize, blipTexture, iconColor);
        }
        else if (!hideLegendsFromOrbit)
        {
            float orbitX, orbitY;
            RadarGeometry::ClampToOrbit(circleScreenX, circleScreenY, centerX, centerY, halfX, halfY, orbitX, orbitY, useSquareOrbit);
            m_pDraw->dxDrawImage2D(orbitX - iconSize * 0.5f, orbitY - iconSize * 0.5f, iconSize, iconSize, blipTexture, iconColor);
        }
    }
}

// Глобальная переменная для отслеживания последней станции
static signed char g_lastRadioId = -1;
static unsigned int g_lastScrollTime = 0;

/**
 * Отрисовка текста радио (название станции и трека)
 * Использует CAERadioTrackManager для получения текущей радиостанции
 */
void RadarRenderer::RenderRadioText()
{
    if (!m_pRenderRadio || !m_pRadioFont)
        return;

    // Проверяем, включено ли радио
    if (!AERadioTrackManager.IsVehicleRadioActive())
        return;

    // Получаем текущую станцию
    signed char radioId = AERadioTrackManager.m_TempSettings.m_nCurrentRadioStation;
    
    // Если радио выключено (ID > 12 или < 0), не рисуем
    if (radioId < 0 || radioId > 12)
        return;

    // Проверяем, не было ли прокрутки колеса (отслеживаем смену станции)
    unsigned int currentTime = CTimer::m_snTimeInMilliseconds;
    if (radioId != g_lastRadioId && g_lastRadioId >= 0)
    {
        // Станция изменилась - запоминаем время
        g_lastScrollTime = currentTime;
    }
    g_lastRadioId = radioId;

    // Получаем название радиостанции
    const char* stationName = AERadioTrackManager.GetRadioStationName(radioId);
    if (!stationName || stationName[0] == '\0')
        return;

    // Позиция текста радио (над радаром по центру, отступ 10px вверх)
    float circleX, circleY, sizeX, sizeY;
    CalculateRadarPosition(circleX, circleY, sizeX, sizeY);

    float textWidth = 250.0f;
    float textHeight = 80.0f;  // Увеличили высоту для большего шрифта


    float radarCenterX = circleX + sizeX * 0.5f;
    float textX = radarCenterX - textWidth * 0.5f;
    float textY = circleY - textHeight - 10.0f;

    // Рисуем текст радио с чёрным контуром
    // Используем всегда оранжево-коричневый цвет, независимо от состояния игры
    m_pRenderRadio->RenderWithOutline(
        stationName,
        "",
        textX, textY,
        textWidth, textHeight,
        tocolor(144, 96, 16, 255), // Оранжево-коричневый текст (всегда)
        tocolor(0, 0, 0, 255),      // Чёрный контур
        66.0f                       // Размер шрифта (увеличен в 2 раза)
    );
}
