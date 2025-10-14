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
        lastAttackTime_(-1000.0f), faction_(faction) {}

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
   */
  bool canMove() const {
    return isAlive() &&
           (state_ == UnitState::IDLE || state_ == UnitState::MOVING);
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
   */
  bool setTargetPosition(const Position &newTarget) {
    if (!canMove()) {
      return false;
    }

    targetPosition_ = newTarget;

    /**
     * Set a new movement target for this unit.
     * If the new target differs from current position, the unit enters MOVING
     * state.
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
   */
  void updatePosition(const Position &newPosition) {
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
};

#endif // SIMULATION_GAME_UNIT_ENTITY_H
