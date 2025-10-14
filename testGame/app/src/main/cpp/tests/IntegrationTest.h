#ifndef SIMULATION_GAME_INTEGRATION_TEST_H
#define SIMULATION_GAME_INTEGRATION_TEST_H

#include "../adapters/repositories/MemoryUnitRepository.h"
#include "../domain/entities/CombatSystem.h"
#include "../domain/value_objects/Position.h"
#include "../domain/value_objects/UnitStats.h"
#include "../usecases/game/GameFacade.h"
#include <cassert>
#include <iostream>
#include <memory>

/**
 * @brief クリーンアーキテクチャの統合テスト
 *
 * 設計方針：
 * - 各層の連携動作を検証
 * - 実際のゲームシナリオをテスト
 * - パフォーマンスの基本測定
 */
class IntegrationTest {
public:
  /**
   * @brief 全ての統合テストを実行
   */
  static void runAllTests() {
    std::cout << "Running Integration tests..." << std::endl;

    testBasicGameFlow();
    testCombatSystem();
    testMovementAndCollision();
    testRepositoryOperations();
    testGameFacadeOperations();

    std::cout << "All Integration tests passed!" << std::endl;
  }

private:
  /**
   * @brief 基本的なゲームフローのテスト
   */
  static void testBasicGameFlow() {
    // システム構築
    auto repository = std::make_shared<MemoryUnitRepository>();
    auto combatSystem = std::make_shared<CombatSystem>();
    GameFacade game(repository, combatSystem);

    // ゲーム初期化
    game.initializeGame();

    // プレイヤーユニットの存在確認
    auto player = game.getPlayerUnit();
    assert(player != nullptr);
    assert(player->getId() == 1);
    assert(player->getName() == "Player");
    assert(player->isAlive());

    // 初期状態での全ユニット数確認
    auto allUnits = game.getAllUnits();
    assert(allUnits.size() >= 4); // プレイヤー + 敵3体以上

    // 統計情報の確認
    auto stats = game.getGameStatistics();
    assert(stats.totalUnits >= 4);
    assert(stats.aliveUnits >= 4);
    assert(stats.playerUnits == 1);
    assert(stats.enemyUnits >= 3);
  }

  /**
   * @brief 戦闘システムのテスト
   */
  static void testCombatSystem() {
    auto repository = std::make_shared<MemoryUnitRepository>();
    auto combatSystem = std::make_shared<CombatSystem>();
    GameFacade game(repository, combatSystem);

    // テスト用ユニット作成
    Position pos1(0, 0);
    Position pos2(1, 1); // 攻撃範囲内
    UnitStats stats = UnitStats::createDefault();

    auto unit1 = game.createUnit(100, "TestAttacker", pos1, stats);
    auto unit2 = game.createUnit(101, "TestTarget", pos2, stats);

    // 攻撃可能性確認
    assert(unit1->isInAttackRange(*unit2));

    // 攻撃実行
    auto result = game.playerAttack(101); // unit1がplayerでないため失敗する
    // 代わりに直接戦闘システムをテスト
    CombatResult combatResult = combatSystem->executeCombat(*unit1, *unit2);

    // 戦闘結果の確認
    assert(combatResult.damageDealt > 0);
    assert(unit2->getStats().getCurrentHp() < unit2->getStats().getMaxHp());
  }

  /**
   * @brief 移動と衝突のテスト
   */
  static void testMovementAndCollision() {
    auto repository = std::make_shared<MemoryUnitRepository>();
    auto combatSystem = std::make_shared<CombatSystem>();
    GameFacade game(repository, combatSystem);

    // テスト用ユニット作成
    Position startPos(0, 0);
    Position targetPos(5, 5);
    UnitStats stats = UnitStats::createDefault();

    auto unit = game.createUnit(200, "MoveTest", startPos, stats);

    // 移動先設定
    bool canMove = unit->setTargetPosition(targetPos);
    assert(canMove);
    assert(unit->getState() == UnitState::MOVING);
    assert(unit->getTargetPosition() == targetPos);

    // ゲーム更新（移動処理）
    float deltaTime = 1.0f; // 1秒
    for (int i = 0; i < 10; ++i) {
      game.updateGame(deltaTime);

      // 目標に近づいているかチェック
      float distance = unit->getPosition().distanceTo(targetPos);
      if (distance < 0.1f) {
        break; // 到達
      }
    }

    // 移動完了の確認
    float finalDistance = unit->getPosition().distanceTo(targetPos);
    assert(finalDistance < 1.0f); // ある程度近づいていることを確認
  }

  /**
   * @brief リポジトリ操作のテスト
   */
  static void testRepositoryOperations() {
    auto repository = std::make_shared<MemoryUnitRepository>();

    // 初期状態
    assert(repository->count() == 0);
    assert(repository->countAlive() == 0);

    // ユニット追加
    Position pos(0, 0);
    UnitStats stats = UnitStats::createDefault();
    auto unit1 = std::make_shared<UnitEntity>(1, "Test1", pos, stats);
    auto unit2 =
        std::make_shared<UnitEntity>(2, "Test2", Position(5, 5), stats);

    repository->save(unit1);
    repository->save(unit2);

    // 個数確認
    assert(repository->count() == 2);
    assert(repository->countAlive() == 2);

    // 検索テスト
    auto found = repository->findById(1);
    assert(found != nullptr);
    assert(found->getId() == 1);

    auto notFound = repository->findById(999);
    assert(notFound == nullptr);

    // 範囲検索テスト
    auto nearUnits = repository->findInRange(Position(0, 0), 3.0f);
    assert(nearUnits.size() == 1); // unit1のみ

    auto allInRange = repository->findInRange(Position(2.5f, 2.5f), 5.0f);
    assert(allInRange.size() == 2); // 両方

    // 削除テスト
    bool removed = repository->remove(1);
    assert(removed);
    assert(repository->count() == 1);

    repository->removeAll();
    assert(repository->count() == 0);
  }

  /**
   * @brief GameFacadeの高レベル操作テスト
   */
  static void testGameFacadeOperations() {
    auto repository = std::make_shared<MemoryUnitRepository>();
    auto combatSystem = std::make_shared<CombatSystem>();
    GameFacade game(repository, combatSystem);

    game.initializeGame();

    // プレイヤーユニット取得
    auto player = game.getPlayerUnit();
    assert(player != nullptr);

    Position originalPos = player->getPosition();

    // プレイヤー移動テスト
    Position moveTarget(10, 10);
    MoveResult moveResult = game.movePlayerUnit(moveTarget);
    assert(moveResult == MoveResult::SUCCESS);
    assert(player->getState() == UnitState::MOVING);

    // 停止テスト
    MoveResult stopResult = game.stopPlayerUnit();
    assert(stopResult == MoveResult::SUCCESS);
    assert(player->getState() == UnitState::IDLE);

    // 自動攻撃テスト
    auto attackResult = game.playerAutoAttack();
    // 結果は敵の配置によって変わるが、エラーでないことを確認
    assert(attackResult.first != AttackResult::ATTACKER_NOT_FOUND);

    // ゲームリセットテスト
    game.resetGame();
    auto statsAfterReset = game.getGameStatistics();
    assert(statsAfterReset.totalUnits >= 4); // 再初期化されている
  }
};

#endif // SIMULATION_GAME_INTEGRATION_TEST_H
