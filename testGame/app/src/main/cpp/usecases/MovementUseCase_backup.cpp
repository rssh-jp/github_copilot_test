#include "MovementUseCase.h"
#include <algorithm>

MovementUseCase::MovementUseCase(std::vector<std::shared_ptr<UnitEntity>>& units)
    : units_(units) {
}

void MovementUseCase::setMovementEventCallback(MovementEventCallback callback) {
    movementEventCallback_ = callback;
}

void MovementUseCase::setMovementFailedCallback(MovementFailedCallback callback) {
    movementFailedCallback_ = callback;
}

bool MovementUseCase::moveUnitTo(int unitId, const Position& targetPosition) {
}:shared_ptr<UnitEntity>>& units)
    : units_(units) {
}

void MovementUseCase::setMovementEventCallback(MovementEventCallback callback) {
    movementEventCallback_ = callback;
}

void MovementUseCase::setMovementFailedCallback(MovementFailedCallback callback) {
    movementFailedCallback_ = callback;
}

bool MovementUseCase::moveUnitTo(int unitId, const Position& targetPosition) {
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

    // 衝突回避を考慮した実際の移動先を計算
    Position actualTarget = CollisionDomainService::calculateAvoidancePosition(
        *unit, targetPosition, otherUnits);

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
    for (auto& unit : units_) {
        if (unit->getStats().getCurrentHp() <= 0) {
            continue; // 死亡ユニットは移動しない
        }

        // 移動更新
        unit->updateMovement(deltaTime);
    }
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

    return CollisionDomainService::canMoveTo(*unit, targetPosition, otherUnits);
}

std::shared_ptr<UnitEntity> MovementUseCase::findUnitById(int unitId) {
    auto it = std::find_if(units_.begin(), units_.end(),
        [unitId](const std::shared_ptr<UnitEntity>& unit) {
            return unit->getId() == unitId;
        });
    
    return (it != units_.end()) ? *it : nullptr;
}

const std::shared_ptr<UnitEntity> MovementUseCase::findUnitById(int unitId) const {
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
