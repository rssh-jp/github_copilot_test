#ifndef TESTGAME_UNIT_H
#define TESTGAME_UNIT_H

#include <string>
#include "Model.h"

/**
 * @brief ゲーム内のユニットを表すクラス
 * 
 * このクラスは平面上を動くユニットの基本情報と動作を定義します。
 */
class Unit {
public:
/**
 * @brief ユニットのコンストラクタ
 * 
 * @param name ユニットの名前
 * @param id ユニットの一意識別子
 * @param x X座標の初期値
 * @param y Y座標の初期値
 * @param speed ユニットの移動速度
 * @param maxHP 最大ヒットポイント
 * @param minAttack 最小攻撃力
 * @param maxAttack 最大攻撃力
 * @param defense 防御力
 * @param attackSpeed 攻撃速度（回/秒）
 */
Unit(const std::string& name, int id, float x, float y, float speed,
     int maxHP = 100, int minAttack = 1, int maxAttack = 6, 
     int defense = 0, float attackSpeed = 1.0f);    /**
     * @brief デストラクタ
     */
    virtual ~Unit();
    
    /**
     * @brief ユニットを更新する
     * 
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    virtual void update(float deltaTime);
    
    /**
     * @brief 戦闘対象を探す
     * 
     * @param units 考慮すべき他のユニットのリスト
     * @return 攻撃対象のユニット（見つからなければnullptr）
     */
    std::shared_ptr<Unit> findTargetToAttack(const std::vector<std::shared_ptr<Unit>>& units);
    
    /**
     * @brief ユニットを指定方向に移動する
     * 
     * @param dirX X方向の移動量（-1.0～1.0）
     * @param dirY Y方向の移動量（-1.0～1.0）
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void move(float dirX, float dirY, float deltaTime);
    
    /**
     * @brief ユニットの座標を設定する
     * 
     * @param x 新しいX座標
     * @param y 新しいY座標
     */
    void setPosition(float x, float y);
    
    /**
     * @brief ユニットの移動先を設定する
     * 
     * @param x 目標X座標
     * @param y 目標Y座標
     */
    void setTargetPosition(float x, float y);
    
    /**
     * @brief ユニットのX座標を取得する
     * 
     * @return X座標
     */
    float getX() const { return x_; }
    
    /**
     * @brief ユニットのY座標を取得する
     * 
     * @return Y座標
     */
    float getY() const { return y_; }
    
    /**
     * @brief ユニットのIDを取得する
     * 
     * @return ユニットID
     */
    int getId() const { return id_; }
    
    /**
     * @brief ユニットの名前を取得する
     * 
     * @return ユニットの名前
     */
    const std::string& getName() const { return name_; }
    
    /**
     * @brief ユニットの速度を取得する
     * 
     * @return 移動速度
     */
    float getSpeed() const { return speed_; }
    
    /**
     * @brief ユニットの速度を設定する
     * 
     * @param newSpeed 新しい移動速度
     */
    void setSpeed(float newSpeed) { speed_ = newSpeed; }
    
    /**
     * @brief 現在衝突中かどうかを取得する
     * 
     * @return 衝突中の場合はtrue、そうでなければfalse
     */
    bool isColliding() const { return isColliding_; }
    
    /**
     * @brief このユニットと別のユニットとの衝突をチェック
     * 
     * @param other 衝突をチェックする相手のユニット
     * @return 衝突しているかどうか
     */
    bool isColliding(const std::shared_ptr<Unit>& other) const;
    
    /**
     * @brief 他のユニットとの衝突を回避するための方向を計算
     * 
     * @param units 考慮すべき他のユニットのリスト
     * @param avoidanceStrength 回避の強さ（0.0～1.0）
     */
    void avoidCollisions(const std::vector<std::shared_ptr<Unit>>& units, float avoidanceStrength);
    
    /**
     * @brief 攻撃処理を行う
     * 
     * @param target 攻撃対象のユニット
     * @return 実際に与えたダメージ量、攻撃できなかった場合は0
     */
    int attack(std::shared_ptr<Unit> target);
    
    /**
     * @brief ダメージを受ける処理
     * 
     * @param damage 受けるダメージの量（防御力で軽減される前の値）
     * @return 実際に受けたダメージ量
     */
    int takeDamage(int damage);
    
    /**
     * @brief 攻撃クールダウンを更新する
     * 
     * @param deltaTime 前フレームからの経過時間（秒）
     * @return 攻撃可能になったらtrue
     */
    bool updateAttackCooldown(float deltaTime);
    
    /**
     * @brief 現在のHPを取得する
     * 
     * @return 現在のHP値
     */
    int getCurrentHP() const { return currentHP_; }
    
    /**
     * @brief 最大HPを取得する
     * 
     * @return 最大HP値
     */
    int getMaxHP() const { return maxHP_; }
    
    /**
     * @brief 最小攻撃力を取得する
     * 
     * @return 最小攻撃力
     */
    int getMinAttack() const { return minAttack_; }
    
    /**
     * @brief 最大攻撃力を取得する
     * 
     * @return 最大攻撃力
     */
    int getMaxAttack() const { return maxAttack_; }
    
    /**
     * @brief 防御力を取得する
     * 
     * @return 防御力
     */
    int getDefense() const { return defense_; }
    
    /**
     * @brief 攻撃速度を取得する
     * 
     * @return 攻撃速度（回/秒）
     */
    float getAttackSpeed() const { return attackSpeed_; }
    
    /**
     * @brief 攻撃可能かどうか確認する
     * 
     * @return 攻撃可能な場合はtrue
     */
    bool canAttack() const { return attackCooldown_ <= 0.0f; }
    
    /**
     * @brief ユニットが攻撃中かどうか確認する
     * 
     * @return 攻撃中の場合はtrue
     */
    bool isAttacking() const { return isAttacking_; }
    
    /**
     * @brief ユニットが生存しているかどうか確認する
     * 
     * @return 生存している場合はtrue
     */
    bool isAlive() const { return currentHP_ > 0; }
    
    /**
     * @brief 衝突判定の半径を取得する
     * 
     * @return 衝突判定の半径
     */
    static float getCollisionRadius() { return COLLISION_RADIUS; }
    
    /**
     * @brief ユニットが戦闘中かどうか確認する
     * 
     * @return 戦闘中の場合はtrue
     */
    bool inCombat() const { return inCombat_; }
    
    /**
     * @brief ユニットの戦闘状態を設定する
     * 
     * @param state 設定する戦闘状態
     */
    void setInCombat(bool state) { inCombat_ = state; }

private:
    std::string name_; ///< ユニットの名前
    int id_;           ///< ユニットの一意識別子
    float x_;          ///< X座標
    float y_;          ///< Y座標
    float speed_;      ///< 移動速度（単位/秒）
    
    float targetX_;    ///< 目標X座標
    float targetY_;    ///< 目標Y座標
    bool hasTarget_;   ///< 目標位置が設定されているか
    bool isBlocked_;   ///< 衝突によって移動が妨げられているか
    bool isColliding_; ///< 現在衝突中かどうか（色変更のため）
    bool isAttacking_; ///< 攻撃中かどうか（視覚効果のため）
    bool inCombat_;    ///< 戦闘中フラグ（移動停止のため）
    
    // 戦闘用パラメータ
    int maxHP_;        ///< 最大ヒットポイント
    int currentHP_;    ///< 現在のヒットポイント
    int minAttack_;    ///< 最小攻撃力（ダイスの最小値）
    int maxAttack_;    ///< 最大攻撃力（ダイスの最大値）
    int defense_;      ///< 防御力
    float attackSpeed_; ///< 攻撃速度（回/秒）
    float attackCooldown_; ///< 次の攻撃までのクールダウン時間
    
    // 衝突回避のための設定
    // ユニットの見た目のサイズに近い値（余裕を持たせる）
    static constexpr float COLLISION_RADIUS = 0.35f; ///< 衝突判定の半径（めり込み防止のため拡大）
};

#endif // TESTGAME_UNIT_H
