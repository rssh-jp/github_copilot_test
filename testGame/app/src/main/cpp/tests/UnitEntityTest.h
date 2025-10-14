#ifndef SIMULATION_GAME_UNIT_ENTITY_TEST_H
#define SIMULATION_GAME_UNIT_ENTITY_TEST_H

#include "../domain/entities/UnitEntity.h"
#include "../domain/value_objects/Position.h"
#include "../domain/value_objects/UnitStats.h"
#include <cassert>
#include <iostream>

/**
 * @brief UnitEntityのユニットテスト
 *
 * 設計方針：
 * - 軽量なテストフレームワーク（assertベース）
 * - ドメインロジックの検証
 * - リーダブルなテストメソッド名
 */
class UnitEntityTest {
public:
  /**
   * @brief 全てのテストを実行
   */
  static void runAllTests() {
    std::cout << "Running UnitEntity tests..." << std::endl;

    testConstructorAndBasicGetters();
    testMovementLogic();
    testCombatLogic();
    testDamageAndHealing();
    testStateTransitions();
    testAttackRange();

    std::cout << "All UnitEntity tests passed!" << std::endl;
  }

private:
  /**
   * @brief コンストラクタと基本ゲッターのテスト
   */
  static void testConstructorAndBasicGetters() {
    Position pos(10.0f, 20.0f);
    UnitStats stats = UnitStats::createDefault();
    UnitEntity unit(1, "TestUnit", pos, stats);

    assert(unit.getId() == 1);
    assert(unit.getName() == "TestUnit");
    assert(unit.getPosition() == pos);
    assert(unit.getStats() == stats);
    assert(unit.getState() == UnitState::IDLE);
    assert(unit.isAlive() == true);
    assert(unit.canMove() == true);
    assert(unit.canAttack() == true);
  }

  /**
   * @brief 移動ロジックのテスト
   */
  static void testMovementLogic() {
    Position startPos(0.0f, 0.0f);
    UnitStats stats = UnitStats::createDefault();
    UnitEntity unit(1, "MoveTest", startPos, stats);

    // 移動先設定テスト
    Position targetPos(5.0f, 5.0f);
    bool canSetTarget = unit.setTargetPosition(targetPos);
    assert(canSetTarget == true);
    assert(unit.getTargetPosition() == targetPos);
    assert(unit.getState() == UnitState::MOVING);

    // 移動完了テスト
    unit.updatePosition(targetPos);
    assert(unit.getPosition() == targetPos);
    assert(unit.getState() == UnitState::IDLE);

    // 移動距離チェックテスト
    Position farPos(100.0f, 100.0f);
    bool canMoveToFar = unit.canMoveTo(farPos, 0.1f); // 短時間での長距離移動
    assert(canMoveToFar == false);

    Position nearPos(1.0f, 1.0f);
    bool canMoveToNear = unit.canMoveTo(nearPos, 1.0f); // 十分な時間
    assert(canMoveToNear == true);
  }

  /**
   * @brief 戦闘ロジックのテスト
   */
  static void testCombatLogic() {
    Position pos(0.0f, 0.0f);
    UnitStats stats = UnitStats::createDefault();
    UnitEntity unit(1, "CombatTest", pos, stats);

    // 戦闘状態テスト
    unit.enterCombat();
    assert(unit.getState() == UnitState::COMBAT);
    assert(unit.canMove() == false);
    assert(unit.canAttack() == true);

    // 戦闘離脱テスト
    unit.exitCombat();
    assert(unit.getState() == UnitState::IDLE);
    assert(unit.canMove() == true);

    // 移動中の戦闘離脱テスト
    Position target(5.0f, 5.0f);
    unit.setTargetPosition(target);
    unit.enterCombat();
    unit.exitCombat();
    assert(unit.getState() == UnitState::MOVING);
  }

  /**
   * @brief ダメージと回復のテスト
   */
  static void testDamageAndHealing() {
    Position pos(0.0f, 0.0f);
    UnitStats stats(100, 100, 20, 1.0f, 2.0f); // maxHP=100, currentHP=100
    UnitEntity unit(1, "DamageTest", pos, stats);

    // ダメージテスト
    bool alive = unit.takeDamage(30);
    assert(alive == true);
    assert(unit.getStats().getCurrentHp() == 70);
    assert(unit.isAlive() == true);

    // 致命的ダメージテスト
    bool stillAlive = unit.takeDamage(80);
    assert(stillAlive == false);
    assert(unit.getStats().getCurrentHp() == 0);
    assert(unit.isAlive() == false);
    assert(unit.getState() == UnitState::DEAD);
    assert(unit.canMove() == false);
    assert(unit.canAttack() == false);

    // 死亡ユニットの回復テスト（回復しない）
    unit.heal(50);
    assert(unit.getStats().getCurrentHp() == 0);
    assert(unit.isAlive() == false);

    // 新しいユニットで回復テスト
    UnitEntity healUnit(2, "HealTest", pos, stats);
    healUnit.takeDamage(50);
    assert(healUnit.getStats().getCurrentHp() == 50);

    healUnit.heal(30);
    assert(healUnit.getStats().getCurrentHp() == 80);

    // 過剰回復テスト（最大HPを超えない）
    healUnit.heal(50);
    assert(healUnit.getStats().getCurrentHp() == 100);
  }

  /**
   * @brief 状態遷移のテスト
   */
  static void testStateTransitions() {
    Position pos(0.0f, 0.0f);
    UnitStats stats = UnitStats::createDefault();
    UnitEntity unit(1, "StateTest", pos, stats);

    // IDLE -> MOVING
    Position target(5.0f, 5.0f);
    unit.setTargetPosition(target);
    assert(unit.getState() == UnitState::MOVING);

    // MOVING -> COMBAT
    unit.enterCombat();
    assert(unit.getState() == UnitState::COMBAT);

    // COMBAT -> MOVING (移動中だった場合)
    unit.exitCombat();
    assert(unit.getState() == UnitState::MOVING);

    // MOVING -> IDLE (目標到達)
    unit.updatePosition(target);
    assert(unit.getState() == UnitState::IDLE);

    // 生きているユニットの強制死亡
    unit.setState(UnitState::DEAD);
    assert(unit.getState() == UnitState::DEAD);
  }

  /**
   * @brief 攻撃範囲のテスト
   */
  static void testAttackRange() {
    Position pos1(0.0f, 0.0f);
    Position pos2(1.5f, 1.5f); // 距離 ≈ 2.12
    Position pos3(5.0f, 5.0f); // 距離 ≈ 7.07

    UnitStats stats(100, 100, 20, 1.0f, 2.0f); // 攻撃範囲 = 2.0
    UnitEntity unit1(1, "AttackRangeTest1", pos1, stats);
    UnitEntity unit2(2, "AttackRangeTest2", pos2, stats);
    UnitEntity unit3(3, "AttackRangeTest3", pos3, stats);

    // 範囲内攻撃テスト
    bool canAttackNear = unit1.isInAttackRange(unit2);
    assert(canAttackNear == false); // 2.12 > 2.0なので範囲外

    // 範囲外攻撃テスト
    bool canAttackFar = unit1.isInAttackRange(unit3);
    assert(canAttackFar == false);

    // 位置での攻撃範囲テスト
    Position nearPos(1.0f, 1.0f); // 距離 ≈ 1.41
    bool canAttackPos = unit1.isInAttackRange(nearPos);
    assert(canAttackPos == true); // 1.41 < 2.0なので範囲内
  }
};

#endif // SIMULATION_GAME_UNIT_ENTITY_TEST_H
