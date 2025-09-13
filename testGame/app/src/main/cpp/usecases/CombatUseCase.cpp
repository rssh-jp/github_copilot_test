#include "CombatUseCase.h"
#include <algorithm>

CombatUseCase::CombatUseCase(std::vector<std::shared_ptr<UnitEntity>>& units)
    : units_(units) {
}

void CombatUseCase::setCombatEventCallback(CombatEventCallback callback) {
    combatEventCallback_ = callback;
}

void CombatUseCase::executeAutoCombat() {
    // 全ユニットをチェックして自動戦闘を実行
    for (auto& unit : units_) {
        if (unit->getStats().getCurrentHp() <= 0) {
            continue; // 死亡ユニットはスキップ
        }

        // 攻撃範囲内の敵を検索
        auto target = findTargetInRange(*unit);
        if (target && target->getStats().getCurrentHp() > 0) {
            // 戦闘実行
            auto result = CombatDomainService::executeCombat(*unit, *target);
            
            // イベント通知
            if (combatEventCallback_) {
                combatEventCallback_(*unit, *target, result);
            }
        }
    }
}

bool CombatUseCase::executeAttack(int attackerId, int targetId) {
    auto attacker = findUnitById(attackerId);
    auto target = findUnitById(targetId);
    
    if (!attacker || !target) {
        return false; // ユニットが見つからない
    }
    
    if (attacker->getStats().getCurrentHp() <= 0 || target->getStats().getCurrentHp() <= 0) {
        return false; // どちらかが死亡している
    }
    
    // 攻撃範囲チェック
    if (!CombatDomainService::isInAttackRange(*attacker, *target)) {
        return false; // 攻撃範囲外
    }
    
    // 戦闘実行
    auto result = CombatDomainService::executeCombat(*attacker, *target);
    
    // イベント通知
    if (combatEventCallback_) {
        combatEventCallback_(*attacker, *target, result);
    }
    
    return true;
}

void CombatUseCase::removeDeadUnits() {
    units_.erase(
        std::remove_if(units_.begin(), units_.end(),
            [](const std::shared_ptr<UnitEntity>& unit) {
                return unit->getStats().getCurrentHp() <= 0;
            }),
        units_.end()
    );
}

size_t CombatUseCase::getAliveUnitsCount() const {
    return std::count_if(units_.begin(), units_.end(),
        [](const std::shared_ptr<UnitEntity>& unit) {
            return unit->getStats().getCurrentHp() > 0;
        });
}

std::shared_ptr<UnitEntity> CombatUseCase::findUnitById(int unitId) {
    auto it = std::find_if(units_.begin(), units_.end(),
        [unitId](const std::shared_ptr<UnitEntity>& unit) {
            return unit->getId() == unitId;
        });
    
    return (it != units_.end()) ? *it : nullptr;
}

std::shared_ptr<UnitEntity> CombatUseCase::findUnitById(int unitId) const {
    auto it = std::find_if(units_.begin(), units_.end(),
        [unitId](const std::shared_ptr<UnitEntity>& unit) {
            return unit->getId() == unitId;
        });
    
    return (it != units_.end()) ? *it : nullptr;
}

std::shared_ptr<UnitEntity> CombatUseCase::findTargetInRange(const UnitEntity& attacker) {
    for (auto& unit : units_) {
        if (unit->getId() == attacker.getId()) {
            continue; // 自分自身は除外
        }
        
        if (unit->getStats().getCurrentHp() <= 0) {
            continue; // 死亡ユニットは除外
        }
        
        if (CombatDomainService::isInAttackRange(attacker, *unit)) {
            return unit;
        }
    }
    
    return nullptr;
}
