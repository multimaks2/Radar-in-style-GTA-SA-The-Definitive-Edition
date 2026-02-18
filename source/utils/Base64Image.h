/*****************************************************************************
 *  PROJECT:     Radar Trilogy SA
 *  FILE:        source/utils/Base64Image.h
 *****************************************************************************/

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Base64Image {

    std::vector<uint8_t> Decode(const std::string& base64String);
    std::string          Encode(const uint8_t* data, size_t length);
    std::string          Encode(const std::vector<uint8_t>& data);
    std::vector<uint8_t> LoadImageFromBase64(const std::string& base64String, int& outWidth, int& outHeight, int& outChannels);
    void*                CreateTextureFromBase64(void* pDevice, const std::string& base64String, int& outWidth, int& outHeight);
    const std::string&   GetEmbeddedImageBase64();
    void                 ClearEmbeddedImageData();

} // namespace Base64Image
