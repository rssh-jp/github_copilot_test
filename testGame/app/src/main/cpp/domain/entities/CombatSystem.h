#ifndef SIMULATION_GAME_COMBAT_SYSTEM_H
#define SIMULATION_GAME_COMBAT_SYSTEM_H

#include "UnitEntity.h"
#include <memory>
#include <vector>
#include <random>
#include <algorithm>

/**
 * @brief 戦闘結果を表す構造体
 */
struct CombatResult {
    bool attackerWon;           // 攻撃者が勝利したか
    int damageDealt;           // 与えたダメージ
    int damageReceived;        // 受けたダメージ
    bool attackerDied;         // 攻撃者が死亡したか
    bool defenderDied;         // 防御者が死亡したか
    
    CombatResult() : attackerWon(false), damageDealt(0), damageReceived(0), 
                    attackerDied(false), defenderDied(false) {}
};

/**
 * @brief 戦闘システムエンティティ
 * 
 * 設計方針：
 * - 戦闘に関するビジネスルールを集約
 * - ダメージ計算、戦闘結果判定のロジックを含む
 * - ランダム要素を扱う
 * - 戦闘ログや統計情報を管理
 */
class CombatSystem {
public:
    /**
     * @brief コンストラクタ
     * @param seed ランダムシード（テスト時の再現性のため）
     */
    explicit CombatSystem(unsigned int seed = std::random_device{}()) 
        : randomEngine_(seed) {}
    
    /**
     * @brief ユニット同士の戦闘を実行
     * @param attacker 攻撃側ユニット
     * @param defender 防御側ユニット
     * @return 戦闘結果
     */
    CombatResult executeCombat(UnitEntity& attacker, UnitEntity& defender) {
        CombatResult result;
        
        // 事前条件チェック
        if (!attacker.canAttack() || !defender.isAlive()) {
            return result; // デフォルト結果（何も起こらない）
        }
        
        if (!attacker.isInAttackRange(defender)) {
            return result; // 攻撃範囲外
        }
        
        // 戦闘状態に移行
        attacker.enterCombat();
        defender.enterCombat();
        
        // ダメージ計算
        int attackerDamage = calculateDamage(attacker.getStats(), defender.getStats());
        int defenderDamage = calculateCounterDamage(defender.getStats(), attacker.getStats());
        
        result.damageDealt = attackerDamage;
        result.damageReceived = defenderDamage;
        
        // ダメージ適用
        bool defenderSurvived = defender.takeDamage(attackerDamage);
        bool attackerSurvived = attacker.takeDamage(defenderDamage);
        
        result.defenderDied = !defenderSurvived;
        result.attackerDied = !attackerSurvived;
        
        // 勝敗判定
        if (!defenderSurvived && attackerSurvived) {
            result.attackerWon = true;
        } else if (defenderSurvived && !attackerSurvived) {
            result.attackerWon = false;
        } else if (!defenderSurvived && !attackerSurvived) {
            // 相討ち - ダメージ量で判定
            result.attackerWon = attackerDamage >= defenderDamage;
        } else {
            // 両方生存 - ダメージ量で判定
            result.attackerWon = attackerDamage > defenderDamage;
        }
        
        // 戦闘終了処理
        if (attacker.isAlive()) {
            attacker.exitCombat();
        }
        if (defender.isAlive()) {
            defender.exitCombat();
        }
        
        return result;
    }
    
    /**
     * @brief 攻撃範囲内の敵を検索
     * @param attacker 攻撃者
     * @param potentialTargets 攻撃対象候補のリスト
     * @return 攻撃範囲内の敵のリスト
     */
    std::vector<std::shared_ptr<UnitEntity>> findTargetsInRange(
        const UnitEntity& attacker,
        const std::vector<std::shared_ptr<UnitEntity>>& potentialTargets) const {
        
        std::vector<std::shared_ptr<UnitEntity>> targets;
        
        for (const auto& target : potentialTargets) {
            if (target && target->isAlive() && 
                target->getId() != attacker.getId() &&
                attacker.isInAttackRange(*target)) {
                targets.push_back(target);
            }
        }
        
        return targets;
    }
    
    /**
     * @brief 最も近い敵を選択
     * @param attacker 攻撃者
     * @param targets 攻撃対象候補
     * @return 最も近い敵（見つからない場合はnullptr）
     */
    std::shared_ptr<UnitEntity> selectNearestTarget(
        const UnitEntity& attacker,
        const std::vector<std::shared_ptr<UnitEntity>>& targets) const {
        
        if (targets.empty()) {
            return nullptr;
        }
        
        std::shared_ptr<UnitEntity> nearest = targets[0];
        float nearestDistance = attacker.getPosition().distanceTo(nearest->getPosition());
        
        for (size_t i = 1; i < targets.size(); ++i) {
            float distance = attacker.getPosition().distanceTo(targets[i]->getPosition());
            if (distance < nearestDistance) {
                nearest = targets[i];
                nearestDistance = distance;
            }
        }
        
        return nearest;
    }
    
    /**
     * @brief ダメージ計算（攻撃側）
     * @param attackerStats 攻撃者のステータス
     * @param defenderStats 防御者のステータス
     * @return 計算されたダメージ
     */
    int calculateDamage(const UnitStats& attackerStats, const UnitStats& defenderStats) const {
        // 基本ダメージ
        int baseDamage = attackerStats.getAttackPower();
        
        // ランダム要素（80%～120%）
        std::uniform_real_distribution<float> randomFactor(0.8f, 1.2f);
        float multiplier = randomFactor(randomEngine_);
        
        int finalDamage = static_cast<int>(baseDamage * multiplier);
        
        // 最低1ダメージは保証
        return std::max(1, finalDamage);
    }
    
    /**
     * @brief 反撃ダメージ計算
     * @param defenderStats 防御者（反撃する側）のステータス
     * @param attackerStats 攻撃者のステータス
     * @return 反撃ダメージ
     */
    int calculateCounterDamage(const UnitStats& defenderStats, const UnitStats& attackerStats) const {
        // 反撃は基本攻撃力の50%
        int baseDamage = defenderStats.getAttackPower() / 2;
        
        // ランダム要素（70%～100%）
        std::uniform_real_distribution<float> randomFactor(0.7f, 1.0f);
        float multiplier = randomFactor(randomEngine_);
        
        int finalDamage = static_cast<int>(baseDamage * multiplier);
        
        // 最低0ダメージ（反撃しない場合もある）
        return std::max(0, finalDamage);
    }
    
    /**
     * @brief クリティカルヒット判定
     * @param attackerStats 攻撃者のステータス
     * @return クリティカルヒットかどうか
     */
    bool isCriticalHit(const UnitStats& attackerStats) const {
        // 基本5%のクリティカル率
        std::uniform_real_distribution<float> critChance(0.0f, 1.0f);
        return critChance(randomEngine_) < 0.05f;
    }
    
    /**
     * @brief 戦闘統計情報をリセット
     */
    void resetStatistics() {
        totalCombats_ = 0;
        totalDamageDealt_ = 0;
        totalUnitsKilled_ = 0;
    }
    
    /**
     * @brief 戦闘統計情報を取得
     */
    struct CombatStatistics {
        int totalCombats;
        int totalDamageDealt;
        int totalUnitsKilled;
    };
    
    CombatStatistics getStatistics() const {
        return {totalCombats_, totalDamageDealt_, totalUnitsKilled_};
    }

private:
    mutable std::mt19937 randomEngine_;  // ランダム数生成器
    int totalCombats_ = 0;               // 戦闘回数統計
    int totalDamageDealt_ = 0;           // 総ダメージ統計
    int totalUnitsKilled_ = 0;           // 撃破数統計
    
    /**
     * @brief 戦闘統計を更新
     */
    void updateStatistics(const CombatResult& result) {
        totalCombats_++;
        totalDamageDealt_ += result.damageDealt;
        if (result.defenderDied) {
            totalUnitsKilled_++;
        }
    }
};

#endif // SIMULATION_GAME_COMBAT_SYSTEM_H
