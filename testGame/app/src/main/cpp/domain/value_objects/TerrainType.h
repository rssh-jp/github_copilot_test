#ifndef SIMULATION_GAME_TERRAIN_TYPE_H
#define SIMULATION_GAME_TERRAIN_TYPE_H

#include <string>

/**
 * Represents terrain categories used by the tactical map.
 * Each terrain type exposes movement and combat related modifiers via
 * TerrainProperties.
 */
enum class TerrainType { Grassland, Forest, Mountain, Water, River, Unknown };

struct TerrainProperties {
  float movementSpeedMultiplier; // ユニットの基本移動速度に掛ける倍率
  bool walkable; // false の場合、そのタイルは移動を完全に遮断する
  float evasionBonus; // 戦闘ボーナス用の仮プレースホルダー（未使用）
};

TerrainProperties getTerrainProperties(TerrainType type);
const char *toString(TerrainType type);

#endif // SIMULATION_GAME_TERRAIN_TYPE_H
