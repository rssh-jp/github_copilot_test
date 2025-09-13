#include "CollisionDomainService.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool CollisionDomainService::canMoveTo(
    const UnitEntity& unit,
    const Position& targetPosition,
    const std::vector<std::shared_ptr<UnitEntity>>& allUnits) {
    
    return !hasCollisionAt(targetPosition, allUnits, &unit);
}

Position CollisionDomainService::calculateAvoidancePosition(
    const UnitEntity& unit,
    const Position& targetPosition,
    const std::vector<std::shared_ptr<UnitEntity>>& allUnits) {
    
    // まず目標位置に移動可能かチェック
    if (canMoveTo(unit, targetPosition, allUnits)) {
        return targetPosition;
    }
    
    // 衝突がある場合、周囲の位置を試す
    const float avoidanceDistance = getCollisionRadius() * 2.5f;
    const int searchSteps = 8; // 8方向を試す
    
    for (int i = 0; i < searchSteps; ++i) {
        float angle = (2.0f * M_PI * i) / searchSteps;
        float offsetX = std::cos(angle) * avoidanceDistance;
        float offsetY = std::sin(angle) * avoidanceDistance;
        
        Position candidatePosition(
            targetPosition.getX() + offsetX,
            targetPosition.getY() + offsetY
        );
        
        if (canMoveTo(unit, candidatePosition, allUnits)) {
            return candidatePosition;
        }
    }
    
    // どこにも移動できない場合は現在位置を返す
    return unit.getPosition();
}

bool CollisionDomainService::hasCollisionAt(
    const Position& position,
    const std::vector<std::shared_ptr<UnitEntity>>& allUnits,
    const UnitEntity* excludeUnit) {
    
    for (const auto& otherUnit : allUnits) {
        if (otherUnit.get() == excludeUnit) {
            continue; // 自分自身は除外
        }
        
        if (isPointInCircle(position, otherUnit->getPosition(), getCollisionRadius())) {
            return true;
        }
    }
    
    return false;
}

bool CollisionDomainService::hasCollisionOnPath(
    const Position& start,
    const Position& end,
    const std::vector<std::shared_ptr<UnitEntity>>& allUnits,
    const UnitEntity* excludeUnit) {
    
    for (const auto& otherUnit : allUnits) {
        if (otherUnit.get() == excludeUnit) {
            continue; // 自分自身は除外
        }
        
        if (isLineSegmentIntersectingCircle(
            start, end, otherUnit->getPosition(), getCollisionRadius())) {
            return true;
        }
    }
    
    return false;
}

bool CollisionDomainService::isPointInCircle(
    const Position& point,
    const Position& circleCenter,
    float radius) {
    
    float dx = point.getX() - circleCenter.getX();
    float dy = point.getY() - circleCenter.getY();
    float distanceSquared = dx * dx + dy * dy;
    
    return distanceSquared < (radius * radius);
}

bool CollisionDomainService::isLineSegmentIntersectingCircle(
    const Position& lineStart,
    const Position& lineEnd,
    const Position& circleCenter,
    float radius) {
    
    // 線分の方向ベクトル
    float dx = lineEnd.getX() - lineStart.getX();
    float dy = lineEnd.getY() - lineStart.getY();
    
    // 線分の長さの2乗
    float lengthSquared = dx * dx + dy * dy;
    
    if (lengthSquared == 0) {
        // 線分が点の場合
        return isPointInCircle(lineStart, circleCenter, radius);
    }
    
    // 円の中心から線分への最短距離を計算
    float t = std::max(0.0f, std::min(1.0f, 
        ((circleCenter.getX() - lineStart.getX()) * dx + (circleCenter.getY() - lineStart.getY()) * dy) / lengthSquared));
    
    // 線分上の最近点
    Position closestPoint(
        lineStart.getX() + t * dx,
        lineStart.getY() + t * dy
    );
    
    return isPointInCircle(closestPoint, circleCenter, radius);
}
