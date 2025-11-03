#ifndef SIMULATION_GAME_UNIT_ENTITY_H
#define SIMULATION_GAME_UNIT_ENTITY_H

/*
 * UnitEntity.h - Domain entity representing a game unit.
 *
 * Responsibilities:
 * - Encapsulate unit identity, position, stats and transient state
 * (IDLE/MOVING/COMBAT/DEAD).
 * - Provide query helpers used by use-cases and systems (isAlive, canMove,
 * canAttack, isInAttackRange).
 *
 * Invariants / Notes:
 * - Position and targetPosition are in world coordinates.
 * - Range checks may consider both attack range and target collision radius
 * (see overloads).
 * - This file contains only pure domain logic; side-effects (IO, rendering)
 * must not be added here.
 */
#define SIMULATION_GAME_UNIT_ENTITY_H

#include "../value_objects/Position.h"
#include "../value_objects/UnitStats.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

/**
 * @brief ユニットの状態を表すEnum
 */
enum class UnitState {
  IDLE,   // 待機状態
  MOVING, // 移動中
  COMBAT, // 戦闘中
  DEAD    // 死亡
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
  UnitEntity(int id, const std::string &name, const Position &position,
             const UnitStats &stats, int faction = 0)
      : id_(id), name_(name), position_(position), stats_(stats),
        targetPosition_(position), state_(UnitState::IDLE),
        lastAttackTime_(-1000.0f), faction_(faction), 
        suppressAttackUntil_(0.0f) {} // 0.0fで初期化 = デフォルトで攻撃したい状態

  // コピー・ムーブセマンティクス
  UnitEntity(const UnitEntity &) = default;
  UnitEntity &operator=(const UnitEntity &) = default;
  UnitEntity(UnitEntity &&) = default;
  UnitEntity &operator=(UnitEntity &&) = default;

  // ゲッター
  int getId() const { return id_; }
  const std::string &getName() const { return name_; }
  const Position &getPosition() const { return position_; }
  const Position &getTargetPosition() const { return targetPosition_; }
  const UnitStats &getStats() const { return stats_; }
  UnitState getState() const { return state_; }
  int getFaction() const { return faction_; }
  void setFaction(int f) { faction_ = f; }

  /**
   * @brief ユニットが生きているかチェック
   */
  bool isAlive() const { return stats_.isAlive() && state_ != UnitState::DEAD; }

  /**
   * @brief ユニットが移動可能かチェック
   * 
   * 戦闘中(COMBAT)でも移動可能にするため、DEAD以外の状態で移動を許可します。
   * これにより、敵ユニットに接近して攻撃モードになった後も移動が可能になります。
   */
  bool canMove() const {
    return isAlive() && state_ != UnitState::DEAD;
  }

  /**
   * @brief ユニットが攻撃可能かチェック
   */
  // 現在時刻(秒)を引数で受け取る想定
  bool canAttack(float nowTime) const {
    if (!isAlive() || state_ == UnitState::DEAD)
      return false;
    float interval = 1.0f / stats_.getAttackSpeed();
    return (nowTime - lastAttackTime_) >= interval;
  }

  // 攻撃処理: 攻撃可能なら攻撃し、lastAttackTime_を更新
  bool tryAttack(UnitEntity &target, float nowTime) {
    /**
     * Attempt an attack on target at nowTime.
     * Pre-conditions:
     * - caller is responsible for ensuring attacker and target are valid and
     * alive.
     * - this method enforces attack cooldown and range checks.
     * Returns true if an attack was performed and damage applied.
     */
    if (!canAttack(nowTime))
      return false;
    if (!isInAttackRange(target))
      return false;
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
  bool isInAttackRange(const Position &targetPosition) const {
    float distance = position_.distanceTo(targetPosition);
    // Note: when checking against a raw position we only compare to our own
    // attack range. When checking against another UnitEntity, prefer the
    // overload that takes a UnitEntity so that the target's collision radius
    // can be considered.
    return distance <= stats_.getAttackRange();
  }

  /**
   * @brief 他のユニットが攻撃範囲内かチェック
   * @param other 攻撃対象のユニット
   */
  bool isInAttackRange(const UnitEntity &other) const {
    // Consider the target's collision radius: attacker can reach the target's
    // body when distance <= attackRange + targetCollisionRadius
    float dx = position_.getX() - other.getPosition().getX();
    float dy = position_.getY() - other.getPosition().getY();
    float distance = std::sqrt(dx * dx + dy * dy);
    return distance <=
           (stats_.getAttackRange() + other.getStats().getCollisionRadius());
  }

  /**
   * @brief 指定した位置まで移動可能かチェック
   * @param newPosition 移動先の位置
   * @param deltaTime 移動にかかる時間
   */
  bool canMoveTo(const Position &newPosition, float deltaTime) const {
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
   * 
   * 新しい移動目標を設定すると、目標位置が現在位置と異なる場合は
   * MOVING状態に遷移します。これにより、COMBAT状態からでも
   * 新しい移動を開始でき、移動中に再び敵を検出できます。
   */
  bool setTargetPosition(const Position &newTarget) {
    if (!canMove()) {
      return false;
    }

    targetPosition_ = newTarget;

    /**
     * Set a new movement target for this unit.
     * 目標位置が現在位置と異なる場合は常にMOVING状態に遷移
     * これにより、COMBAT状態からでも新しい移動を開始できる
     */
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
   * 
   * 目標位置に到達した場合、MOVING状態からIDLEに遷移します。
   * ただし、COMBAT状態の場合は、戦闘中に少し移動しただけなので
   * COMBAT状態を維持します（戦闘からの離脱はexitCombat()で明示的に行う）。
   */
  void updatePosition(const Position &newPosition) {
    position_ = newPosition;

    // 目標位置に到達したかチェック
    if (position_ == targetPosition_) {
      // MOVING状態の場合のみIDLEに遷移
      // COMBAT状態の場合は戦闘状態を維持
      if (state_ == UnitState::MOVING) {
        state_ = UnitState::IDLE;
      }
    }
  }

  /**
   * @brief 移動を更新（フレーム毎の移動処理）
   * @param deltaTime フレーム間の時間（秒）
   */
  void updateMovement(float deltaTime) {
    updateMovementWithModifier(deltaTime, 1.0f);
  }

  void updateMovementWithModifier(float deltaTime, float speedModifier) {
    if (state_ == UnitState::DEAD) {
      return;
    }

    const float clampedModifier = std::max(0.0f, speedModifier);
    const float effectiveSpeed = stats_.getMoveSpeed() * clampedModifier;
    if (effectiveSpeed <= 0.0f || deltaTime <= 0.0f) {
      state_ = UnitState::IDLE;
      return;
    }

    float distance = position_.distanceTo(targetPosition_);
    static constexpr float kArrivalThreshold = 0.05f;

    if (distance <= kArrivalThreshold) {
      position_ = targetPosition_;
      state_ = UnitState::IDLE;
      return;
    }

    float dx = targetPosition_.getX() - position_.getX();
    float dy = targetPosition_.getY() - position_.getY();
    if (std::abs(dx) < 1e-6f && std::abs(dy) < 1e-6f) {
      state_ = UnitState::IDLE;
      return;
    }

    float moveDistance = effectiveSpeed * deltaTime;
    float moveX = (dx / distance) * moveDistance;
    float moveY = (dy / distance) * moveDistance;

    float moveLen = std::sqrt(moveX * moveX + moveY * moveY);
    if (moveLen >= distance) {
      position_ = targetPosition_;
      state_ = UnitState::IDLE;
    } else {
      Position nextPos(position_.getX() + moveX, position_.getY() + moveY);
      position_ = nextPos;
      state_ = UnitState::MOVING;
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
    case UnitState::IDLE:
      return "IDLE";
    case UnitState::MOVING:
      return "MOVING";
    case UnitState::COMBAT:
      return "COMBAT";
    case UnitState::DEAD:
      return "DEAD";
    default:
      return "UNKNOWN";
    }
  }

  /**
   * @brief ユニットを完全に初期状態にリセット
   * HPを最大HPに戻し、状態をIDLEにリセットします
   */
  void resetToInitialState() {
    // HPを最大HPにリセット
    int maxHp = stats_.getMaxHp();
    stats_ = UnitStats(maxHp, maxHp, stats_.getMinAttackPower(),
                       stats_.getMaxAttackPower(), stats_.getMoveSpeed(),
                       stats_.getAttackRange(), stats_.getAttackSpeed(),
                       stats_.getCollisionRadius());

    // 状態をIDLEにリセット
    state_ = UnitState::IDLE;

    // 移動目標を現在位置にリセット
    targetPosition_ = position_;

    // 最後の攻撃時刻をリセット
    lastAttackTime_ = -1000.0f;
    
    // 攻撃抑制をリセット（0.0fで初期化 = 攻撃したい状態）
    suppressAttackUntil_ = 0.0f;
  }

  /**
   * @brief 攻撃意思があるかチェック
   * @param currentTime 現在時刻（秒）
   * @return 攻撃する意思があるかどうか
   * @note 初期状態（suppressAttackUntil_ = 0.0f）では常に攻撃意思ありとなる
   */
  bool wantsToAttack(float currentTime) const {
    // suppressAttackUntil_ が現在時刻より前なら攻撃意思あり
    // 初期値0.0fなので、デフォルトでは攻撃したい状態
    return currentTime >= suppressAttackUntil_;
  }

  /**
   * @brief 一定時間攻撃を抑制する
   * @param currentTime 現在時刻（秒）
   * @param duration 抑制する時間（秒）
   */
  void suppressAttackFor(float currentTime, float duration) {
    suppressAttackUntil_ = currentTime + duration;
  }

  /**
   * @brief 攻撃抑制を即座に解除
   */
  void clearAttackSuppression() {
    suppressAttackUntil_ = 0.0f;
  }

  /**
   * @brief 攻撃抑制状態かどうかを取得
   * @param currentTime 現在時刻（秒）
   */
  bool isAttackSuppressed(float currentTime) const {
    return currentTime < suppressAttackUntil_;
  }

  // 等価性は ID で判定（エンティティの特性）
  bool operator==(const UnitEntity &other) const { return id_ == other.id_; }

  bool operator!=(const UnitEntity &other) const { return !(*this == other); }

private:
  int id_;                  // 一意ID
  std::string name_;        // ユニット名
  Position position_;       // 現在位置
  Position targetPosition_; // 移動目標位置
  UnitStats stats_;         // ステータス
  UnitState state_;         // 現在の状態
  float lastAttackTime_;    // 最後の攻撃時刻
  int faction_;             // 陣営ID（0: default / neutral）
  float suppressAttackUntil_; // この時刻まで攻撃を抑制（秒）
};

#endif // SIMULATION_GAME_UNIT_ENTITY_H
