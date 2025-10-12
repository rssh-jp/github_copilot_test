#include "TileMapLoader.h"

#include <android/imagedecoder.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cmath>
#include <vector>

#include "android/AndroidOut.h"

namespace {
struct TerrainColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    TerrainType terrain;
};

constexpr std::array<TerrainColor, 5> kColorTable = {{
    {168, 230, 161, TerrainType::Grassland}, // light green
    {42, 123, 42, TerrainType::Forest},      // rich green
    {139, 69, 19, TerrainType::Mountain},    // brown
    {30, 96, 220, TerrainType::Water},       // deep blue
    {135, 206, 250, TerrainType::River}      // light blue
}};

constexpr uint8_t kMatchTolerance = 10;

TerrainType colorToTerrain(uint8_t r, uint8_t g, uint8_t b) {
    for (const auto& entry : kColorTable) {
        if (std::abs(static_cast<int>(entry.r) - static_cast<int>(r)) <= kMatchTolerance &&
            std::abs(static_cast<int>(entry.g) - static_cast<int>(g)) <= kMatchTolerance &&
            std::abs(static_cast<int>(entry.b) - static_cast<int>(b)) <= kMatchTolerance) {
            return entry.terrain;
        }
    }
    return TerrainType::Unknown;
}
}

std::optional<TileMapLoadResult> TileMapLoader::loadFromPng(AAssetManager* assetManager,
                                                            const std::string& assetPath,
                                                            float tileSize) {
    if (!assetManager) {
        aout << "TileMapLoader: assetManager is null" << std::endl;
        return std::nullopt;
    }

    AAsset* asset = AAssetManager_open(assetManager, assetPath.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        aout << "TileMapLoader: failed to open asset " << assetPath << std::endl;
        return std::nullopt;
    }

    AImageDecoder* decoder = nullptr;
    auto createResult = AImageDecoder_createFromAAsset(asset, &decoder);
    if (createResult != ANDROID_IMAGE_DECODER_SUCCESS) {
        aout << "TileMapLoader: failed to create decoder for " << assetPath << std::endl;
        AAsset_close(asset);
        return std::nullopt;
    }

    AImageDecoder_setAndroidBitmapFormat(decoder, ANDROID_BITMAP_FORMAT_RGBA_8888);
    const AImageDecoderHeaderInfo* header = AImageDecoder_getHeaderInfo(decoder);
    const int width = AImageDecoderHeaderInfo_getWidth(header);
    const int height = AImageDecoderHeaderInfo_getHeight(header);
    const int stride = AImageDecoder_getMinimumStride(decoder);

    std::vector<uint8_t> decoded(static_cast<size_t>(height) * stride);
    auto decodeResult = AImageDecoder_decodeImage(decoder, decoded.data(), stride, decoded.size());
    if (decodeResult != ANDROID_IMAGE_DECODER_SUCCESS) {
        aout << "TileMapLoader: decode failed for " << assetPath << std::endl;
        AImageDecoder_delete(decoder);
        AAsset_close(asset);
        return std::nullopt;
    }

    std::vector<uint8_t> textureData(static_cast<size_t>(width) * height * 4);
    std::array<int, static_cast<size_t>(TerrainType::Unknown) + 1> counts{};
    counts.fill(0);

    auto map = std::make_shared<GameMap>(width, height, tileSize,
                                         -0.5f * tileSize * width,
                                         -0.5f * tileSize * height);

    for (int row = 0; row < height; ++row) {
        const uint8_t* rowPtr = decoded.data() + static_cast<size_t>(row) * stride;
        const int textureRow = height - 1 - row; // flip so texture matches world orientation
        for (int col = 0; col < width; ++col) {
            const uint8_t* pixel = rowPtr + col * 4;
            uint8_t r = pixel[0];
            uint8_t g = pixel[1];
            uint8_t b = pixel[2];
            TerrainType terrain = colorToTerrain(r, g, b);
            counts[static_cast<size_t>(terrain)] += 1;

            int mapY = height - 1 - row; // flip vertically so row 0 becomes top of the world
            map->setTile(col, mapY, terrain);

            size_t dstIndex = static_cast<size_t>(textureRow) * width * 4 + col * 4;
            textureData[dstIndex + 0] = r;
            textureData[dstIndex + 1] = g;
            textureData[dstIndex + 2] = b;
            textureData[dstIndex + 3] = pixel[3];
        }
    }

    AImageDecoder_delete(decoder);
    AAsset_close(asset);

    auto texture = TextureAsset::createFromPixels(width, height, textureData);
    if (!texture) {
        aout << "TileMapLoader: failed to upload texture for " << assetPath << std::endl;
        return std::nullopt;
    }

    aout << "TileMapLoader: loaded map " << assetPath << " (" << width << "x" << height << ")" << std::endl;
    aout << "  Bounds: X(" << map->getMinX() << ", " << map->getMaxX()
         << "), Y(" << map->getMinY() << ", " << map->getMaxY() << ")" << std::endl;

    for (const auto& entry : kColorTable) {
        TerrainType terrain = entry.terrain;
        size_t idx = static_cast<size_t>(terrain);
        if (idx < counts.size()) {
            aout << "  Tiles[" << toString(terrain) << "]: " << counts[idx] << std::endl;
        }
    }

    TileMapLoadResult result;
    result.map = std::move(map);
    result.texture = std::move(texture);
    return result;
}
