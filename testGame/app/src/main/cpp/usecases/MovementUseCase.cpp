#include "MovementUseCase.h"
#include <algorithm>
#include <cmath>
#include "../domain/services/MovementField.h"
#include "../frameworks/android/AndroidOut.h"

MovementUseCase::MovementUseCase(std::vector<std::shared_ptr<UnitEntity>>& units,
    const MovementField* movementField,
    const GameMap* gameMap)
    : units_(units),
      movementField_(movementField),
      gameMap_(gameMap),
      movementEnabled_(true) {
}

void MovementUseCase::setMovementEventCallback(MovementEventCallback callback) {
    movementEventCallback_ = callback;
}

void MovementUseCase::setMovementFailedCallback(MovementFailedCallback callback) {
    movementFailedCallback_ = callback;
}

bool MovementUseCase::moveUnitTo(int unitId, const Position& targetPosition) {
    if (!movementEnabled_) {
        if (movementFailedCallback_) {
            auto unit = findUnitById(unitId);
            if (unit) {
                movementFailedCallback_(*unit, targetPosition, "Unit movement is currently disabled");
            }
        }
        return false;
    }

    auto unit = findUnitById(unitId);
    if (!unit) {
        return false;
    }

    if (unit->getStats().getCurrentHp() <= 0) {
        if (movementFailedCallback_) {
            movementFailedCallback_(*unit, targetPosition, "Unit is dead");
        }
        return false;
    }

    auto otherUnits = getOtherUnits(*unit);

    Position boundedTarget = applyBounds(*unit, targetPosition);
    Position terrainAwareTarget = boundedTarget;
    if (gameMap_) {
        terrainAwareTarget = gameMap_->resolveMovementTarget(unit->getPosition(), boundedTarget,
                                                            unit->getStats().getCollisionRadius());
        if (!gameMap_->isWalkable(terrainAwareTarget, unit->getStats().getCollisionRadius())) {
            if (movementFailedCallback_) {
                movementFailedCallback_(*unit, targetPosition, "Target blocked by terrain");
            }
            return false;
        }
    }

    Position avoidanceTarget = CollisionDomainService::calculateAvoidancePosition(
        *unit, terrainAwareTarget, otherUnits);

    avoidanceTarget = applyBounds(*unit, avoidanceTarget);
    if (gameMap_) {
        avoidanceTarget = gameMap_->resolveMovementTarget(unit->getPosition(), avoidanceTarget,
                                                         unit->getStats().getCollisionRadius());
    }

    Position fromPosition = unit->getPosition();
    const float travelDistance = fromPosition.distanceTo(avoidanceTarget);
    if (travelDistance <= 1e-4f) {
        if (movementFailedCallback_) {
            movementFailedCallback_(*unit, targetPosition, "No viable path found");
        }
        return false;
    }

    unit->setTargetPosition(avoidanceTarget);

    if (movementEventCallback_) {
        movementEventCallback_(*unit, fromPosition, avoidanceTarget);
    }

    if (gameMap_ && targetPosition.distanceTo(avoidanceTarget) > 0.05f) {
        aout << "MovementUseCase: adjusted target due to terrain from ("
             << targetPosition.getX() << ", " << targetPosition.getY() << ") to ("
             << avoidanceTarget.getX() << ", " << avoidanceTarget.getY() << ")" << std::endl;
    }

    return true;
}

void MovementUseCase::updateMovements(float deltaTime) {
    for (auto& unit : units_) {
        if (!unit || unit->getStats().getCurrentHp() <= 0) {
            continue;
        }
        if (unit->getState() != UnitState::MOVING) {
            continue;
        }

        unit->setTargetPosition(applyBounds(*unit, unit->getTargetPosition()));

        Position currentPos = unit->getPosition();
        Position nextPos = calculateNextPosition(*unit, deltaTime);

        auto otherUnits = getOtherUnits(*unit);
        float movingRadius = unit->getStats().getCollisionRadius();
        Position contactPos = currentPos;
        const UnitEntity* contactUnit = nullptr;
        bool hasContact = CollisionDomainService::findFirstContactOnPath(
            currentPos, nextPos, otherUnits, contactPos, contactUnit, unit.get(), movingRadius);

        if (hasContact) {
            constexpr float kEpsT = 1e-3f;
            constexpr float kBackoffDistance = 0.02f;

            float segmentLen = currentPos.distanceTo(nextPos);
            float t = 0.0f;
            if (segmentLen > 1e-6f) {
                t = currentPos.distanceTo(contactPos) / segmentLen;
            }

            float dirX = 0.0f;
            float dirY = 0.0f;
            if (segmentLen > 1e-6f) {
                dirX = (nextPos.getX() - currentPos.getX()) / segmentLen;
                dirY = (nextPos.getY() - currentPos.getY()) / segmentLen;
            }

            bool currentlyOverlapping = CollisionDomainService::hasCollisionAt(
                currentPos, otherUnits, unit.get(), movingRadius);

            if (currentlyOverlapping) {
                Position backPos = currentPos.moveBy(-dirX * kBackoffDistance, -dirY * kBackoffDistance);
                backPos = resolveTerrainConstraints(*unit, currentPos, backPos);
                if (CollisionDomainService::hasCollisionAt(backPos, otherUnits, unit.get(), movingRadius)) {
                    Position safePos = CollisionDomainService::calculateAvoidancePosition(*unit, currentPos, otherUnits);
                    safePos = resolveTerrainConstraints(*unit, currentPos, safePos);
                    unit->setTargetPosition(safePos);
                    unit->updatePosition(safePos);
                } else {
                    unit->setTargetPosition(backPos);
                    unit->updatePosition(backPos);
                }
            } else if (t <= kEpsT) {
                Position stopBefore = contactPos.moveBy(-dirX * kBackoffDistance, -dirY * kBackoffDistance);
                stopBefore = resolveTerrainConstraints(*unit, currentPos, stopBefore);
                unit->setTargetPosition(stopBefore);
                unit->updatePosition(stopBefore);
            } else {
                Position adjustedContact = resolveTerrainConstraints(*unit, currentPos, contactPos);
                unit->setTargetPosition(adjustedContact);
                unit->updatePosition(adjustedContact);
            }
        } else {
            unit->updatePosition(nextPos);
        }
    }
}

Position MovementUseCase::calculateNextPosition(const UnitEntity& unit, float deltaTime) const {
    Position currentPos = unit.getPosition();
    Position targetPos = applyBounds(unit, unit.getTargetPosition());

    if (gameMap_) {
        targetPos = gameMap_->resolveMovementTarget(currentPos, targetPos,
                                                    unit.getStats().getCollisionRadius());
    }

    const float distance = currentPos.distanceTo(targetPos);
    if (distance <= 0.001f || deltaTime <= 0.0f) {
        return targetPos;
    }

    // 地形ごとの移動速度補正を反映（森・山などは multiplier < 1.0）
    const float speedMultiplier = terrainSpeedMultiplier(currentPos);
    const float effectiveSpeed = unit.getStats().getMoveSpeed() * speedMultiplier;
    if (effectiveSpeed <= 0.0f) {
        return currentPos;
    }

    const float maxDistance = std::max(0.0f, effectiveSpeed * deltaTime);
    if (maxDistance <= 0.0f) {
        return currentPos;
    }

    // travelDistance は今回のフレームで進める距離（地形倍率を反映済み）
    const float travelDistance = std::min(distance, maxDistance);
    if (travelDistance <= 0.0f) {
        return currentPos;
    }

    if (travelDistance >= distance - 1e-5f) {
        return targetPos;
    }

    const float stepRatio = travelDistance / distance;
    Position candidate(
        currentPos.getX() + (targetPos.getX() - currentPos.getX()) * stepRatio,
        currentPos.getY() + (targetPos.getY() - currentPos.getY()) * stepRatio);

    if (gameMap_) {
        candidate = gameMap_->resolveMovementTarget(currentPos, candidate,
                                                    unit.getStats().getCollisionRadius());
    }

    Position boundedCandidate = applyBounds(unit, candidate);

    // 最終的にユニットへ適用する移動先をログ出力して検証できるようにする
    aout << "MovementUseCase::calculateNextPosition unit=" << unit.getId()
        << " currentPos=(" << currentPos.getX() << ", " << currentPos.getY() << ")"
        << " targetPos=(" << targetPos.getX() << ", " << targetPos.getY() << ")"
        << " effectiveSpeed=" << effectiveSpeed << " deltaTime=" << deltaTime
    << " travelDistance=" << travelDistance
        << " result=(" << boundedCandidate.getX() << ", " << boundedCandidate.getY() << ")"
        << std::endl;

    return boundedCandidate;
}

size_t MovementUseCase::getMovingUnitsCount() const {
    return std::count_if(units_.begin(), units_.end(),
        [](const std::shared_ptr<UnitEntity>& unit) {
            return unit->getStats().getCurrentHp() > 0 && unit->getState() == UnitState::MOVING;
        });
}

bool MovementUseCase::canMoveToPosition(int unitId, const Position& targetPosition) const {
    auto unit = findUnitById(unitId);
    if (!unit) {
        return false;
    }

    if (unit->getStats().getCurrentHp() <= 0) {
        return false; // 死亡ユニットは移動できない
    }

    auto otherUnits = getOtherUnits(*unit);

    Position boundedTarget = applyBounds(*unit, targetPosition);
    if (gameMap_) {
        boundedTarget = gameMap_->resolveMovementTarget(unit->getPosition(), boundedTarget,
                                                        unit->getStats().getCollisionRadius());
        if (!gameMap_->isWalkable(boundedTarget, unit->getStats().getCollisionRadius())) {
            return false;
        }
    }

    return CollisionDomainService::canMoveTo(*unit, boundedTarget, otherUnits);
}

std::shared_ptr<UnitEntity> MovementUseCase::findUnitById(int unitId) {
    auto it = std::find_if(units_.begin(), units_.end(),
        [unitId](const std::shared_ptr<UnitEntity>& unit) {
            return unit->getId() == unitId;
        });
    
    return (it != units_.end()) ? *it : nullptr;
}

std::shared_ptr<UnitEntity> MovementUseCase::findUnitById(int unitId) const {
    auto it = std::find_if(units_.begin(), units_.end(),
        [unitId](const std::shared_ptr<UnitEntity>& unit) {
            return unit->getId() == unitId;
        });
    
    return (it != units_.end()) ? *it : nullptr;
}

std::vector<std::shared_ptr<UnitEntity>> MovementUseCase::getOtherUnits(const UnitEntity& excludeUnit) const {
    std::vector<std::shared_ptr<UnitEntity>> otherUnits;
    
    for (const auto& unit : units_) {
        if (unit->getId() != excludeUnit.getId() && unit->getStats().getCurrentHp() > 0) {
            otherUnits.push_back(unit);
        }
    }
    
    return otherUnits;
}

void MovementUseCase::setMovementEnabled(bool enabled, const std::string& reason) {
    if (movementEnabled_ != enabled) {
        movementEnabled_ = enabled;
        // ログ出力は AndroidOut を使用しないため、コンソール出力はしない
        // 必要に応じて movementFailedCallback_ 等で通知
    }
}

bool MovementUseCase::isMovementEnabled() const {
    return movementEnabled_;
}

Position MovementUseCase::applyBounds(const UnitEntity& unit, const Position& desired) const {
    Position bounded = desired;
    if (movementField_) {
        bounded = movementField_->snapInside(bounded);
    }
    if (gameMap_) {
        bounded = gameMap_->clampInside(bounded, unit.getStats().getCollisionRadius());
    }
    return bounded;
}

float MovementUseCase::terrainSpeedMultiplier(const Position& position) const {
    if (!gameMap_) {
        return 1.0f;
    }
    // GameMap 側は TerrainType の定義に基づき 0.0〜1.0 の係数を返す
    return std::max(0.0f, gameMap_->getMovementMultiplier(position));
}

Position MovementUseCase::resolveTerrainConstraints(const UnitEntity& unit,
                                                     const Position& start,
                                                     const Position& desired) const {
    Position bounded = applyBounds(unit, desired);
    if (gameMap_) {
        return gameMap_->resolveMovementTarget(start, bounded, unit.getStats().getCollisionRadius());
    }
    return bounded;
}
