#ifndef SIMULATION_GAME_TILE_MAP_LOADER_H
#define SIMULATION_GAME_TILE_MAP_LOADER_H

#include <memory>
#include <optional>
#include <string>

#include <android/asset_manager.h>

#include "../../domain/entities/GameMap.h"
#include "TextureAsset.h"

struct TileMapLoadResult {
    std::shared_ptr<GameMap> map;
    std::shared_ptr<TextureAsset> texture;
};

/**
 * Helper for translating PNG map chips into the domain GameMap representation.
 * Each pixel corresponds to one tile. Pixel colors are mapped to TerrainType values.
 */
class TileMapLoader {
public:
    static std::optional<TileMapLoadResult> loadFromPng(AAssetManager* assetManager,
                                                        const std::string& assetPath,
                                                        float tileSize);
};

#endif // SIMULATION_GAME_TILE_MAP_LOADER_H
