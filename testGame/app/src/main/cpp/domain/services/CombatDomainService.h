#ifndef SIMULATION_GAME_COMBAT_DOMAIN_SERVICE_H
#define SIMULATION_GAME_COMBAT_DOMAIN_SERVICE_H

#include "../entities/UnitEntity.h"
#include "../value_objects/Position.h"
#include <memory>
#include <random>

/**
 * @brief 戦闘に関するドメインロジックを提供するサービス
 * 
 * ドメインサービスとして、複数のエンティティ間の複雑な計算や
 * ビジネスルールを扱います。
 * 
 * 責任:
 * - ダメージ計算
 * - 攻撃範囲判定
 * - 戦闘結果の決定
 */
class CombatDomainService {
public:
    /**
     * @brief 攻撃力をmin-max範囲でランダム取得
     */
    static int getRandomAttackPower(const UnitStats& stats);
    /**
     * @brief 戦闘結果を表す構造体
     */
    struct CombatResult {
        int damageDealt;        // 与えたダメージ
        bool targetKilled;      // 相手を倒したか
        bool attackerKilled;    // 反撃で倒されたか
        
        CombatResult() : damageDealt(0), targetKilled(false), attackerKilled(false) {}
        CombatResult(int damage, bool killed, bool counterKilled = false) 
            : damageDealt(damage), targetKilled(killed), attackerKilled(counterKilled) {}
    };

    /**
     * @brief 2体のユニット間で戦闘を実行
     * @param attacker 攻撃側ユニット
     * @param target 被攻撃側ユニット
     * @return 戦闘結果
     */
    static CombatResult executeCombat(UnitEntity& attacker, UnitEntity& target);

    /**
     * @brief ダメージを計算
     * @param attackerStats 攻撃側のステータス
     * @param targetStats 防御側のステータス（将来の防御力実装用）
     * @return 与えるダメージ
     */
    static int calculateDamage(const UnitStats& attackerStats, const UnitStats& targetStats);

    /**
     * @brief 攻撃範囲内かどうかを判定
     * @param attacker 攻撃側ユニット
     * @param target 対象ユニット
     * @return 攻撃範囲内かどうか
     */
    static bool isInAttackRange(const UnitEntity& attacker, const UnitEntity& target);

    /**
     * @brief 衝突半径を取得
     * @return ユニットの衝突半径
     */
    static constexpr float getCollisionRadius() {
        return 0.1f; // ユニットの衝突半径
    }

    /**
     * @brief 2つのユニットが衝突しているかを判定
     * @param unit1 ユニット1
     * @param unit2 ユニット2
     * @return 衝突しているかどうか
     */
    static bool isColliding(const UnitEntity& unit1, const UnitEntity& unit2);

private:
    static std::random_device rd_;
    static std::mt19937 gen_;
    static std::uniform_real_distribution<float> dis_;
};

#endif // SIMULATION_GAME_COMBAT_DOMAIN_SERVICE_H
