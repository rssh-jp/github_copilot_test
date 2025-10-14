#ifndef SIMULATION_GAME_ATTACK_UNIT_USECASE_H
#define SIMULATION_GAME_ATTACK_UNIT_USECASE_H

#include "../../domain/entities/CombatSystem.h"
#include "../../domain/entities/UnitEntity.h"
#include "../interfaces/IUnitRepository.h"
#include <memory>
#include <vector>

/**
 * @brief 攻撃結果
 */
enum class AttackResult {
  SUCCESS,                // 攻撃成功
  ATTACKER_NOT_FOUND,     // 攻撃者が見つからない
  TARGET_NOT_FOUND,       // 攻撃対象が見つからない
  ATTACKER_CANNOT_ATTACK, // 攻撃者が攻撃できない状態
  TARGET_OUT_OF_RANGE,    // 攻撃対象が射程外
  TARGET_ALREADY_DEAD,    // 攻撃対象が既に死亡
  NO_TARGETS_IN_RANGE     // 射程内に攻撃対象がいない
};

/**
 * @brief 攻撃ユースケース
 *
 * 設計方針：
 * - 攻撃に関するビジネスロジックを集約
 * - CombatSystemエンティティを活用
 * - 自動攻撃と手動攻撃の両方をサポート
 */
class AttackUnitUseCase {
public:
  /**
   * @brief コンストラクタ
   * @param unitRepository ユニットリポジトリ
   * @param combatSystem 戦闘システム
   */
  AttackUnitUseCase(std::shared_ptr<IUnitRepository> unitRepository,
                    std::shared_ptr<CombatSystem> combatSystem)
      : unitRepository_(unitRepository), combatSystem_(combatSystem) {}

  /**
   * @brief 指定したユニットで指定した対象を攻撃
   * @param attackerId 攻撃者のID
   * @param targetId 攻撃対象のID
   * @return 攻撃結果
   */
  std::pair<AttackResult, CombatResult> attackTarget(int attackerId,
                                                     int targetId) {
    // 攻撃者取得
    auto attacker = unitRepository_->findById(attackerId);
    if (!attacker) {
      return {AttackResult::ATTACKER_NOT_FOUND, CombatResult{}};
    }

    // 攻撃対象取得
    auto target = unitRepository_->findById(targetId);
    if (!target) {
      return {AttackResult::TARGET_NOT_FOUND, CombatResult{}};
    }

    // 攻撃可能性チェック
    if (!attacker->canAttack()) {
      return {AttackResult::ATTACKER_CANNOT_ATTACK, CombatResult{}};
    }

    if (!target->isAlive()) {
      return {AttackResult::TARGET_ALREADY_DEAD, CombatResult{}};
    }

    // 射程チェック
    if (!attacker->isInAttackRange(*target)) {
      return {AttackResult::TARGET_OUT_OF_RANGE, CombatResult{}};
    }

    // 戦闘実行
    CombatResult combatResult =
        combatSystem_->executeCombat(*attacker, *target);

    // 結果をリポジトリに保存
    unitRepository_->save(attacker);
    unitRepository_->save(target);

    return {AttackResult::SUCCESS, combatResult};
  }

  /**
   * @brief 自動攻撃（射程内の最も近い敵を攻撃）
   * @param attackerId 攻撃者のID
   * @return 攻撃結果
   */
  std::pair<AttackResult, CombatResult> autoAttack(int attackerId) {
    auto attacker = unitRepository_->findById(attackerId);
    if (!attacker) {
      return {AttackResult::ATTACKER_NOT_FOUND, CombatResult{}};
    }

    if (!attacker->canAttack()) {
      return {AttackResult::ATTACKER_CANNOT_ATTACK, CombatResult{}};
    }

    // 射程内の敵を検索
    auto allUnits = unitRepository_->findAlive();
    auto targetsInRange =
        combatSystem_->findTargetsInRange(*attacker, allUnits);

    if (targetsInRange.empty()) {
      return {AttackResult::NO_TARGETS_IN_RANGE, CombatResult{}};
    }

    // 最も近い敵を選択
    auto nearestTarget =
        combatSystem_->selectNearestTarget(*attacker, targetsInRange);
    if (!nearestTarget) {
      return {AttackResult::NO_TARGETS_IN_RANGE, CombatResult{}};
    }

    // 攻撃実行
    return attackTarget(attackerId, nearestTarget->getId());
  }

  /**
   * @brief 範囲内の攻撃可能な対象を取得
   * @param attackerId 攻撃者のID
   * @return 攻撃可能な対象のリスト
   */
  std::vector<std::shared_ptr<UnitEntity>> getTargetsInRange(int attackerId) {
    auto attacker = unitRepository_->findById(attackerId);
    if (!attacker) {
      return {};
    }

    auto allUnits = unitRepository_->findAlive();
    return combatSystem_->findTargetsInRange(*attacker, allUnits);
  }

  /**
   * @brief 指定したユニットが攻撃可能かチェック
   * @param attackerId 攻撃者のID
   * @param targetId 攻撃対象のID
   * @return 攻撃可能かどうか
   */
  bool canAttackTarget(int attackerId, int targetId) {
    auto attacker = unitRepository_->findById(attackerId);
    auto target = unitRepository_->findById(targetId);

    if (!attacker || !target) {
      return false;
    }

    return attacker->canAttack() && target->isAlive() &&
           attacker->isInAttackRange(*target);
  }

  /**
   * @brief 全ユニットの自動攻撃を処理（AI用）
   * @return 実行された攻撃の数
   */
  int processAutoAttacksForAllUnits() {
    auto allUnits = unitRepository_->findAlive();
    int attacksExecuted = 0;

    for (const auto &unit : allUnits) {
      if (unit->canAttack()) {
        auto result = autoAttack(unit->getId());
        if (result.first == AttackResult::SUCCESS) {
          attacksExecuted++;
        }
      }
    }

    return attacksExecuted;
  }

  /**
   * @brief 戦闘統計を取得
   */
  CombatSystem::CombatStatistics getCombatStatistics() const {
    return combatSystem_->getStatistics();
  }

  /**
   * @brief 戦闘統計をリセット
   */
  void resetCombatStatistics() { combatSystem_->resetStatistics(); }

  /**
   * @brief 攻撃結果を文字列に変換（デバッグ用）
   */
  static std::string attackResultToString(AttackResult result) {
    switch (result) {
    case AttackResult::SUCCESS:
      return "SUCCESS";
    case AttackResult::ATTACKER_NOT_FOUND:
      return "ATTACKER_NOT_FOUND";
    case AttackResult::TARGET_NOT_FOUND:
      return "TARGET_NOT_FOUND";
    case AttackResult::ATTACKER_CANNOT_ATTACK:
      return "ATTACKER_CANNOT_ATTACK";
    case AttackResult::TARGET_OUT_OF_RANGE:
      return "TARGET_OUT_OF_RANGE";
    case AttackResult::TARGET_ALREADY_DEAD:
      return "TARGET_ALREADY_DEAD";
    case AttackResult::NO_TARGETS_IN_RANGE:
      return "NO_TARGETS_IN_RANGE";
    default:
      return "UNKNOWN";
    }
  }

private:
  std::shared_ptr<IUnitRepository> unitRepository_;
  std::shared_ptr<CombatSystem> combatSystem_;
};

#endif // SIMULATION_GAME_ATTACK_UNIT_USECASE_H
