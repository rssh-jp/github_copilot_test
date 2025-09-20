#ifndef SIMULATION_GAME_COLLISION_DOMAIN_SERVICE_H
#define SIMULATION_GAME_COLLISION_DOMAIN_SERVICE_H

#include "../value_objects/Position.h"
#include <vector>
#include <memory>

// Forward declaration to avoid circular includes
class UnitEntity;
#include "../value_objects/Position.h"
#include <vector>
#include <memory>

/**
 * @brief 衝突検知に関するドメインロジックを提供するサービス
 * 
 * 責任:
 * - ユニット間の衝突判定
 * - 移動時の衝突回避パス計算
 * - 有効な位置の判定
 */
class CollisionDomainService {
public:
    /**
     * @brief 移動可能かどうかを判定
     * @param unit 移動するユニット
     * @param targetPosition 移動先の位置
     * @param allUnits 全ユニットのリスト（自分以外）
     * @return 移動可能かどうか
     */
    static bool canMoveTo(
        const UnitEntity& unit, 
        const Position& targetPosition,
        const std::vector<std::shared_ptr<UnitEntity>>& allUnits
    );

    /**
     * @brief 衝突回避した移動先を計算
     * @param unit 移動するユニット
     * @param targetPosition 理想の移動先
     * @param allUnits 全ユニットのリスト
     * @return 実際の移動先位置
     */
    static Position calculateAvoidancePosition(
        const UnitEntity& unit,
        const Position& targetPosition,
        const std::vector<std::shared_ptr<UnitEntity>>& allUnits
    );

    /**
     * @brief 指定位置に他のユニットがいるかチェック
     * @param position チェックする位置
     * @param allUnits 全ユニットのリスト
     * @param excludeUnit 除外するユニット（自分自身など）
     * @return 衝突するユニットがあるかどうか
     */
    static bool hasCollisionAt(
        const Position& position,
        const std::vector<std::shared_ptr<UnitEntity>>& allUnits,
        const UnitEntity* excludeUnit = nullptr,
        float movingRadius = 0.0f
    );

    /**
     * @brief 2点間の線分上に衝突するユニットがいるかチェック
     * @param start 開始位置
     * @param end 終了位置
     * @param allUnits 全ユニットのリスト
     * @param excludeUnit 除外するユニット
     * @return 経路上に衝突があるかどうか
     */
    static bool hasCollisionOnPath(
        const Position& start,
        const Position& end,
        const std::vector<std::shared_ptr<UnitEntity>>& allUnits,
        const UnitEntity* excludeUnit = nullptr,
        float movingRadius = 0.0f
    );

    /**
     * @brief 線分(start->end)上で最初に接触する点を計算する。
     * @param start 開始位置
     * @param end 終了位置
     * @param allUnits 全ユニットリスト
     * @param outContactPosition 最初に接触する位置を返す（見つからない場合は不変）
     * @param outContactUnit 接触したユニットへのポインタ（見つからない場合は nullptr）
     * @param excludeUnit 除外ユニット
     * @param movingRadius 移動ユニットの半径（合算して判定）
     * @return 接触が見つかった場合 true を返す
     */
    static bool findFirstContactOnPath(
        const Position& start,
        const Position& end,
        const std::vector<std::shared_ptr<UnitEntity>>& allUnits,
        Position& outContactPosition,
        const UnitEntity*& outContactUnit,
        const UnitEntity* excludeUnit = nullptr,
        float movingRadius = 0.0f
    );

    /**
     * @brief 衝突半径を取得
     */
    static constexpr float getCollisionRadius() {
        return 0.1f; // ユニットの衝突半径
    }

private:
    /**
     * @brief 点と円の衝突判定
     */
    static bool isPointInCircle(
        const Position& point,
        const Position& circleCenter,
        float radius
    );

    /**
     * @brief 線分と円の衝突判定
     */
    static bool isLineSegmentIntersectingCircle(
        const Position& lineStart,
        const Position& lineEnd,
        const Position& circleCenter,
        float radius
    );
};

#endif // SIMULATION_GAME_COLLISION_DOMAIN_SERVICE_H
