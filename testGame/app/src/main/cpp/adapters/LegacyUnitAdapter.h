#ifndef SIMULATION_GAME_LEGACY_UNIT_ADAPTER_H
#define SIMULATION_GAME_LEGACY_UNIT_ADAPTER_H

#include "../legacy/Unit.h" // 既存のUnitクラス
#include "../domain/entities/UnitEntity.h"
#include "../domain/value_objects/Position.h"
#include "../domain/value_objects/UnitStats.h"
#include <memory>

/**
 * @brief 既存UnitクラスとUnitEntityの間のアダプター
 * 
 * 設計方針：
 * - 段階的移行を可能にする
 * - 既存コードの破壊的変更を避ける
 * - 新旧アーキテクチャの橋渡し
 */
class LegacyUnitAdapter {
public:
    /**
     * @brief 既存UnitからUnitEntityを作成
     * @param legacyUnit 既存のUnitインスタンス
     * @return 新しいUnitEntity
     */
    static std::shared_ptr<UnitEntity> fromLegacyUnit(const std::shared_ptr<Unit>& legacyUnit) {
        if (!legacyUnit) {
            return nullptr;
        }
        
        // 位置情報の変換
        Position position(legacyUnit->getX(), legacyUnit->getY());
        
        // ステータスの変換
        UnitStats stats(
            legacyUnit->getMaxHP(),
            legacyUnit->getCurrentHP(),
            20, // デフォルト攻撃力（既存Unitにない場合）
            legacyUnit->getMoveSpeed(),
            2.0f // デフォルト攻撃範囲
        );
        
        // UnitEntityの作成
        auto unitEntity = std::make_shared<UnitEntity>(
            legacyUnit->getId(),
            legacyUnit->getName(),
            position,
            stats
        );
        
        // 状態の変換
        UnitState newState = convertLegacyState(legacyUnit->getState());
        unitEntity->setState(newState);
        
        // 移動先の設定
        if (legacyUnit->hasTargetPosition()) {
            Position targetPos(legacyUnit->getTargetX(), legacyUnit->getTargetY());
            unitEntity->setTargetPosition(targetPos);
        }
        
        return unitEntity;
    }
    
    /**
     * @brief UnitEntityから既存Unitを更新
     * @param unitEntity 新しいUnitEntity
     * @param legacyUnit 更新対象の既存Unit
     */
    static void updateLegacyUnit(const std::shared_ptr<UnitEntity>& unitEntity, 
                                std::shared_ptr<Unit>& legacyUnit) {
        if (!unitEntity || !legacyUnit) {
            return;
        }
        
        // 位置の更新
        Position pos = unitEntity->getPosition();
        legacyUnit->setPosition(pos.getX(), pos.getY());
        
        // ステータスの更新
        UnitStats stats = unitEntity->getStats();
        legacyUnit->setCurrentHP(stats.getCurrentHp());
        legacyUnit->setMoveSpeed(stats.getMoveSpeed());
        
        // 状態の更新
        UnitState state = unitEntity->getState();
        legacyUnit->setState(convertToLegacyState(state));
        
        // 移動先の更新
        Position targetPos = unitEntity->getTargetPosition();
        legacyUnit->setTargetPosition(targetPos.getX(), targetPos.getY());
    }
    
    /**
     * @brief 既存UnitのリストをUnitEntityのリストに変換
     */
    static std::vector<std::shared_ptr<UnitEntity>> convertUnitsToEntities(
        const std::vector<std::shared_ptr<Unit>>& legacyUnits) {
        
        std::vector<std::shared_ptr<UnitEntity>> entities;
        entities.reserve(legacyUnits.size());
        
        for (const auto& legacyUnit : legacyUnits) {
            auto entity = fromLegacyUnit(legacyUnit);
            if (entity) {
                entities.push_back(entity);
            }
        }
        
        return entities;
    }
    
    /**
     * @brief UnitEntityのリストから既存Unitのリストを更新
     */
    static void updateLegacyUnits(const std::vector<std::shared_ptr<UnitEntity>>& entities,
                                 std::vector<std::shared_ptr<Unit>>& legacyUnits) {
        
        // IDでマッピングして更新
        for (const auto& entity : entities) {
            for (auto& legacyUnit : legacyUnits) {
                if (legacyUnit && legacyUnit->getId() == entity->getId()) {
                    updateLegacyUnit(entity, legacyUnit);
                    break;
                }
            }
        }
    }

private:
    /**
     * @brief 既存の状態からUnitStateに変換
     */
    static UnitState convertLegacyState(::UnitState legacyState) {
        switch (legacyState) {
            case ::UnitState::IDLE:
                return UnitState::IDLE;
            case ::UnitState::MOVING:
                return UnitState::MOVING;
            case ::UnitState::COMBAT:
                return UnitState::COMBAT;
            default:
                return UnitState::IDLE;
        }
    }
    
    /**
     * @brief UnitStateから既存の状態に変換
     */
    static ::UnitState convertToLegacyState(UnitState newState) {
        switch (newState) {
            case UnitState::IDLE:
                return ::UnitState::IDLE;
            case UnitState::MOVING:
                return ::UnitState::MOVING;
            case UnitState::COMBAT:
                return ::UnitState::COMBAT;
            case UnitState::DEAD:
                return ::UnitState::IDLE; // 既存システムにDEADがない場合はIDLE
            default:
                return ::UnitState::IDLE;
        }
    }
};

#endif // SIMULATION_GAME_LEGACY_UNIT_ADAPTER_H
