#include "GameMap.h"

#include <algorithm>
#include <cmath>

#include "../value_objects/TerrainType.h"

namespace {
constexpr float kEpsilon = 1e-5f;
constexpr float kStopBuffer = 0.02f; // distance to stay away from blocking tiles
}

GameMap::GameMap(int width, int height, float tileSize, float minX, float minY)
    : width_(width),
      height_(height),
      tileSize_(tileSize),
      minX_(minX),
      minY_(minY),
      maxX_(minX + tileSize * width),
      maxY_(minY + tileSize * height),
      tiles_(static_cast<size_t>(width) * height, TerrainType::Unknown) {
}

void GameMap::setTile(int x, int y, TerrainType terrain) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return;
    }
    tiles_[toIndex(x, y)] = terrain;
}

TerrainType GameMap::getTile(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return TerrainType::Unknown;
    }
    return tiles_[toIndex(x, y)];
}

TerrainType GameMap::terrainAt(const Position& worldPos) const {
    int tileX = 0;
    int tileY = 0;
    if (!positionToTile(worldPos, tileX, tileY)) {
        return TerrainType::Unknown;
    }
    return getTile(tileX, tileY);
}

float GameMap::getMovementMultiplier(const Position& worldPos) const {
    // TerrainType ごとに決まった移動倍率（草原=1.0f, 森=0.4f など）を返す
    TerrainType terrain = terrainAt(worldPos);
    return getTerrainProperties(terrain).movementSpeedMultiplier;
}

bool GameMap::isWalkable(const Position& worldPos, float /*radius*/) const {
    TerrainType terrain = terrainAt(worldPos);
    return getTerrainProperties(terrain).walkable;
}

Position GameMap::clampInside(const Position& worldPos, float radius) const {
    float minAllowedX = minX_ + radius;
    float maxAllowedX = maxX_ - radius;
    if (minAllowedX > maxAllowedX) {
        const float centerX = (minX_ + maxX_) * 0.5f;
        minAllowedX = maxAllowedX = centerX;
    }

    float minAllowedY = minY_ + radius;
    float maxAllowedY = maxY_ - radius;
    if (minAllowedY > maxAllowedY) {
        const float centerY = (minY_ + maxY_) * 0.5f;
        minAllowedY = maxAllowedY = centerY;
    }

    float clampedX = std::clamp(worldPos.getX(), minAllowedX, maxAllowedX);
    float clampedY = std::clamp(worldPos.getY(), minAllowedY, maxAllowedY);
    return Position(clampedX, clampedY);
}

Position GameMap::resolveMovementTarget(const Position& start, const Position& desired, float radius) const {
    Position clampedDesired = clampInside(desired, radius);
    if (isWalkable(clampedDesired, radius)) {
        return clampedDesired;
    }

    const float dx = clampedDesired.getX() - start.getX();
    const float dy = clampedDesired.getY() - start.getY();
    const float distance = std::sqrt(dx * dx + dy * dy);
    if (distance <= kEpsilon) {
        return clampInside(start, radius);
    }

    const float samplesPerTile = 4.0f;
    const float sampleLength = std::max(tileSize_ / samplesPerTile, 0.05f);
    const int sampleCount = std::max(1, static_cast<int>(std::ceil(distance / sampleLength)));
    const float stepX = dx / static_cast<float>(sampleCount);
    const float stepY = dy / static_cast<float>(sampleCount);
    Position lastWalkable = clampInside(start, radius);

    for (int i = 1; i <= sampleCount; ++i) {
        Position sample(start.getX() + stepX * i, start.getY() + stepY * i);
        sample = clampInside(sample, radius);
        if (!isWalkable(sample, radius)) {
            const float invDistance = 1.0f / std::max(distance, kEpsilon);
            const float offsetX = dx * invDistance * kStopBuffer;
            const float offsetY = dy * invDistance * kStopBuffer;
            Position adjusted(lastWalkable.getX() - offsetX, lastWalkable.getY() - offsetY);
            return clampInside(adjusted, radius);
        }
        lastWalkable = sample;
    }

    return lastWalkable;
}

bool GameMap::positionToTile(const Position& worldPos, int& tileX, int& tileY) const {
    if (worldPos.getX() < minX_ || worldPos.getX() >= maxX_ ||
        worldPos.getY() < minY_ || worldPos.getY() >= maxY_) {
        return false;
    }

    float localX = (worldPos.getX() - minX_) / tileSize_;
    float localY = (worldPos.getY() - minY_) / tileSize_;

    tileX = static_cast<int>(std::floor(localX));
    tileY = static_cast<int>(std::floor(localY));

    if (tileX < 0 || tileX >= width_ || tileY < 0 || tileY >= height_) {
        return false;
    }
    return true;
}

int GameMap::toIndex(int x, int y) const {
    return y * width_ + x;
}
