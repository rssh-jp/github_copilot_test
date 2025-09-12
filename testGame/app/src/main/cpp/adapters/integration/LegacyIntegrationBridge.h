#ifndef SIMULATION_GAME_LEGACY_INTEGRATION_BRIDGE_H
#define SIMULATION_GAME_LEGACY_INTEGRATION_BRIDGE_H

#include "../../usecases/game/GameFacade.h"
#include "../repositories/MemoryUnitRepository.h"
#include "../LegacyUnitAdapter.h"
#include "../../UnitRenderer.h" // 既存のUnitRenderer
#include "../../domain/entities/CombatSystem.h"
#include <memory>

/**
 * @brief 新しいクリーンアーキテクチャと既存システムの統合ブリッジ
 * 
 * 設計方針：
 * - 段階的移行を可能にする
 * - 既存コードの破壊的変更を最小限に
 * - 新旧システムの同期を管理
 * - 将来的な完全移行への道筋を提供
 */
class LegacyIntegrationBridge {
public:
    /**
     * @brief コンストラクタ
     * @param existingUnitRenderer 既存のUnitRenderer
     */
    explicit LegacyIntegrationBridge(UnitRenderer* existingUnitRenderer)
        : legacyUnitRenderer_(existingUnitRenderer),
          repository_(std::make_shared<MemoryUnitRepository>()),
          combatSystem_(std::make_shared<CombatSystem>()),
          gameFacade_(repository_, combatSystem_) {
        
        // 既存ユニットを新システムに同期
        syncFromLegacySystem();
        
        // ゲーム初期化
        gameFacade_.initializeGame();
    }
    
    /**
     * @brief ゲーム更新（毎フレーム呼び出し）
     * @param deltaTime 経過時間
     */
    void updateGame(float deltaTime) {
        // 新システムでゲーム更新
        gameFacade_.updateGame(deltaTime);
        
        // 変更を既存システムに同期
        syncToLegacySystem();
    }
    
    /**
     * @brief プレイヤーユニットの移動指令（JNI/UI から呼ばれる）
     */
    bool movePlayerUnit(float x, float y) {
        Position targetPos(x, y);
        MoveResult result = gameFacade_.movePlayerUnit(targetPos);
        
        return result == MoveResult::SUCCESS;
    }
    
    /**
     * @brief プレイヤーユニットの停止
     */
    bool stopPlayerUnit() {
        MoveResult result = gameFacade_.stopPlayerUnit();
        return result == MoveResult::SUCCESS;
    }
    
    /**
     * @brief プレイヤーユニットの自動攻撃
     */
    bool playerAutoAttack() {
        auto result = gameFacade_.playerAutoAttack();
        return result.first == AttackResult::SUCCESS;
    }
    
    /**
     * @brief プレイヤーユニットの情報を取得（JNI用）
     */
    struct PlayerUnitInfo {
        bool exists;
        int id;
        std::string name;
        float x, y;
        float targetX, targetY;
        int currentHP, maxHP;
        int attackPower;
        float moveSpeed;
        float attackRange;
        std::string state;
    };
    
    PlayerUnitInfo getPlayerUnitInfo() {
        PlayerUnitInfo info{};
        
        auto playerUnit = gameFacade_.getPlayerUnit();
        if (!playerUnit) {
            info.exists = false;
            return info;
        }
        
        info.exists = true;
        info.id = playerUnit->getId();
        info.name = playerUnit->getName();
        
        Position pos = playerUnit->getPosition();
        info.x = pos.getX();
        info.y = pos.getY();
        
        Position targetPos = playerUnit->getTargetPosition();
        info.targetX = targetPos.getX();
        info.targetY = targetPos.getY();
        
        UnitStats stats = playerUnit->getStats();
        info.currentHP = stats.getCurrentHp();
        info.maxHP = stats.getMaxHp();
        info.attackPower = stats.getAttackPower();
        info.moveSpeed = stats.getMoveSpeed();
        info.attackRange = stats.getAttackRange();
        
        info.state = playerUnit->getStateString();
        
        return info;
    }
    
    /**
     * @brief 全ユニットの情報を取得（描画用）
     */
    std::vector<PlayerUnitInfo> getAllUnitsInfo() {
        std::vector<PlayerUnitInfo> unitsInfo;
        auto allUnits = gameFacade_.getAllUnits();
        
        for (const auto& unit : allUnits) {
            if (!unit) continue;
            
            PlayerUnitInfo info{};
            info.exists = true;
            info.id = unit->getId();
            info.name = unit->getName();
            
            Position pos = unit->getPosition();
            info.x = pos.getX();
            info.y = pos.getY();
            
            Position targetPos = unit->getTargetPosition();
            info.targetX = targetPos.getX();
            info.targetY = targetPos.getY();
            
            UnitStats stats = unit->getStats();
            info.currentHP = stats.getCurrentHp();
            info.maxHP = stats.getMaxHp();
            info.attackPower = stats.getAttackPower();
            info.moveSpeed = stats.getMoveSpeed();
            info.attackRange = stats.getAttackRange();
            
            info.state = unit->getStateString();
            
            unitsInfo.push_back(info);
        }
        
        return unitsInfo;
    }
    
    /**
     * @brief ゲーム統計情報を取得
     */
    GameFacade::GameStatistics getGameStatistics() {
        return gameFacade_.getGameStatistics();
    }
    
    /**
     * @brief ゲームをリセット
     */
    void resetGame() {
        gameFacade_.resetGame();
        syncToLegacySystem();
    }
    
    /**
     * @brief デバッグ情報を出力
     */
    void printDebugInfo() {
        gameFacade_.printDebugInfo();
    }

private:
    UnitRenderer* legacyUnitRenderer_;
    std::shared_ptr<MemoryUnitRepository> repository_;
    std::shared_ptr<CombatSystem> combatSystem_;
    GameFacade gameFacade_;
    
    /**
     * @brief 既存システムから新システムに同期
     */
    void syncFromLegacySystem() {
        if (!legacyUnitRenderer_) {
            return;
        }
        
        // 既存のユニット情報を取得
        auto legacyUnits = legacyUnitRenderer_->getAllUnits();
        
        // 新システムに変換して保存
        for (const auto& legacyPair : legacyUnits) {
            auto unitEntity = LegacyUnitAdapter::fromLegacyUnit(legacyPair.second);
            if (unitEntity) {
                repository_->save(unitEntity);
            }
        }
    }
    
    /**
     * @brief 新システムから既存システムに同期
     */
    void syncToLegacySystem() {
        if (!legacyUnitRenderer_) {
            return;
        }
        
        // 新システムのユニット情報を取得
        auto entities = repository_->findAll();
        
        // 既存システムのユニットを更新
        auto& legacyUnits = const_cast<std::unordered_map<int, std::shared_ptr<Unit>>&>(
            legacyUnitRenderer_->getAllUnits());
        
        for (const auto& entity : entities) {
            if (!entity) continue;
            
            auto it = legacyUnits.find(entity->getId());
            if (it != legacyUnits.end()) {
                // 既存ユニットを更新
                LegacyUnitAdapter::updateLegacyUnit(entity, it->second);
            } else {
                // 新しいユニットを作成（必要に応じて）
                // この部分は具体的な既存UnitクラスのAPIに依存
            }
        }
    }
};

#endif // SIMULATION_GAME_LEGACY_INTEGRATION_BRIDGE_H
