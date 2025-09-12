#ifndef SIMULATION_GAME_MOVE_UNIT_USECASE_H
#define SIMULATION_GAME_MOVE_UNIT_USECASE_H

#include "../interfaces/IUnitRepository.h"
#include "../../domain/entities/UnitEntity.h"
#include "../../domain/value_objects/Position.h"
#include <memory>
#include <vector>

/**
 * @brief ユニット移動の結果
 */
enum class MoveResult {
    SUCCESS,              // 移動成功
    UNIT_NOT_FOUND,      // ユニットが見つからない
    UNIT_CANNOT_MOVE,    // ユニットが移動できない状態
    COLLISION_DETECTED,   // 衝突が検出された
    INVALID_POSITION,     // 無効な移動先
    MOVE_TOO_FAR         // 移動距離が長すぎる
};

/**
 * @brief ユニット移動ユースケース
 * 
 * 設計方針：
 * - 単一責任の原則：ユニット移動に関するビジネスロジックのみ
 * - 依存関係逆転：インターフェースに依存
 * - 衝突検知とルール適用を含む
 */
class MoveUnitUseCase {
public:
    /**
     * @brief コンストラクタ
     * @param unitRepository ユニットリポジトリ
     */
    explicit MoveUnitUseCase(std::shared_ptr<IUnitRepository> unitRepository)
        : unitRepository_(unitRepository), collisionRadius_(1.0f) {}
    
    /**
     * @brief ユニットの移動先を設定
     * @param unitId 移動するユニットのID
     * @param targetPosition 移動先位置
     * @return 移動結果
     */
    MoveResult setTargetPosition(int unitId, const Position& targetPosition) {
        // ユニット取得
        auto unit = unitRepository_->findById(unitId);
        if (!unit) {
            return MoveResult::UNIT_NOT_FOUND;
        }
        
        // 移動可能性チェック
        if (!unit->canMove()) {
            return MoveResult::UNIT_CANNOT_MOVE;
        }
        
        // 移動先設定
        bool success = unit->setTargetPosition(targetPosition);
        if (!success) {
            return MoveResult::UNIT_CANNOT_MOVE;
        }
        
        // リポジトリに保存
        unitRepository_->save(unit);
        
        return MoveResult::SUCCESS;
    }
    
    /**
     * @brief ユニットの位置を更新（フレーム毎の呼び出し）
     * @param unitId 更新するユニットのID
     * @param deltaTime 経過時間
     * @return 移動結果
     */
    MoveResult updatePosition(int unitId, float deltaTime) {
        auto unit = unitRepository_->findById(unitId);
        if (!unit) {
            return MoveResult::UNIT_NOT_FOUND;
        }
        
        // 移動中でない場合は何もしない
        if (unit->getState() != UnitState::MOVING) {
            return MoveResult::SUCCESS;
        }
        
        Position currentPos = unit->getPosition();
        Position targetPos = unit->getTargetPosition();
        
        // 既に目標位置に到達している場合
        if (currentPos == targetPos) {
            return MoveResult::SUCCESS;
        }
        
        // 移動計算
        Position newPosition = calculateNextPosition(*unit, deltaTime);
        
        // 衝突検知
        if (hasCollision(*unit, newPosition)) {
            return MoveResult::COLLISION_DETECTED;
        }
        
        // 位置更新
        unit->updatePosition(newPosition);
        unitRepository_->save(unit);
        
        return MoveResult::SUCCESS;
    }
    
    /**
     * @brief 衝突を考慮した安全な移動位置を計算
     * @param unitId 移動するユニットのID
     * @param deltaTime 経過時間
     * @return 新しい位置と移動結果のペア
     */
    std::pair<Position, MoveResult> calculateSafePosition(int unitId, float deltaTime) {
        auto unit = unitRepository_->findById(unitId);
        if (!unit) {
            return {unit->getPosition(), MoveResult::UNIT_NOT_FOUND};
        }
        
        Position currentPos = unit->getPosition();
        Position targetPos = unit->getTargetPosition();
        
        if (currentPos == targetPos) {
            return {currentPos, MoveResult::SUCCESS};
        }
        
        // 最大移動可能な位置を計算
        Position nextPosition = calculateNextPosition(*unit, deltaTime);
        
        // 衝突検知
        if (!hasCollision(*unit, nextPosition)) {
            return {nextPosition, MoveResult::SUCCESS};
        }
        
        // 衝突がある場合、安全な位置を探す
        Position safePosition = findSafePosition(*unit, nextPosition);
        
        if (safePosition == currentPos) {
            return {currentPos, MoveResult::COLLISION_DETECTED};
        }
        
        return {safePosition, MoveResult::SUCCESS};
    }
    
    /**
     * @brief 衝突判定半径を設定
     * @param radius 衝突半径
     */
    void setCollisionRadius(float radius) {
        collisionRadius_ = std::max(0.1f, radius);
    }
    
    /**
     * @brief 移動結果を文字列に変換（デバッグ用）
     */
    static std::string moveResultToString(MoveResult result) {
        switch (result) {
            case MoveResult::SUCCESS: return "SUCCESS";
            case MoveResult::UNIT_NOT_FOUND: return "UNIT_NOT_FOUND";
            case MoveResult::UNIT_CANNOT_MOVE: return "UNIT_CANNOT_MOVE";
            case MoveResult::COLLISION_DETECTED: return "COLLISION_DETECTED";
            case MoveResult::INVALID_POSITION: return "INVALID_POSITION";
            case MoveResult::MOVE_TOO_FAR: return "MOVE_TOO_FAR";
            default: return "UNKNOWN";
        }
    }

private:
    std::shared_ptr<IUnitRepository> unitRepository_;
    float collisionRadius_;
    
    /**
     * @brief 次のフレームでの位置を計算
     */
    Position calculateNextPosition(const UnitEntity& unit, float deltaTime) const {
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
    
    /**
     * @brief 衝突判定
     */
    bool hasCollision(const UnitEntity& unit, const Position& newPosition) const {
        // 範囲内の他ユニットを取得
        auto nearbyUnits = unitRepository_->findInRange(newPosition, collisionRadius_ * 2.0f);
        
        for (const auto& otherUnit : nearbyUnits) {
            if (otherUnit->getId() == unit.getId() || !otherUnit->isAlive()) {
                continue; // 自分自身または死亡ユニットは無視
            }
            
            float distance = newPosition.distanceTo(otherUnit->getPosition());
            if (distance < collisionRadius_) {
                return true; // 衝突検出
            }
        }
        
        return false;
    }
    
    /**
     * @brief 安全な移動位置を探す
     */
    Position findSafePosition(const UnitEntity& unit, const Position& desiredPosition) const {
        Position currentPos = unit.getPosition();
        
        // 現在位置から目標位置への方向で、衝突しない最大の位置を見つける
        const int steps = 10;
        for (int i = steps - 1; i >= 0; --i) {
            float ratio = static_cast<float>(i) / steps;
            Position testPos = Position(
                currentPos.getX() + (desiredPosition.getX() - currentPos.getX()) * ratio,
                currentPos.getY() + (desiredPosition.getY() - currentPos.getY()) * ratio
            );
            
            if (!hasCollision(unit, testPos)) {
                return testPos;
            }
        }
        
        return currentPos; // 安全な位置が見つからない場合は現在位置
    }
};

#endif // SIMULATION_GAME_MOVE_UNIT_USECASE_H
