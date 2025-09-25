#ifndef SIMULATION_GAME_MOVEMENT_USECASE_H
#define SIMULATION_GAME_MOVEMENT_USECASE_H

#include "../domain/entities/UnitEntity.h"
#include "../domain/services/CollisionDomainService.h"
#include "../domain/value_objects/Position.h"
#include <vector>
#include <memory>
#include <functional>

/**
 * @brief 移動に関するユースケース
 * 
 * ユニットの移動を管理し、衝突回避や経路計算を行います。
 * 
 * 責任:
 * - 衝突を考慮した移動処理
 * - 移動可能性の判定
 * - 移動イベントの通知
 */
class MovementUseCase {
public:
    /**
     * @brief 移動イベントのコールバック型
     */
    using MovementEventCallback = std::function<void(
        const UnitEntity& unit,
        const Position& fromPosition,
        const Position& toPosition
    )>;

    /**
     * @brief 移動失敗イベントのコールバック型
     */
    using MovementFailedCallback = std::function<void(
        const UnitEntity& unit,
        const Position& targetPosition,
        const std::string& reason
    )>;

    /**
     * @brief コンストラクタ
     * @param units 管理するユニットのリスト
     */
    explicit MovementUseCase(std::vector<std::shared_ptr<UnitEntity>>& units,
                             const class MovementField* movementField = nullptr);

    /**
     * @brief 移動イベントコールバックを設定
     */
    void setMovementEventCallback(MovementEventCallback callback);
    void setMovementFailedCallback(MovementFailedCallback callback);

    /**
     * @brief ユニットを指定位置に移動
     * @param unitId 移動するユニットのID
     * @param targetPosition 移動先の位置
     * @return 移動が成功したかどうか
     */
    bool moveUnitTo(int unitId, const Position& targetPosition);

    /**
     * @brief 全ユニットの移動更新処理
     * @param deltaTime フレーム間の経過時間
     */
    void updateMovements(float deltaTime);

    /**
     * @brief 移動中のユニット数を取得
     * @return 移動中のユニット数
     */
    size_t getMovingUnitsCount() const;

    /**
     * @brief 指定位置への移動可能性をチェック
     * @param unitId チェックするユニットのID
     * @param targetPosition チェックする位置
     * @return 移動可能かどうか
     */
    bool canMoveToPosition(int unitId, const Position& targetPosition) const;

    /**
     * @brief ユニット移動を有効化/無効化
     * @param enabled 移動を有効にするかどうか
     * @param reason 無効化する理由（ログ用）
     */
    void setMovementEnabled(bool enabled, const std::string& reason = "");

    /**
     * @brief 現在ユニット移動が有効かどうかを取得
     * @return 移動が有効かどうか
     */
    bool isMovementEnabled() const;

private:
    std::vector<std::shared_ptr<UnitEntity>>& units_;
    const class MovementField* movementField_ = nullptr;
    MovementEventCallback movementEventCallback_;
    MovementFailedCallback movementFailedCallback_;
    
    // 移動制御フラグ
    bool movementEnabled_;

    /**
     * @brief ユニットIDからユニットを検索
     */
    std::shared_ptr<UnitEntity> findUnitById(int unitId);

    /**
     * @brief ユニットIDからユニットを検索（const版）
     */
    std::shared_ptr<UnitEntity> findUnitById(int unitId) const;

    /**
     * @brief 他のユニットのリストを取得（指定ユニット以外）
     */
    std::vector<std::shared_ptr<UnitEntity>> getOtherUnits(const UnitEntity& excludeUnit) const;
    
    /**
     * @brief 指定ユニットの次フレームでの位置を計算
     */
    Position calculateNextPosition(const UnitEntity& unit, float deltaTime) const;
};

#endif // SIMULATION_GAME_MOVEMENT_USECASE_H
