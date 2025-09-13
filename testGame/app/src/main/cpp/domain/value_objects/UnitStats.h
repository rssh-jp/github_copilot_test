#ifndef SIMULATION_GAME_UNIT_STATS_H
#define SIMULATION_GAME_UNIT_STATS_H

#include <cmath>
#include <cstdlib>

/**
 * @brief ユニットのステータスを表す値オブジェクト
 * 
 * 設計方針：
 * - 不変オブジェクト
 * - ゲームバランスに関するビジネスルールを含む
 * - ステータス操作は新しいインスタンスを返す
 */
class UnitStats {
public:
    /**
     * @brief コンストラクタ
     * @param maxHp 最大HP
     * @param currentHp 現在HP
     * @param minAttackPower 最小攻撃力
     * @param maxAttackPower 最大攻撃力
     * @param moveSpeed 移動速度
     * @param attackRange 攻撃範囲
     * @param attackSpeed 攻撃速度（1秒あたり攻撃回数）
     */
    UnitStats(int maxHp, int currentHp, int minAttackPower, int maxAttackPower, float moveSpeed, float attackRange, float attackSpeed)
        : maxHp_(maxHp), currentHp_(currentHp), minAttackPower_(minAttackPower), maxAttackPower_(maxAttackPower),
          moveSpeed_(moveSpeed), attackRange_(attackRange), attackSpeed_(attackSpeed) {
        // ビジネスルール: 現在HPは最大HPを超えられない
        if (currentHp_ > maxHp_) currentHp_ = maxHp_;
        if (currentHp_ < 0) currentHp_ = 0;
        if (maxHp_ < 1) maxHp_ = 1;
        if (minAttackPower_ < 0) minAttackPower_ = 0;
        if (maxAttackPower_ < minAttackPower_) maxAttackPower_ = minAttackPower_;
        if (moveSpeed_ < 0.0f) moveSpeed_ = 0.0f;
        if (attackRange_ < 0.0f) attackRange_ = 0.0f;
        if (attackSpeed_ < 0.01f) attackSpeed_ = 0.01f;
    }
    
    /**
     * @brief デフォルトステータスでユニットを作成
     */
    static UnitStats createDefault() {
        return UnitStats(100, 100, 10, 20, 1.0f, 2.0f, 1.0f); // 攻撃速度: 1回/秒
    }
    static UnitStats createStrong() {
        return UnitStats(150, 150, 20, 35, 1.2f, 2.5f, 2.0f); // 攻撃速度: 2回/秒
    }
    
    // ゲッター
    int getMaxHp() const { return maxHp_; }
    int getCurrentHp() const { return currentHp_; }
    int getMinAttackPower() const { return minAttackPower_; }
    int getMaxAttackPower() const { return maxAttackPower_; }
    float getMoveSpeed() const { return moveSpeed_; }
    float getAttackRange() const { return attackRange_; }
    float getAttackSpeed() const { return attackSpeed_; }

    /**
     * @brief 攻撃力をランダムで取得（min～maxの範囲）
     */
    int getRandomAttackPower() const {
    if (minAttackPower_ == maxAttackPower_) return minAttackPower_;
    return minAttackPower_ + (rand() % (maxAttackPower_ - minAttackPower_ + 1));
    }
    
    /**
     * @brief HPの割合を取得（0.0〜1.0）
     */
    float getHpRatio() const {
        return static_cast<float>(currentHp_) / static_cast<float>(maxHp_);
    }
    
    /**
     * @brief ユニットが生きているかチェック
     */
    bool isAlive() const {
        return currentHp_ > 0;
    }
    
    /**
     * @brief ダメージを受けた新しいステータスを返す
     * @param damage 受けるダメージ
     * @return ダメージ後の新しいステータス
     */
    UnitStats takeDamage(int damage) const {
        int newHp = currentHp_ - damage;
        return UnitStats(maxHp_, newHp, minAttackPower_, maxAttackPower_, moveSpeed_, attackRange_, attackSpeed_);
    }
    
    /**
     * @brief 回復した新しいステータスを返す
     * @param healAmount 回復量
     * @return 回復後の新しいステータス
     */
    UnitStats heal(int healAmount) const {
        int newHp = currentHp_ + healAmount;
        return UnitStats(maxHp_, newHp, minAttackPower_, maxAttackPower_, moveSpeed_, attackRange_, attackSpeed_);
    }
    
    /**
     * @brief 攻撃力を変更した新しいステータスを返す
     * @param newAttackPower 新しい攻撃力
     * @return 変更後のステータス
     */
    UnitStats withAttackPower(int newMinAttack, int newMaxAttack) const {
        return UnitStats(maxHp_, currentHp_, newMinAttack, newMaxAttack, moveSpeed_, attackRange_, attackSpeed_);
    }
    
    /**
     * @brief 移動速度を変更した新しいステータスを返す
     * @param newMoveSpeed 新しい移動速度
     * @return 変更後のステータス
     */
    UnitStats withMoveSpeed(float newMoveSpeed) const {
        return UnitStats(maxHp_, currentHp_, minAttackPower_, maxAttackPower_, newMoveSpeed, attackRange_, attackSpeed_);
    }
    
    // 等価性演算子
    bool operator==(const UnitStats& other) const {
        return maxHp_ == other.maxHp_ &&
               currentHp_ == other.currentHp_ &&
               minAttackPower_ == other.minAttackPower_ &&
               maxAttackPower_ == other.maxAttackPower_ &&
               std::abs(moveSpeed_ - other.moveSpeed_) < EPSILON &&
               std::abs(attackRange_ - other.attackRange_) < EPSILON &&
               std::abs(attackSpeed_ - other.attackSpeed_) < EPSILON;
    }
    
    bool operator!=(const UnitStats& other) const {
        return !(*this == other);
    }

private:
    int maxHp_;
    int currentHp_;
    int minAttackPower_;
    int maxAttackPower_;
    float moveSpeed_;
    float attackRange_;
    float attackSpeed_; // 1秒あたり攻撃回数
    static constexpr float EPSILON = 1e-6f;
};

#endif // SIMULATION_GAME_UNIT_STATS_H
