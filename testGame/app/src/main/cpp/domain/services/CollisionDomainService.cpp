#include "CollisionDomainService.h"
#include "../entities/UnitEntity.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool CollisionDomainService::canMoveTo(
    const UnitEntity& unit,
    const Position& targetPosition,
    const std::vector<std::shared_ptr<UnitEntity>>& allUnits) {
    // 衝突は、指定位置が他ユニットの衝突円と重なっているかで判定する。
    // ユニットごとの衝突半径を考慮し、距離が (r1 + r2) 未満なら衝突と見なす。
    for (const auto& otherUnit : allUnits) {
        if (otherUnit.get() == &unit) continue;
        float combinedRadius = unit.getStats().getCollisionRadius() + otherUnit->getStats().getCollisionRadius();
        if (isPointInCircle(targetPosition, otherUnit->getPosition(), combinedRadius)) {
            return false;
        }
    }
    return true;
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
    const UnitEntity* excludeUnit,
    float movingRadius) {
    
    for (const auto& otherUnit : allUnits) {
        if (otherUnit.get() == excludeUnit) {
            continue; // 自分自身は除外
        }
        float otherRadius = otherUnit->getStats().getCollisionRadius();
        // 移動体の半径と相手の半径を合算して判定
        float combined = movingRadius + otherRadius;
        if (isPointInCircle(position, otherUnit->getPosition(), combined)) {
            return true;
        }
    }
    
    return false;
}

bool CollisionDomainService::hasCollisionOnPath(
    const Position& start,
    const Position& end,
    const std::vector<std::shared_ptr<UnitEntity>>& allUnits,
    const UnitEntity* excludeUnit,
    float movingRadius) {
    
    for (const auto& otherUnit : allUnits) {
        if (otherUnit.get() == excludeUnit) {
            continue; // 自分自身は除外
        }
        float otherRadius = otherUnit->getStats().getCollisionRadius();
        float combined = movingRadius + otherRadius;
        if (isLineSegmentIntersectingCircle(
            start, end, otherUnit->getPosition(), combined)) {
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

bool CollisionDomainService::findFirstContactOnPath(
    const Position& start,
    const Position& end,
    const std::vector<std::shared_ptr<UnitEntity>>& allUnits,
    Position& outContactPosition,
    const UnitEntity*& outContactUnit,
    const UnitEntity* excludeUnit,
    float movingRadius) {

    bool found = false;
    float bestT = 1.0f + 1e-6f; // 初期は範囲外
    Position bestPos = outContactPosition;
    const UnitEntity* bestUnit = nullptr;

    // 線分をパラメトリックに表現: P(t) = start + t * (end - start), t in [0,1]
    float dx = end.getX() - start.getX();
    float dy = end.getY() - start.getY();
    const float EPS = 1e-6f;

    for (const auto& otherPtr : allUnits) {
        const UnitEntity* other = otherPtr.get();
        if (other == excludeUnit) continue;

        float otherRadius = other->getStats().getCollisionRadius();
        float combined = movingRadius + otherRadius;

        // Solve |start + t * d - C|^2 = combined^2  for t
        float fx = start.getX() - other->getPosition().getX();
        float fy = start.getY() - other->getPosition().getY();

        float a = dx * dx + dy * dy;
        float b = 2.0f * (fx * dx + fy * dy);
        float c = fx * fx + fy * fy - combined * combined;

        // Handle near-zero length segment: fall back to point-in-circle check
        if (a < EPS) {
            // If start is inside the combined circle, t = 0 contact
            if (c <= 0.0f) {
                float candidateT = 0.0f;
                if (candidateT < bestT) {
                    bestT = candidateT;
                    bestPos = start;
                    bestUnit = other;
                    found = true;
                }
            }
            continue;
        }

        // Quadratic discriminant
        float disc = b * b - 4.0f * a * c;
        if (disc < 0.0f) continue; // 衝突なし

        float sqrtD = std::sqrt(disc);
        float t1 = (-b - sqrtD) / (2.0f * a);
        float t2 = (-b + sqrtD) / (2.0f * a);

        // 我々は線分上で最小の t を欲しい。
        // t1 <= t2
        float candidateT = 1.0f + 1e-6f;

        const float T_EPS = 1e-5f;
        // If start is inside (t1 < 0 < t2), treat contact as t=0 (already overlapping)
        if (t1 < 0.0f && t2 >= 0.0f) {
            candidateT = 0.0f;
        } else {
            if (t1 >= -T_EPS && t1 <= 1.0f + T_EPS) candidateT = std::max(0.0f, std::min(1.0f, t1));
            else if (t2 >= -T_EPS && t2 <= 1.0f + T_EPS) candidateT = std::max(0.0f, std::min(1.0f, t2));
        }

        if (candidateT <= 1.0f + T_EPS) {
            if (candidateT < bestT) {
                bestT = candidateT;
                float clampedT = std::max(0.0f, std::min(1.0f, candidateT));
                bestPos = Position(start.getX() + dx * clampedT, start.getY() + dy * clampedT);
                bestUnit = other;
                found = true;
            }
        }
    }

    if (found) {
        outContactPosition = bestPos;
        outContactUnit = bestUnit;
        return true;
    }

    outContactUnit = nullptr;
    return false;
}
