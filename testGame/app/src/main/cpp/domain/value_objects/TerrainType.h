#ifndef SIMULATION_GAME_TERRAIN_TYPE_H
#define SIMULATION_GAME_TERRAIN_TYPE_H

#include <string>

/**
 * Represents terrain categories used by the tactical map.
 * Each terrain type exposes movement and combat related modifiers via TerrainProperties.
 */
enum class TerrainType {
    Grassland,
    Forest,
    Mountain,
    Water,
    River,
    Unknown
};

struct TerrainProperties {
    float movementSpeedMultiplier; // Scalar applied to unit base move speed
    bool walkable;                 // False indicates the tile blocks movement entirely
    float evasionBonus;            // Placeholder for combat bonuses (not yet consumed)
};

TerrainProperties getTerrainProperties(TerrainType type);
const char* toString(TerrainType type);

#endif // SIMULATION_GAME_TERRAIN_TYPE_H
