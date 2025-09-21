#include "MovementUseCase.h"
#include <algorithm>
#include <cmath>
#include "../domain/services/MovementField.h"

MovementUseCase::MovementUseCase(std::vector<std::shared_ptr<UnitEntity>>& units,
    const MovementField* movementField)
    : units_(units), movementField_(movementField) {
}

void MovementUseCase::setMovementEventCallback(MovementEventCallback callback) {
    movementEventCallback_ = callback;
}

void MovementUseCase::setMovementFailedCallback(MovementFailedCallback callback) {
    movementFailedCallback_ = callback;
}

bool MovementUseCase::moveUnitTo(int unitId, const Position& targetPosition) {
    /**
     * Move the specified unit towards targetPosition.
     * Behavior:
     * - Validates unit existence and liveliness.
     * - If a MovementField is configured, snaps the target inside the field and validates walkability.
     * - Calculates an avoidance-aware actual target using CollisionDomainService and sets the unit's
     *   targetPosition accordingly.
     * - Emits movementEventCallback_ with the from/to positions when movement is started.
     */
    auto unit = findUnitById(unitId);
    if (!unit) {
        return false; // ユニットが見つからない
    }

    if (unit->getStats().getCurrentHp() <= 0) {
        if (movementFailedCallback_) {
            movementFailedCallback_(*unit, targetPosition, "Unit is dead");
        }
        return false; // 死亡ユニットは移動できない
    }

    // 他のユニットのリストを取得
    auto otherUnits = getOtherUnits(*unit);

    // MovementField がある場合はターゲットをフィールド内にスナップし、歩行可能か確認
    Position desiredTarget = targetPosition;
    if (movementField_) {
        desiredTarget = movementField_->snapInside(desiredTarget);
        if (!movementField_->isWalkable(desiredTarget, unit->getStats().getCollisionRadius())) {
            if (movementFailedCallback_) {
                movementFailedCallback_(*unit, targetPosition, "Target not walkable on field");
            }
            return false;
        }
    }

    // 衝突回避を考慮した実際の移動先を計算
    Position actualTarget = CollisionDomainService::calculateAvoidancePosition(
        *unit, desiredTarget, otherUnits);

    // 移動前の位置を記録
    Position fromPosition = unit->getPosition();

    // 移動を実行
    unit->setTargetPosition(actualTarget);

    // イベント通知
    if (movementEventCallback_) {
        movementEventCallback_(*unit, fromPosition, actualTarget);
    }

    return true;
}

void MovementUseCase::updateMovements(float deltaTime) {
    /**
     * Update movement for all units.
     *
     *  - 各ユニットについて次フレームでの位置を予測し、他ユニットとの接触を検出します。
     *  - 接触が予測される場合はバックオフや停止処理を行い、衝突を回避します。
     *  - 問題がなければ UnitEntity::updateMovement(deltaTime) を呼んで実際の移動を行います。
     */
    for (auto& unit : units_) {
        if (unit->getStats().getCurrentHp() <= 0) {
            continue; // 死亡ユニットは移動しない
        }

        if (unit->getState() != UnitState::MOVING) continue;

    // 予測される次の位置を計算
    Position currentPos = unit->getPosition();
    Position nextPos = calculateNextPosition(*unit, deltaTime);

        // 他ユニットを取得
        auto otherUnits = getOtherUnits(*unit);

        // 衝突判定: currentPos -> nextPos の線分上で最初に接触する点を探す
        float movingRadius = unit->getStats().getCollisionRadius();
        Position contactPos = currentPos; // out param 初期値
        const UnitEntity* contactUnit = nullptr;
        bool hasContact = CollisionDomainService::findFirstContactOnPath(
            currentPos, nextPos, otherUnits, contactPos, contactUnit, unit.get(), movingRadius);

        if (hasContact) {
            // 接触点が見つかった場合、接触点が現在位置に極めて近い（再検出でスタックする）
            // か、現在位置が既にめり込んでいる場合は少し後退させる（バックオフ）
            const float EPS_T = 1e-3f; // 接触点が現在位置に近いとみなす閾値（割合）
            const float BACKOFF_DISTANCE = 0.02f; // バックオフ量（ワールド単位）

            // 線分長と接触位置の t を計算
            float segmentLen = currentPos.distanceTo(nextPos);
            float t = 0.0f;
            if (segmentLen > 1e-6f) {
                t = currentPos.distanceTo(contactPos) / segmentLen;
            } else {
                t = 0.0f;
            }

            // currentPos が既に他ユニットと衝突しているかを確認
            bool currentlyOverlapping = CollisionDomainService::hasCollisionAt(currentPos, otherUnits, unit.get(), movingRadius);

            if (currentlyOverlapping) {
                // 既にめり込んでいる: 目標方向の逆方向に少し戻す
                float dirX = 0.0f, dirY = 0.0f;
                if (segmentLen > 1e-6f) {
                    dirX = (nextPos.getX() - currentPos.getX()) / segmentLen;
                    dirY = (nextPos.getY() - currentPos.getY()) / segmentLen;
                }
                // 逆方向へ少し移動
                Position backPos = currentPos.moveBy(-dirX * BACKOFF_DISTANCE, -dirY * BACKOFF_DISTANCE);
                // バックオフ後も衝突するなら calculateAvoidancePosition にフォールバック
                if (CollisionDomainService::hasCollisionAt(backPos, otherUnits, unit.get(), movingRadius)) {
                    Position safePos = CollisionDomainService::calculateAvoidancePosition(*unit, currentPos, otherUnits);
                    unit->setTargetPosition(safePos);
                    unit->updatePosition(safePos);
                } else {
                    unit->setTargetPosition(backPos);
                    unit->updatePosition(backPos);
                }
            } else if (t <= EPS_T) {
                // 接触点が現在位置に非常に近い: 接触点の手前へ少しずらして停止させる
                float dirX = 0.0f, dirY = 0.0f;
                if (segmentLen > 1e-6f) {
                    dirX = (nextPos.getX() - currentPos.getX()) / segmentLen;
                    dirY = (nextPos.getY() - currentPos.getY()) / segmentLen;
                }
                Position stopBefore = contactPos.moveBy(-dirX * BACKOFF_DISTANCE, -dirY * BACKOFF_DISTANCE);
                unit->setTargetPosition(stopBefore);
                unit->updatePosition(stopBefore);
            } else {
                // 通常: 接触点で停止
                unit->setTargetPosition(contactPos);
                unit->updatePosition(contactPos);
            }
        } else {
            // 衝突なしなら通常の移動更新
            unit->updateMovement(deltaTime);
        }
    }
}

Position MovementUseCase::calculateNextPosition(const UnitEntity& unit, float deltaTime) const {
    /**
     * Calculate next position for the given unit assuming movement for deltaTime seconds.
     * This is deterministic and side-effect free: it does not mutate the unit.
     */
    Position currentPos = unit.getPosition();
    Position targetPos = unit.getTargetPosition();

    // 移動ベクトル計算
    Position direction = targetPos - currentPos;
    float distance = currentPos.distanceTo(targetPos);

    if (distance <= 0.001f) {
        return targetPos; // 既に到達
    }

    // 最大移動距離
    float maxDistance = unit.getStats().getMoveSpeed() * deltaTime;

    if (distance <= maxDistance) {
        return targetPos; // 一度に到達可能
    }

    // 正規化して移動
    float normalizedX = direction.getX() / distance;
    float normalizedY = direction.getY() / distance;

    return currentPos.moveBy(
        normalizedX * maxDistance,
        normalizedY * maxDistance
    );
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

    // 他のユニットのリストを取得
    auto otherUnits = getOtherUnits(*unit);

    // MovementField がある場合はフィールド上で歩行可能かチェック
    // If a movement field exists, snap and validate walkability first.
    if (movementField_) {
        Position snapped = movementField_->snapInside(targetPosition);
        if (!movementField_->isWalkable(snapped, unit->getStats().getCollisionRadius())) return false;
    }

    // Delegate detailed collision/path checks to CollisionDomainService.
    return CollisionDomainService::canMoveTo(*unit, targetPosition, otherUnits);
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
