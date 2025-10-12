#ifndef SIMULATION_GAME_GAME_MAP_TEST_H
#define SIMULATION_GAME_GAME_MAP_TEST_H

#include "../domain/entities/GameMap.h"
#include "../domain/value_objects/Position.h"
#include <cassert>
#include <iostream>

class GameMapTest {
public:
    static void runAllTests() {
        std::cout << "Running GameMap tests..." << std::endl;
        testTerrainLookup();
        testMovementStoppingBeforeWater();
        testClampInside();
        std::cout << "GameMap tests passed!" << std::endl;
    }

private:
    static GameMap buildSampleMap() {
        GameMap map(4, 4, 1.0f, 0.0f, 0.0f);
        // Fill row 0-2 with grass, last row water
        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 4; ++x) {
                map.setTile(x, y, TerrainType::Grassland);
            }
        }
        for (int x = 0; x < 4; ++x) {
            map.setTile(x, 3, TerrainType::Water);
        }
        map.setTile(1, 1, TerrainType::Forest);
        map.setTile(2, 1, TerrainType::Mountain);
        return map;
    }

    static void testTerrainLookup() {
        GameMap map = buildSampleMap();
        assert(map.terrainAt(Position(0.5f, 0.5f)) == TerrainType::Grassland);
        assert(map.terrainAt(Position(1.5f, 1.5f)) == TerrainType::Forest);
        assert(map.terrainAt(Position(2.5f, 1.5f)) == TerrainType::Mountain);
        assert(map.isWalkable(Position(0.5f, 0.5f)));
        assert(!map.isWalkable(Position(0.5f, 3.5f)));
        float forestSpeed = map.getMovementMultiplier(Position(1.5f, 1.5f));
        assert(forestSpeed > 0.0f && forestSpeed < 1.0f);
    }

    static void testMovementStoppingBeforeWater() {
        GameMap map = buildSampleMap();
        Position start(1.5f, 1.5f);
        Position desired(1.5f, 3.6f);
        Position resolved = map.resolveMovementTarget(start, desired, 0.1f);
        assert(resolved.getY() < 3.0f);
        assert(map.isWalkable(resolved));
    }

    static void testClampInside() {
        GameMap map = buildSampleMap();
        Position outside(-5.0f, 10.0f);
        Position clamped = map.clampInside(outside, 0.0f);
        assert(clamped.getX() >= map.getMinX());
        assert(clamped.getY() <= map.getMaxY());
    }
};

#endif // SIMULATION_GAME_GAME_MAP_TEST_H
