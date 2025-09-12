#ifndef SIMULATION_GAME_IUNIT_REPOSITORY_H
#define SIMULATION_GAME_IUNIT_REPOSITORY_H

#include "../domain/entities/UnitEntity.h"
#include <memory>
#include <vector>

/**
 * @brief ユニットリポジトリのインターフェース
 * 
 * 設計方針：
 * - 依存関係逆転の原則（DIP）を適用
 * - ドメイン層は具体的な実装に依存しない
 * - データアクセスの抽象化
 */
class IUnitRepository {
public:
    virtual ~IUnitRepository() = default;
    
    /**
     * @brief IDでユニットを検索
     * @param id ユニットID
     * @return ユニット（見つからない場合はnullptr）
     */
    virtual std::shared_ptr<UnitEntity> findById(int id) = 0;
    
    /**
     * @brief 全ユニットを取得
     * @return 全ユニットのリスト
     */
    virtual std::vector<std::shared_ptr<UnitEntity>> findAll() = 0;
    
    /**
     * @brief 生きているユニットのみを取得
     * @return 生存ユニットのリスト
     */
    virtual std::vector<std::shared_ptr<UnitEntity>> findAlive() = 0;
    
    /**
     * @brief 指定した範囲内のユニットを検索
     * @param center 中心位置
     * @param radius 検索半径
     * @return 範囲内のユニットリスト
     */
    virtual std::vector<std::shared_ptr<UnitEntity>> findInRange(
        const Position& center, float radius) = 0;
    
    /**
     * @brief ユニットを保存
     * @param unit 保存するユニット
     */
    virtual void save(std::shared_ptr<UnitEntity> unit) = 0;
    
    /**
     * @brief ユニットを削除
     * @param id 削除するユニットのID
     * @return 削除に成功したかどうか
     */
    virtual bool remove(int id) = 0;
    
    /**
     * @brief 全ユニットを削除
     */
    virtual void removeAll() = 0;
    
    /**
     * @brief ユニット数を取得
     * @return 総ユニット数
     */
    virtual size_t count() const = 0;
    
    /**
     * @brief 生存ユニット数を取得
     * @return 生存ユニット数
     */
    virtual size_t countAlive() const = 0;
};

#endif // SIMULATION_GAME_IUNIT_REPOSITORY_H
