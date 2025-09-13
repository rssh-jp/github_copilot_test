#ifndef SIMULATION_GAME_UNIT_ENTITY_H
#define SIMULATION_GAME_UNIT_ENTITY_H

#include "../value_objects/Position.h"
#include "../value_objects/UnitStats.h"
#include <string>
#include <memory>
#include <cmath>

/**
 * @brief ユニットの状態を表すEnum
 */
enum class UnitState {
    IDLE,       // 待機状態
    MOVING,     // 移動中
    COMBAT,     // 戦闘中
    DEAD        // 死亡
};

/**
 * @brief ユニットエンティティ
 * 
 * 設計方針：
 * - ドメインの中心となるエンティティ
 * - ビジネスルールとロジックを含む
 * - 一意性をIDで管理
 * - 状態変更は明示的なメソッドを通してのみ行う
 */
class UnitEntity {
public:
    /**
     * @brief コンストラクタ
     * @param id ユニットの一意ID
     * @param name ユニット名
     * @param position 初期位置
     * @param stats 初期ステータス
     */
    UnitEntity(int id, const std::string& name, const Position& position, const UnitStats& stats)
        : id_(id), name_(name), position_(position), stats_(stats), 
          targetPosition_(position), state_(UnitState::IDLE), lastAttackTime_(-1000.0f) {}
    
    // コピー・ムーブセマンティクス
    UnitEntity(const UnitEntity&) = default;
    UnitEntity& operator=(const UnitEntity&) = default;
    UnitEntity(UnitEntity&&) = default;
    UnitEntity& operator=(UnitEntity&&) = default;
    
    // ゲッター
    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    const Position& getPosition() const { return position_; }
    const Position& getTargetPosition() const { return targetPosition_; }
    const UnitStats& getStats() const { return stats_; }
    UnitState getState() const { return state_; }
    
    /**
     * @brief ユニットが生きているかチェック
     */
    bool isAlive() const {
        return stats_.isAlive() && state_ != UnitState::DEAD;
    }
    
    /**
     * @brief ユニットが移動可能かチェック
     */
    bool canMove() const {
        return isAlive() && (state_ == UnitState::IDLE || state_ == UnitState::MOVING);
    }
    
    /**
     * @brief ユニットが攻撃可能かチェック
     */
    // 現在時刻(秒)を引数で受け取る想定
    bool canAttack(float nowTime) const {
        if (!isAlive() || state_ == UnitState::DEAD) return false;
        float interval = 1.0f / stats_.getAttackSpeed();
        return (nowTime - lastAttackTime_) >= interval;
    }

    // 攻撃処理: 攻撃可能なら攻撃し、lastAttackTime_を更新
    bool tryAttack(UnitEntity& target, float nowTime) {
        if (!canAttack(nowTime)) return false;
        if (!isInAttackRange(target)) return false;
        lastAttackTime_ = nowTime;
        int damage = stats_.getRandomAttackPower();
        target.takeDamage(damage);
        return true;
    }
    
        /**
         * @brief 外部からlastAttackTime_を設定する（UseCaseが攻撃受付後に呼ぶ）
         */
        void setLastAttackTime(float t) { lastAttackTime_ = t; }
    
    /**
     * @brief 指定した位置が攻撃範囲内かチェック
     * @param targetPosition 攻撃対象の位置
     */
    bool isInAttackRange(const Position& targetPosition) const {
        float distance = position_.distanceTo(targetPosition);
        return distance <= stats_.getAttackRange();
    }
    
    /**
     * @brief 他のユニットが攻撃範囲内かチェック
     * @param other 攻撃対象のユニット
     */
    bool isInAttackRange(const UnitEntity& other) const {
        return isInAttackRange(other.getPosition());
    }
    
    /**
     * @brief 指定した位置まで移動可能かチェック
     * @param newPosition 移動先の位置
     * @param deltaTime 移動にかかる時間
     */
    bool canMoveTo(const Position& newPosition, float deltaTime) const {
        if (!canMove()) {
            return false;
        }
        
        float distance = position_.distanceTo(newPosition);
        float maxDistance = stats_.getMoveSpeed() * deltaTime;
        
        return distance <= maxDistance;
    }
    
    /**
     * @brief 移動先を設定
     * @param newTarget 新しい移動先
     * @return 設定に成功したかどうか
     */
    bool setTargetPosition(const Position& newTarget) {
        if (!canMove()) {
            return false;
        }
        
        targetPosition_ = newTarget;
        
        // 目標位置に到達していない場合は移動状態に
        if (position_ != targetPosition_) {
            state_ = UnitState::MOVING;
        } else {
            state_ = UnitState::IDLE;
        }
        
        return true;
    }
    
    /**
     * @brief 位置を更新（移動処理）
     * @param newPosition 新しい位置
     */
    void updatePosition(const Position& newPosition) {
        position_ = newPosition;
        
        // 目標位置に到達したかチェック
        if (position_ == targetPosition_) {
            state_ = UnitState::IDLE;
        }
    }
    
    /**
     * @brief 移動を更新（フレーム毎の移動処理）
     * @param deltaTime フレーム間の時間（秒）
     */
    void updateMovement(float deltaTime) {
        if (state_ == UnitState::DEAD) {
            return;
        }
        
        // 目標位置に向かって移動
        float distance = position_.distanceTo(targetPosition_);
        const float ARRIVAL_THRESHOLD = 0.05f; // 到着判定の閾値
        
        if (distance > ARRIVAL_THRESHOLD) {
            // 移動方向を計算
            float dx = targetPosition_.getX() - position_.getX();
            float dy = targetPosition_.getY() - position_.getY();
            
            // 正規化
            float moveX = (dx / distance) * stats_.getMoveSpeed() * deltaTime;
            float moveY = (dy / distance) * stats_.getMoveSpeed() * deltaTime;
            
            // 移動量が残り距離を超える場合は目標位置に直接設定
            if (std::sqrt(moveX * moveX + moveY * moveY) >= distance) {
                position_ = targetPosition_;
                state_ = UnitState::IDLE;
            } else {
                // 新しい位置に移動
                position_ = Position(position_.getX() + moveX, position_.getY() + moveY);
                state_ = UnitState::MOVING;
            }
        } else {
            // 目標位置に到達
            position_ = targetPosition_;
            state_ = UnitState::IDLE;
        }
    }
    
    /**
     * @brief ダメージを受ける
     * @param damage ダメージ量
     * @return ダメージを受けた後も生きているかどうか
     */
    bool takeDamage(int damage) {
        stats_ = stats_.takeDamage(damage);
        
        if (!stats_.isAlive()) {
            state_ = UnitState::DEAD;
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief 回復する
     * @param healAmount 回復量
     */
    void heal(int healAmount) {
        if (state_ != UnitState::DEAD) {
            stats_ = stats_.heal(healAmount);
        }
    }
    
    /**
     * @brief 戦闘状態に入る
     */
    void enterCombat() {
        if (isAlive()) {
            state_ = UnitState::COMBAT;
        }
    }
    
    /**
     * @brief 戦闘状態から離脱
     */
    void exitCombat() {
        if (isAlive() && state_ == UnitState::COMBAT) {
            // 移動中だった場合は移動状態に戻す
            if (position_ != targetPosition_) {
                state_ = UnitState::MOVING;
            } else {
                state_ = UnitState::IDLE;
            }
        }
    }
    
    /**
     * @brief 強制的に状態を設定（テスト用）
     */
    void setState(UnitState newState) {
        if (isAlive() || newState == UnitState::DEAD) {
            state_ = newState;
        }
    }
    
    /**
     * @brief デバッグ用の状態文字列を取得
     */
    std::string getStateString() const {
        switch (state_) {
            case UnitState::IDLE: return "IDLE";
            case UnitState::MOVING: return "MOVING";
            case UnitState::COMBAT: return "COMBAT";
            case UnitState::DEAD: return "DEAD";
            default: return "UNKNOWN";
        }
    }
    
    // 等価性は ID で判定（エンティティの特性）
    bool operator==(const UnitEntity& other) const {
        return id_ == other.id_;
    }
    
    bool operator!=(const UnitEntity& other) const {
        return !(*this == other);
    }

private:
    int id_;                    // 一意ID
    std::string name_;          // ユニット名
    Position position_;         // 現在位置
    Position targetPosition_;   // 移動目標位置
    UnitStats stats_;          // ステータス
    UnitState state_;          // 現在の状態
    float lastAttackTime_;     // 最後の攻撃時刻
};

#endif // SIMULATION_GAME_UNIT_ENTITY_H
