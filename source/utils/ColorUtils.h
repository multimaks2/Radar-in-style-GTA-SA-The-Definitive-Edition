/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/utils/ColorUtils.h
 *****************************************************************************/

#pragma once

#include <d3d9.h>
#include <cstdint>

inline DWORD ToColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return D3DCOLOR_ARGB(a, r, g, b);
}

#define tocolor(r, g, b, a) D3DCOLOR_ARGB((a), (r), (g), (b))
