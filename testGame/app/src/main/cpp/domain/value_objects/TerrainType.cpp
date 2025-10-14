#include "TerrainType.h"

#include <algorithm>

namespace {
// movementSpeedMultiplier / walkable / evasionBonus
constexpr TerrainProperties kGrassland{1.0f, true, 0.0f};
constexpr TerrainProperties kForest{0.01f, true, 0.15f};
constexpr TerrainProperties kMountain{0.005f, true, 0.25f};
constexpr TerrainProperties kWater{0.0f, false, 0.0f};
constexpr TerrainProperties kRiver{0.0001f, true, 0.05f};
constexpr TerrainProperties kUnknown{1.0f, true, 0.0f};
} // namespace

TerrainProperties getTerrainProperties(TerrainType type) {
  switch (type) {
  case TerrainType::Grassland:
    return kGrassland;
  case TerrainType::Forest:
    return kForest;
  case TerrainType::Mountain:
    return kMountain;
  case TerrainType::Water:
    return kWater;
  case TerrainType::River:
    return kRiver;
  case TerrainType::Unknown:
  default:
    return kUnknown;
  }
}

const char *toString(TerrainType type) {
  switch (type) {
  case TerrainType::Grassland:
    return "Grassland";
  case TerrainType::Forest:
    return "Forest";
  case TerrainType::Mountain:
    return "Mountain";
  case TerrainType::Water:
    return "Water";
  case TerrainType::River:
    return "River";
  case TerrainType::Unknown:
  default:
    return "Unknown";
  }
}
