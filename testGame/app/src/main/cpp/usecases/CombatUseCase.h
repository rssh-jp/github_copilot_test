#ifndef SIMULATION_GAME_COMBAT_USECASE_H
#define SIMULATION_GAME_COMBAT_USECASE_H

/*
 * CombatUseCase.h
 *
 * Public API for higher-level combat orchestration used by game loop / AI.
 *
 * Contract:
 * - Caller provides a reference to the in-memory unit list that this use-case will operate on.
 * - All returned UnitEntity pointers are non-owning references to objects managed elsewhere.
 */

#include "../domain/entities/UnitEntity.h"
#include "../domain/services/CombatDomainService.h"
#include <vector>
#include <memory>
#include <functional>

/**
 * @brief 戦闘に関するユースケース
 * 
 * アプリケーション層として、戦闘のオーケストレーションを行います。
 * ドメインサービスを組み合わせて、複雑な戦闘フローを管理します。
 * 
 * 責任:
 * - 戦闘の実行管理
 * - 戦闘イベントの通知
 * - ユニット管理との連携
 */
class CombatUseCase {
public:
    /**
     * @brief 戦闘イベントのコールバック型
     */
    using CombatEventCallback = std::function<void(
        const UnitEntity& attacker,
        const UnitEntity& target,
        const CombatDomainService::CombatResult& result
    )>;

    /**
     * @brief コンストラクタ
     * @param units 管理するユニットのリスト
     */
    explicit CombatUseCase(std::vector<std::shared_ptr<UnitEntity>>& units);

    /**
     * @brief 戦闘イベントコールバックを設定
     * @param callback 戦闘時に呼ばれるコールバック
     */
    void setCombatEventCallback(CombatEventCallback callback);

    /**
     * @brief 自動戦闘処理を実行
     * 
     * 全ユニットに対して攻撃範囲内の敵ユニットがいるかチェックし、
     * 自動的に戦闘を実行します。
     */
    void executeAutoCombat();

    /**
     * @brief 指定されたユニットで攻撃実行
     * @param attackerId 攻撃するユニットのID
     * @param targetId 攻撃対象のユニットID
     * @return 戦闘が実行されたかどうか
     */
    bool executeAttack(int attackerId, int targetId);

    /**
     * @brief 死亡したユニットを除去
     */
    void removeDeadUnits();

    /**
     * @brief 生存ユニット数を取得
     * @return 生存しているユニットの数
     */
    size_t getAliveUnitsCount() const;

private:
    std::vector<std::shared_ptr<UnitEntity>>& units_;
    CombatEventCallback combatEventCallback_;

    /**
     * @brief ユニットIDからユニットを検索
     * @param unitId 検索するユニットID
     * @return 見つかったユニット（見つからない場合はnullptr）
     */
    std::shared_ptr<UnitEntity> findUnitById(int unitId);
    std::shared_ptr<UnitEntity> findUnitById(int unitId) const;

    /**
     * @brief 攻撃範囲内の敵ユニットを検索
     * @param attacker 攻撃側ユニット
     * @return 攻撃可能な敵ユニット（見つからない場合はnullptr）
     */
    std::shared_ptr<UnitEntity> findTargetInRange(const UnitEntity& attacker);
};

#endif // SIMULATION_GAME_COMBAT_USECASE_H
