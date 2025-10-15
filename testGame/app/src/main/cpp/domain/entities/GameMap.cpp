#include "GameMap.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "../value_objects/TerrainType.h"

namespace {
constexpr float kEpsilon = 1e-5f;
constexpr int kBinarySearchIterations = 12;
constexpr float kContactTolerance = 1e-4f;
constexpr float kContactBackoff = 1e-3f;

bool segmentIntersectsAabb(const Position &start, const Position &end,
                           float minX, float minY, float maxX, float maxY,
                           float &outTEnter) {
  const float startX = start.getX();
  const float startY = start.getY();
  const float deltaX = end.getX() - startX;
  const float deltaY = end.getY() - startY;

  // まず開始点がAABB内かどうかを確認（既に侵入しているケース）
  if (startX >= minX && startX <= maxX && startY >= minY &&
      startY <= maxY) {
    outTEnter = 0.0f;
    return true;
  }

  float tMin = 0.0f;
  float tMax = 1.0f;

  auto intersectAxis = [&](float startCoord, float deltaCoord, float minCoord,
                           float maxCoord) -> bool {
    if (std::abs(deltaCoord) < kEpsilon) {
      return startCoord >= minCoord && startCoord <= maxCoord;
    }

    float inv = 1.0f / deltaCoord;
    float t1 = (minCoord - startCoord) * inv;
    float t2 = (maxCoord - startCoord) * inv;
    if (t1 > t2) {
      std::swap(t1, t2);
    }

    tMin = std::max(tMin, t1);
    tMax = std::min(tMax, t2);
    return tMin <= tMax;
  };

  if (!intersectAxis(startX, deltaX, minX, maxX)) {
    return false;
  }

  if (!intersectAxis(startY, deltaY, minY, maxY)) {
    return false;
  }

  if (tMax < 0.0f || tMin > 1.0f) {
    return false;
  }

  outTEnter = std::clamp(tMin, 0.0f, 1.0f);
  return true;
}
} // namespace

GameMap::GameMap(int width, int height, float tileSize, float minX, float minY)
    : width_(width), height_(height), tileSize_(tileSize), minX_(minX),
      minY_(minY), maxX_(minX + tileSize * width),
      maxY_(minY + tileSize * height),
      tiles_(static_cast<size_t>(width) * height, TerrainType::Unknown) {}

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

TerrainType GameMap::terrainAt(const Position &worldPos) const {
  int tileX = 0;
  int tileY = 0;
  if (!positionToTile(worldPos, tileX, tileY)) {
    return TerrainType::Unknown;
  }
  return getTile(tileX, tileY);
}

float GameMap::getMovementMultiplier(const Position &worldPos,
                                     float radius) const {
  const float effectiveRadius = std::max(radius, 0.0f);
  if (effectiveRadius <= kEpsilon) {
    TerrainType terrain = terrainAt(worldPos);
    return getTerrainProperties(terrain).movementSpeedMultiplier;
  }

  int minTileX = 0;
  int maxTileX = 0;
  int minTileY = 0;
  int maxTileY = 0;
  if (!computeTileRangeForCircle(worldPos, effectiveRadius, minTileX, maxTileX,
                                 minTileY, maxTileY)) {
    return 0.0f;
  }

  float minMultiplier = std::numeric_limits<float>::max();
  bool touchedAnyTile = false;

  for (int ty = minTileY; ty <= maxTileY; ++ty) {
    for (int tx = minTileX; tx <= maxTileX; ++tx) {
      if (!circleIntersectsTile(tx, ty, worldPos, effectiveRadius)) {
        continue;
      }

      touchedAnyTile = true;
      TerrainProperties props = getTerrainProperties(getTile(tx, ty));
      if (!props.walkable) {
        return 0.0f;
      }
      minMultiplier = std::min(minMultiplier, props.movementSpeedMultiplier);
    }
  }

  if (!touchedAnyTile) {
    TerrainType terrain = terrainAt(worldPos);
    return getTerrainProperties(terrain).movementSpeedMultiplier;
  }

  if (minMultiplier == std::numeric_limits<float>::max()) {
    return 1.0f;
  }

  return std::max(0.0f, minMultiplier);
}

bool GameMap::isWalkable(const Position &worldPos, float radius) const {
  const float effectiveRadius = std::max(radius, 0.0f);
  if (effectiveRadius <= kEpsilon) {
    TerrainType terrain = terrainAt(worldPos);
    return getTerrainProperties(terrain).walkable;
  }

  int minTileX = 0;
  int maxTileX = 0;
  int minTileY = 0;
  int maxTileY = 0;
  const bool inBounds = computeTileRangeForCircle(worldPos, effectiveRadius,
                                                  minTileX, maxTileX,
                                                  minTileY, maxTileY);
  bool touchedAnyTile = false;

  if (!inBounds) {
    // 中心がマップ境界付近の場合、円がはみ出していても、内部にある分については歩行可能性を確認する
    minTileX = std::max(0, minTileX);
    maxTileX = std::min(width_ - 1, maxTileX);
    minTileY = std::max(0, minTileY);
    maxTileY = std::min(height_ - 1, maxTileY);
  }

  for (int ty = minTileY; ty <= maxTileY; ++ty) {
    for (int tx = minTileX; tx <= maxTileX; ++tx) {
      if (!circleIntersectsTile(tx, ty, worldPos, effectiveRadius)) {
        continue;
      }

      touchedAnyTile = true;
      TerrainProperties props = getTerrainProperties(getTile(tx, ty));
      if (!props.walkable) {
        return false;
      }
    }
  }

  if (!touchedAnyTile) {
    TerrainType terrain = terrainAt(worldPos);
    return getTerrainProperties(terrain).walkable;
  }

  return touchedAnyTile && inBounds;
}

Position GameMap::clampInside(const Position &worldPos, float radius) const {
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

Position GameMap::resolveMovementTarget(const Position &start,
                                        const Position &desired,
                                        float radius) const {
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
  const float maxStepByTile = tileSize_ / samplesPerTile;
  const float maxStepByRadius = std::max(radius * 0.5f, 0.02f);
  const float sampleLength =
      std::max(kEpsilon, std::min(maxStepByTile, maxStepByRadius));

  int sampleCount = std::max(1, static_cast<int>(
                             std::ceil(distance / sampleLength)));
  if (distance > kEpsilon) {
    sampleCount = std::max(2, sampleCount);
  }

  const float stepX = dx / static_cast<float>(sampleCount);
  const float stepY = dy / static_cast<float>(sampleCount);
  Position lastWalkable = clampInside(start, radius);

  if (!isWalkable(lastWalkable, radius)) {
    return lastWalkable;
  }

  for (int i = 1; i <= sampleCount; ++i) {
    Position sample(start.getX() + stepX * i, start.getY() + stepY * i);
    sample = clampInside(sample, radius);
    if (!isWalkable(sample, radius)) {
      Position contact = findContactAlongPath(lastWalkable, sample, radius);
      return clampInside(contact, radius);
    }
    lastWalkable = sample;
  }

  return lastWalkable;
}

bool GameMap::positionToTile(const Position &worldPos, int &tileX,
                             int &tileY) const {
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

int GameMap::toIndex(int x, int y) const { return y * width_ + x; }

bool GameMap::computeTileRangeForCircle(const Position &center, float radius,
                                        int &minTileX, int &maxTileX,
                                        int &minTileY, int &maxTileY) const {
  // 半径を考慮したバウンディングボックスを算出し、マップ境界からはみ出したかを返す
  const float effectiveRadius = std::max(radius, 0.0f);
  const float left = center.getX() - effectiveRadius;
  const float right = center.getX() + effectiveRadius;
  const float bottom = center.getY() - effectiveRadius;
  const float top = center.getY() + effectiveRadius;

  const bool fullyInside = (left >= minX_ && right <= maxX_ && bottom >= minY_ &&
                            top <= maxY_);

  const float clampedLeft = std::max(left, minX_);
  const float clampedRight = std::min(right, maxX_);
  const float clampedBottom = std::max(bottom, minY_);
  const float clampedTop = std::min(top, maxY_);

  auto toTileFloorX = [&](float coordinate) {
    float local = (coordinate - minX_) / tileSize_;
    int tile = static_cast<int>(std::floor(local));
    return std::clamp(tile, 0, width_ - 1);
  };

  auto toTileCeilX = [&](float coordinate) {
    float local = (coordinate - minX_) / tileSize_;
    int tile = static_cast<int>(std::ceil(local));
    return std::clamp(tile - 1, 0, width_ - 1);
  };

  auto toTileFloorY = [&](float coordinate) {
    float local = (coordinate - minY_) / tileSize_;
    int tile = static_cast<int>(std::floor(local));
    return std::clamp(tile, 0, height_ - 1);
  };

  auto toTileCeilY = [&](float coordinate) {
    float local = (coordinate - minY_) / tileSize_;
    int tile = static_cast<int>(std::ceil(local));
    return std::clamp(tile - 1, 0, height_ - 1);
  };

  minTileX = toTileFloorX(clampedLeft);
  maxTileX = toTileCeilX(clampedRight);
  minTileY = toTileFloorY(clampedBottom);
  maxTileY = toTileCeilY(clampedTop);

  if (minTileX > maxTileX || minTileY > maxTileY) {
    return false;
  }

  return fullyInside;
}

bool GameMap::circleIntersectsTile(int tileX, int tileY,
                                   const Position &center, float radius) const {
  if (tileX < 0 || tileX >= width_ || tileY < 0 || tileY >= height_) {
    return false;
  }

  const float tileMinX = minX_ + tileX * tileSize_;
  const float tileMaxX = tileMinX + tileSize_;
  const float tileMinY = minY_ + tileY * tileSize_;
  const float tileMaxY = tileMinY + tileSize_;

  const float closestX = std::clamp(center.getX(), tileMinX, tileMaxX);
  const float closestY = std::clamp(center.getY(), tileMinY, tileMaxY);
  const float dx = center.getX() - closestX;
  const float dy = center.getY() - closestY;

  return (dx * dx + dy * dy) <= (radius * radius + kEpsilon);
}

Position GameMap::findContactAlongPath(const Position &walkablePoint,
                                       const Position &blockedPoint,
                                       float radius) const {
  // 二分探索で「歩ける点」と「歩けない点」の境界を求め、タイルに接する位置を導く
  Position low = walkablePoint;
  Position high = blockedPoint;
  Position best = low;

  for (int i = 0; i < kBinarySearchIterations; ++i) {
    Position mid = low.midpointWith(high);
    if (mid.distanceTo(low) <= kContactTolerance) {
      break;
    }

    if (isWalkable(mid, radius)) {
      low = mid;
      best = mid;
    } else {
      high = mid;
    }
  }

  return best;
}

GameMap::MovementRaycastResult
GameMap::clipMovementRaycast(const Position &start, const Position &desired,
                             float radius) const {
  MovementRaycastResult result;

  Position clampedStart = clampInside(start, radius);
  Position clampedDesired = clampInside(desired, radius);

  result.position = clampedDesired;
  result.hitBlocking = false;

  const float distance = clampedStart.distanceTo(clampedDesired);
  if (distance <= kEpsilon) {
    if (!isWalkable(clampedDesired, radius) ||
        getMovementMultiplier(clampedDesired, radius) <= 0.0f) {
      result.position = clampedStart;
      result.hitBlocking = true;
    }
    return result;
  }

  const float dirX = clampedDesired.getX() - clampedStart.getX();
  const float dirY = clampedDesired.getY() - clampedStart.getY();

  const float minWorldX =
      std::min(clampedStart.getX(), clampedDesired.getX()) - radius;
  const float maxWorldX =
      std::max(clampedStart.getX(), clampedDesired.getX()) + radius;
  const float minWorldY =
      std::min(clampedStart.getY(), clampedDesired.getY()) - radius;
  const float maxWorldY =
      std::max(clampedStart.getY(), clampedDesired.getY()) + radius;

  auto worldToTileClamped = [&](float coord, float minBoundary,
                                float maxBoundary, int maxIndex) {
    float value = std::clamp(coord, minBoundary + kEpsilon,
                             maxBoundary - kEpsilon);
    float local = (value - minBoundary) / tileSize_;
    int tile = static_cast<int>(std::floor(local));
    return std::clamp(tile, 0, maxIndex);
  };

  int minTileX = worldToTileClamped(minWorldX, minX_, maxX_, width_ - 1);
  int maxTileX = worldToTileClamped(maxWorldX, minX_, maxX_, width_ - 1);
  int minTileY = worldToTileClamped(minWorldY, minY_, maxY_, height_ - 1);
  int maxTileY = worldToTileClamped(maxWorldY, minY_, maxY_, height_ - 1);

  float earliestHitT = 1.0f;
  bool hitBlocking = false;

  for (int ty = minTileY; ty <= maxTileY; ++ty) {
    for (int tx = minTileX; tx <= maxTileX; ++tx) {
      TerrainProperties props = getTerrainProperties(getTile(tx, ty));
      if (props.walkable && props.movementSpeedMultiplier > kEpsilon) {
        continue;
      }

      const float tileMinX = minX_ + tx * tileSize_ - radius;
      const float tileMaxX = tileMinX + tileSize_ + radius * 2.0f;
      const float tileMinY = minY_ + ty * tileSize_ - radius;
      const float tileMaxY = tileMinY + tileSize_ + radius * 2.0f;

      float tEnter = 1.0f;
      if (!segmentIntersectsAabb(clampedStart, clampedDesired, tileMinX,
                                 tileMinY, tileMaxX, tileMaxY, tEnter)) {
        continue;
      }

      if (tEnter < earliestHitT) {
        earliestHitT = tEnter;
        hitBlocking = true;
      }
    }
  }

  if (hitBlocking) {
    const float clampedT = std::clamp(earliestHitT, 0.0f, 1.0f);
    float adjustT = clampedT;
    if (distance > kEpsilon) {
      const float backoff = kContactBackoff / distance;
      adjustT = std::max(0.0f, clampedT - backoff);
    }

    Position contact(clampedStart.getX() + dirX * adjustT,
                     clampedStart.getY() + dirY * adjustT);
    result.position = clampInside(contact, radius);
    result.hitBlocking = true;
    return result;
  }

  // 最終到達点が安全であるかを念のため確認
  if (!isWalkable(clampedDesired, radius) ||
      getMovementMultiplier(clampedDesired, radius) <= 0.0f) {
    result.position = clampInside(clampedStart, radius);
    result.hitBlocking = true;
  }

  return result;
}
