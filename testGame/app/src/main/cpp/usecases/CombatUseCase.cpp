#include "CombatUseCase.h"

/*
 * CombatUseCase.cpp
 *
 * Responsibilities:
 * - Orchestrate per-frame or triggered combat behavior for units (auto-attack and manual attack).
 * - Respect domain invariants (attack cooldowns, faction checks, movement state) before invoking
 *   domain combat helpers.
 *
 * Notes:
 * - This module should not perform rendering or platform IO. Use callbacks to notify higher layers
 *   about combat events (for UI / audio / effects).
 */
#include <algorithm>
#include <chrono>

CombatUseCase::CombatUseCase(std::vector<std::shared_ptr<UnitEntity>>& units)
    : units_(units) {
}

void CombatUseCase::setCombatEventCallback(CombatEventCallback callback) {
    combatEventCallback_ = callback;
}

void CombatUseCase::executeAutoCombat() {
    // 自動戦闘のエントリポイント: フレームごとに呼ばれ、攻撃可能なユニットを探して攻撃を実行します。
    // 現在時刻を秒で取得
    auto now = std::chrono::high_resolution_clock::now();
    float nowSec = std::chrono::duration<float>(now.time_since_epoch()).count();

    // 全ユニットをチェックして自動戦闘を実行
    for (auto& unit : units_) {
        if (unit->getStats().getCurrentHp() <= 0) {
            continue; // 死亡ユニットはスキップ
        }

    // Find a target inside effective range (considering collision radii)
        auto target = findTargetInRange(*unit);
        if (!(target && target->getStats().getCurrentHp() > 0)) {
            continue;
        }

        // 攻撃可能かどうか（攻撃速度によるクールダウンを尊重）
        // Do not attack if the unit is currently moving; require it to be idle or already in combat.
        if (unit->getState() == UnitState::MOVING) {
            continue;
        }

        if (!unit->canAttack(nowSec)) {
            continue;
        }

    // Execute combat via domain service. The domain service performs the damage application
    // and any state changes (enter/exit combat).
        auto result = CombatDomainService::executeCombat(*unit, *target);

        // 攻撃時刻の更新（ユースケース側で管理）
        unit->setLastAttackTime(nowSec);

        // イベント通知
        if (combatEventCallback_) {
            combatEventCallback_(*unit, *target, result);
        }
    }
}

bool CombatUseCase::executeAttack(int attackerId, int targetId) {
    // 手動攻撃の実行: attackerId が targetId を攻撃するよう要求する。
    // 関連する検証（存在チェック、生存確認、射程、同陣営判定等）を行い、問題なければ domain に委譲して戦闘を実行します。
    auto attacker = findUnitById(attackerId);
    auto target = findUnitById(targetId);
    
    if (!attacker || !target) {
        return false; // ユニットが見つからない
    }
    
    if (attacker->getStats().getCurrentHp() <= 0 || target->getStats().getCurrentHp() <= 0) {
        return false; // どちらかが死亡している
    }
    
    // 攻撃範囲チェック
    // Ensure attacker is not moving (must have completed movement) before allowing manual attack
    if (attacker->getState() == UnitState::MOVING) {
        return false;
    }

    if (!CombatDomainService::isInAttackRange(*attacker, *target)) {
        return false; // 攻撃範囲外
    }
    
    // 同陣営への攻撃は許可しない
    if (attacker->getFaction() == target->getFaction()) {
        return false;
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
    // 攻撃対象を射程内から探索して最初に見つかった敵ユニットを返す（衝突半径は domain 側で考慮）
    for (auto& unit : units_) {
        if (unit->getId() == attacker.getId()) {
            continue; // 自分自身は除外
        }
        
        if (unit->getStats().getCurrentHp() <= 0) {
            continue; // 死亡ユニットは除外
        }
        
        // 同陣営は攻撃対象外
        if (unit->getFaction() == attacker.getFaction()) {
            continue;
        }
        
        if (CombatDomainService::isInAttackRange(attacker, *unit)) {
            return unit;
        }
    }
    
    return nullptr;
}
