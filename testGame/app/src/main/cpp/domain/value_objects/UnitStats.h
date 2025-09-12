#ifndef SIMULATION_GAME_UNIT_STATS_H
#define SIMULATION_GAME_UNIT_STATS_H

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
     * @param attackPower 攻撃力
     * @param moveSpeed 移動速度
     * @param attackRange 攻撃範囲
     */
    UnitStats(int maxHp, int currentHp, int attackPower, float moveSpeed, float attackRange)
        : maxHp_(maxHp), currentHp_(currentHp), attackPower_(attackPower), 
          moveSpeed_(moveSpeed), attackRange_(attackRange) {
        
        // ビジネスルール: 現在HPは最大HPを超えられない
        if (currentHp_ > maxHp_) {
            currentHp_ = maxHp_;
        }
        
        // ビジネスルール: 負の値は許可しない
        if (currentHp_ < 0) currentHp_ = 0;
        if (maxHp_ < 1) maxHp_ = 1;
        if (attackPower_ < 0) attackPower_ = 0;
        if (moveSpeed_ < 0.0f) moveSpeed_ = 0.0f;
        if (attackRange_ < 0.0f) attackRange_ = 0.0f;
    }
    
    /**
     * @brief デフォルトステータスでユニットを作成
     */
    static UnitStats createDefault() {
        return UnitStats(100, 100, 20, 1.0f, 2.0f);
    }
    
    /**
     * @brief 強いユニットのステータスを作成
     */
    static UnitStats createStrong() {
        return UnitStats(150, 150, 30, 1.2f, 2.5f);
    }
    
    // ゲッター
    int getMaxHp() const { return maxHp_; }
    int getCurrentHp() const { return currentHp_; }
    int getAttackPower() const { return attackPower_; }
    float getMoveSpeed() const { return moveSpeed_; }
    float getAttackRange() const { return attackRange_; }
    
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
        return UnitStats(maxHp_, newHp, attackPower_, moveSpeed_, attackRange_);
    }
    
    /**
     * @brief 回復した新しいステータスを返す
     * @param healAmount 回復量
     * @return 回復後の新しいステータス
     */
    UnitStats heal(int healAmount) const {
        int newHp = currentHp_ + healAmount;
        return UnitStats(maxHp_, newHp, attackPower_, moveSpeed_, attackRange_);
    }
    
    /**
     * @brief 攻撃力を変更した新しいステータスを返す
     * @param newAttackPower 新しい攻撃力
     * @return 変更後のステータス
     */
    UnitStats withAttackPower(int newAttackPower) const {
        return UnitStats(maxHp_, currentHp_, newAttackPower, moveSpeed_, attackRange_);
    }
    
    /**
     * @brief 移動速度を変更した新しいステータスを返す
     * @param newMoveSpeed 新しい移動速度
     * @return 変更後のステータス
     */
    UnitStats withMoveSpeed(float newMoveSpeed) const {
        return UnitStats(maxHp_, currentHp_, attackPower_, newMoveSpeed, attackRange_);
    }
    
    // 等価性演算子
    bool operator==(const UnitStats& other) const {
        return maxHp_ == other.maxHp_ &&
               currentHp_ == other.currentHp_ &&
               attackPower_ == other.attackPower_ &&
               std::abs(moveSpeed_ - other.moveSpeed_) < EPSILON &&
               std::abs(attackRange_ - other.attackRange_) < EPSILON;
    }
    
    bool operator!=(const UnitStats& other) const {
        return !(*this == other);
    }

private:
    int maxHp_;
    int currentHp_;
    int attackPower_;
    float moveSpeed_;
    float attackRange_;
    
    static constexpr float EPSILON = 1e-6f;
};

#endif // SIMULATION_GAME_UNIT_STATS_H
