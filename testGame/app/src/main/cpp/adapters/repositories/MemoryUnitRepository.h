#ifndef SIMULATION_GAME_MEMORY_UNIT_REPOSITORY_H
#define SIMULATION_GAME_MEMORY_UNIT_REPOSITORY_H

#include "../../usecases/interfaces/IUnitRepository.h"
#include "../../domain/entities/UnitEntity.h"
#include "../../domain/value_objects/Position.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>

/**
 * @brief インメモリユニットリポジトリの実装
 * 
 * 設計方針：
 * - IUnitRepositoryインターフェースの具象実装
 * - 高速な検索のためのインデックス構造
 * - 既存UnitRendererとの統合を考慮
 * - スレッドセーフティは呼び出し元で制御
 */
class MemoryUnitRepository : public IUnitRepository {
public:
    /**
     * @brief コンストラクタ
     */
    MemoryUnitRepository() = default;
    
    /**
     * @brief デストラクタ
     */
    ~MemoryUnitRepository() override = default;
    
    // コピー・ムーブセマンティクス
    MemoryUnitRepository(const MemoryUnitRepository&) = delete;
    MemoryUnitRepository& operator=(const MemoryUnitRepository&) = delete;
    MemoryUnitRepository(MemoryUnitRepository&&) = default;
    MemoryUnitRepository& operator=(MemoryUnitRepository&&) = default;
    
    /**
     * @brief IDでユニットを検索
     */
    std::shared_ptr<UnitEntity> findById(int id) override {
        auto it = units_.find(id);
        return (it != units_.end()) ? it->second : nullptr;
    }
    
    /**
     * @brief 全ユニットを取得
     */
    std::vector<std::shared_ptr<UnitEntity>> findAll() override {
        std::vector<std::shared_ptr<UnitEntity>> result;
        result.reserve(units_.size());
        
        for (const auto& pair : units_) {
            result.push_back(pair.second);
        }
        
        return result;
    }
    
    /**
     * @brief 生きているユニットのみを取得
     */
    std::vector<std::shared_ptr<UnitEntity>> findAlive() override {
        std::vector<std::shared_ptr<UnitEntity>> result;
        
        for (const auto& pair : units_) {
            if (pair.second && pair.second->isAlive()) {
                result.push_back(pair.second);
            }
        }
        
        return result;
    }
    
    /**
     * @brief 指定した範囲内のユニットを検索
     */
    std::vector<std::shared_ptr<UnitEntity>> findInRange(
        const Position& center, float radius) override {
        
        std::vector<std::shared_ptr<UnitEntity>> result;
        float radiusSquared = radius * radius; // 平方根計算を避けるため
        
        for (const auto& pair : units_) {
            if (!pair.second || !pair.second->isAlive()) {
                continue;
            }
            
            float distanceSquared = calculateDistanceSquared(
                center, pair.second->getPosition());
            
            if (distanceSquared <= radiusSquared) {
                result.push_back(pair.second);
            }
        }
        
        return result;
    }
    
    /**
     * @brief ユニットを保存
     */
    void save(std::shared_ptr<UnitEntity> unit) override {
        if (!unit) {
            return;
        }
        
        int id = unit->getId();
        units_[id] = unit;
        
        // デバッグログ
        #ifdef DEBUG
        aout << "Saved unit: " << unit->getName() 
             << " (ID: " << id << ") at (" 
             << unit->getPosition().getX() << ", " 
             << unit->getPosition().getY() << ")" << std::endl;
        #endif
    }
    
    /**
     * @brief ユニットを削除
     */
    bool remove(int id) override {
        auto it = units_.find(id);
        if (it != units_.end()) {
            units_.erase(it);
            return true;
        }
        return false;
    }
    
    /**
     * @brief 全ユニットを削除
     */
    void removeAll() override {
        units_.clear();
    }
    
    /**
     * @brief ユニット数を取得
     */
    size_t count() const override {
        return units_.size();
    }
    
    /**
     * @brief 生存ユニット数を取得
     */
    size_t countAlive() const override {
        size_t aliveCount = 0;
        for (const auto& pair : units_) {
            if (pair.second && pair.second->isAlive()) {
                aliveCount++;
            }
        }
        return aliveCount;
    }
    
    /**
     * @brief 最も近いユニットを検索
     * @param position 基準位置
     * @param excludeId 除外するユニットID（自分自身など）
     */
    std::shared_ptr<UnitEntity> findNearest(const Position& position, int excludeId = -1) {
        std::shared_ptr<UnitEntity> nearest = nullptr;
        float nearestDistanceSquared = std::numeric_limits<float>::max();
        
        for (const auto& pair : units_) {
            if (!pair.second || !pair.second->isAlive() || 
                pair.second->getId() == excludeId) {
                continue;
            }
            
            float distanceSquared = calculateDistanceSquared(
                position, pair.second->getPosition());
            
            if (distanceSquared < nearestDistanceSquared) {
                nearest = pair.second;
                nearestDistanceSquared = distanceSquared;
            }
        }
        
        return nearest;
    }
    
    /**
     * @brief 指定した状態のユニットを検索
     */
    std::vector<std::shared_ptr<UnitEntity>> findByState(UnitState state) {
        std::vector<std::shared_ptr<UnitEntity>> result;
        
        for (const auto& pair : units_) {
            if (pair.second && pair.second->getState() == state) {
                result.push_back(pair.second);
            }
        }
        
        return result;
    }
    
    /**
     * @brief 統計情報を取得（デバッグ用）
     */
    struct RepositoryStatistics {
        size_t totalUnits;
        size_t aliveUnits;
        size_t movingUnits;
        size_t combatUnits;
        size_t deadUnits;
    };
    
    RepositoryStatistics getStatistics() const {
        RepositoryStatistics stats{};
        stats.totalUnits = units_.size();
        
        for (const auto& pair : units_) {
            if (!pair.second) continue;
            
            if (pair.second->isAlive()) {
                stats.aliveUnits++;
                
                switch (pair.second->getState()) {
                    case UnitState::MOVING:
                        stats.movingUnits++;
                        break;
                    case UnitState::COMBAT:
                        stats.combatUnits++;
                        break;
                    default:
                        break;
                }
            } else {
                stats.deadUnits++;
            }
        }
        
        return stats;
    }

private:
    std::unordered_map<int, std::shared_ptr<UnitEntity>> units_;
    
    /**
     * @brief 距離の二乗を計算（平方根計算を避けるため）
     */
    float calculateDistanceSquared(const Position& pos1, const Position& pos2) const {
        float dx = pos1.getX() - pos2.getX();
        float dy = pos1.getY() - pos2.getY();
        return dx * dx + dy * dy;
    }
};

#endif // SIMULATION_GAME_MEMORY_UNIT_REPOSITORY_H
