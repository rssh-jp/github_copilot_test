#ifndef SIMULATION_GAME_GAME_MAP_H
#define SIMULATION_GAME_GAME_MAP_H

#include <vector>

#include "../value_objects/Position.h"
#include "../value_objects/TerrainType.h"

/**
 * Immutable-style representation of the tactical map.
 * Stores a grid of TerrainType values with helpers to answer pathing related queries.
 * Coordinates are expressed in world units: (minX_, minY_) denotes the bottom-left corner.
 */
class GameMap {
public:
	GameMap(int width, int height, float tileSize, float minX, float minY);

	int getWidth() const { return width_; }
	int getHeight() const { return height_; }
	float getTileSize() const { return tileSize_; }

	float getMinX() const { return minX_; }
	float getMinY() const { return minY_; }
	float getMaxX() const { return maxX_; }
	float getMaxY() const { return maxY_; }

	void setTile(int x, int y, TerrainType terrain);
	TerrainType getTile(int x, int y) const;

	TerrainType terrainAt(const Position& worldPos) const;
	float getMovementMultiplier(const Position& worldPos) const;
	bool isWalkable(const Position& worldPos, float radius = 0.0f) const;
	Position clampInside(const Position& worldPos, float radius = 0.0f) const;
	Position resolveMovementTarget(const Position& start, const Position& desired, float radius) const;

private:
	bool positionToTile(const Position& worldPos, int& tileX, int& tileY) const;
	int toIndex(int x, int y) const;

	int width_;
	int height_;
	float tileSize_;
	float minX_;
	float minY_;
	float maxX_;
	float maxY_;
	std::vector<TerrainType> tiles_;
};

#endif // SIMULATION_GAME_GAME_MAP_H
