#ifndef SIMULATION_GAME_GAME_FACADE_H
#define SIMULATION_GAME_GAME_FACADE_H

#include "../../domain/entities/CombatSystem.h"
#include "../../domain/entities/UnitEntity.h"
#include "../../domain/value_objects/Position.h"
#include "../interfaces/IUnitRepository.h"
#include "../unit/AttackUnitUseCase.h"
#include "../unit/MoveUnitUseCase.h"
#include <memory>
#include <vector>

/**
 * @brief ゲーム全体の統合窓口（Facade Pattern）
 *
 * 設計方針：
 * - 複数のユースケースを組み合わせた高レベル操作を提供
 * - 外部システム（Renderer、UI）からの単一窓口
 * - ユースケース間の協調処理を管理
 * - ゲームループとの統合を簡化
 */
class GameFacade {
public:
  /**
   * @brief コンストラクタ
   * @param unitRepository ユニットリポジトリ
   * @param combatSystem 戦闘システム
   */
  GameFacade(std::shared_ptr<IUnitRepository> unitRepository,
             std::shared_ptr<CombatSystem> combatSystem)
      : unitRepository_(unitRepository), combatSystem_(combatSystem),
        moveUseCase_(unitRepository),
        attackUseCase_(unitRepository, combatSystem) {

    // 衝突判定の設定
    moveUseCase_.setCollisionRadius(1.0f);
  }

  /**
   * @brief ゲームの初期化
   */
  void initializeGame() {
    // デフォルトユニットの作成
    createPlayerUnit();
    createEnemyUnits();
  }

  /**
   * @brief ゲーム状態の更新（メインループから呼ばれる）
   * @param deltaTime 経過時間（秒）
   */
  void updateGame(float deltaTime) {
    // 1. 全ユニットの移動更新
    updateAllUnitMovements(deltaTime);

    // 2. 自動戦闘処理
    processAutoCombat();

    // 3. ゲーム終了条件チェック
    checkGameEndConditions();
  }

  /**
   * @brief プレイヤーユニットの移動指令
   * @param targetPosition 移動先
   * @return 移動結果
   */
  MoveResult movePlayerUnit(const Position &targetPosition) {
    const int PLAYER_UNIT_ID = 1;
    return moveUseCase_.setTargetPosition(PLAYER_UNIT_ID, targetPosition);
  }

  /**
   * @brief プレイヤーユニットの停止
   */
  MoveResult stopPlayerUnit() {
    auto playerUnit = getPlayerUnit();
    if (!playerUnit) {
      return MoveResult::UNIT_NOT_FOUND;
    }

    // 現在位置を目標位置に設定（停止）
    return moveUseCase_.setTargetPosition(playerUnit->getId(),
                                          playerUnit->getPosition());
  }

  /**
   * @brief プレイヤーユニットの手動攻撃
   * @param targetId 攻撃対象のID
   */
  std::pair<AttackResult, CombatResult> playerAttack(int targetId) {
    const int PLAYER_UNIT_ID = 1;
    return attackUseCase_.attackTarget(PLAYER_UNIT_ID, targetId);
  }

  /**
   * @brief プレイヤーユニットの自動攻撃
   */
  std::pair<AttackResult, CombatResult> playerAutoAttack() {
    const int PLAYER_UNIT_ID = 1;
    return attackUseCase_.autoAttack(PLAYER_UNIT_ID);
  }

  /**
   * @brief 指定位置にユニットを作成
   */
  std::shared_ptr<UnitEntity> createUnit(int id, const std::string &name,
                                         const Position &position,
                                         const UnitStats &stats) {
    auto unit = std::make_shared<UnitEntity>(id, name, position, stats);
    unitRepository_->save(unit);
    return unit;
  }

  /**
   * @brief プレイヤーユニットを取得
   */
  std::shared_ptr<UnitEntity> getPlayerUnit() {
    return unitRepository_->findById(1);
  }

  /**
   * @brief 全ユニットを取得
   */
  std::vector<std::shared_ptr<UnitEntity>> getAllUnits() {
    return unitRepository_->findAll();
  }

  /**
   * @brief 生存ユニットを取得
   */
  std::vector<std::shared_ptr<UnitEntity>> getAliveUnits() {
    return unitRepository_->findAlive();
  }

  /**
   * @brief 指定位置の周辺ユニットを取得
   */
  std::vector<std::shared_ptr<UnitEntity>>
  getUnitsNear(const Position &position, float radius) {
    return unitRepository_->findInRange(position, radius);
  }

  /**
   * @brief ゲーム統計情報を取得
   */
  struct GameStatistics {
    size_t totalUnits;
    size_t aliveUnits;
    size_t playerUnits;
    size_t enemyUnits;
    CombatSystem::CombatStatistics combatStats;
  };

  GameStatistics getGameStatistics() {
    GameStatistics stats{};
    auto allUnits = unitRepository_->findAll();

    stats.totalUnits = allUnits.size();
    stats.aliveUnits = unitRepository_->countAlive();

    // プレイヤー・敵ユニット数カウント
    for (const auto &unit : allUnits) {
      if (unit && unit->isAlive()) {
        if (unit->getId() == 1) {
          stats.playerUnits++;
        } else {
          stats.enemyUnits++;
        }
      }
    }

    stats.combatStats = attackUseCase_.getCombatStatistics();
    return stats;
  }

  /**
   * @brief ゲームをリセット
   */
  void resetGame() {
    unitRepository_->removeAll();
    attackUseCase_.resetCombatStatistics();
    initializeGame();
  }

  /**
   * @brief デバッグ情報を出力
   */
  void printDebugInfo() {
    auto stats = getGameStatistics();

#ifdef DEBUG
    aout << "=== Game Debug Info ===" << std::endl;
    aout << "Total Units: " << stats.totalUnits << std::endl;
    aout << "Alive Units: " << stats.aliveUnits << std::endl;
    aout << "Player Units: " << stats.playerUnits << std::endl;
    aout << "Enemy Units: " << stats.enemyUnits << std::endl;
    aout << "Total Combats: " << stats.combatStats.totalCombats << std::endl;
    aout << "Total Damage: " << stats.combatStats.totalDamageDealt << std::endl;
    aout << "Units Killed: " << stats.combatStats.totalUnitsKilled << std::endl;
    aout << "======================" << std::endl;
#endif
  }

private:
  std::shared_ptr<IUnitRepository> unitRepository_;
  std::shared_ptr<CombatSystem> combatSystem_;
  MoveUnitUseCase moveUseCase_;
  AttackUnitUseCase attackUseCase_;

  /**
   * @brief プレイヤーユニットを作成
   */
  void createPlayerUnit() {
    Position playerPos(0.0f, 0.0f);
    UnitStats playerStats = UnitStats::createStrong(); // 強めのステータス

    createUnit(1, "Player", playerPos, playerStats);
  }

  /**
   * @brief 敵ユニットを作成
   */
  void createEnemyUnits() {
    // 敵ユニット1
    Position enemy1Pos(10.0f, 10.0f);
    UnitStats enemy1Stats = UnitStats::createDefault();
    createUnit(2, "Enemy1", enemy1Pos, enemy1Stats);

    // 敵ユニット2
    Position enemy2Pos(-10.0f, 10.0f);
    UnitStats enemy2Stats = UnitStats::createDefault();
    createUnit(3, "Enemy2", enemy2Pos, enemy2Stats);

    // 敵ユニット3（強敵）
    Position enemy3Pos(0.0f, 15.0f);
    UnitStats enemy3Stats = UnitStats::createStrong();
    createUnit(4, "BossEnemy", enemy3Pos, enemy3Stats);
  }

  /**
   * @brief 全ユニットの移動を更新
   */
  void updateAllUnitMovements(float deltaTime) {
    auto allUnits = unitRepository_->findAll();

    for (const auto &unit : allUnits) {
      if (unit && unit->getState() == UnitState::MOVING) {
        moveUseCase_.updatePosition(unit->getId(), deltaTime);
      }
    }
  }

  /**
   * @brief 自動戦闘処理
   */
  void processAutoCombat() {
    // プレイヤー以外のユニットは自動攻撃
    auto allUnits = unitRepository_->findAll();

    for (const auto &unit : allUnits) {
      if (unit && unit->isAlive() && unit->getId() != 1) { // プレイヤー以外
        attackUseCase_.autoAttack(unit->getId());
      }
    }
  }

  /**
   * @brief ゲーム終了条件をチェック
   */
  void checkGameEndConditions() {
    auto playerUnit = getPlayerUnit();

    // プレイヤーが死亡した場合
    if (!playerUnit || !playerUnit->isAlive()) {
      onPlayerDeath();
      return;
    }

    // 敵が全滅した場合
    auto aliveUnits = unitRepository_->findAlive();
    bool hasEnemies = false;
    for (const auto &unit : aliveUnits) {
      if (unit->getId() != 1) { // プレイヤー以外
        hasEnemies = true;
        break;
      }
    }

    if (!hasEnemies) {
      onPlayerVictory();
    }
  }

  /**
   * @brief プレイヤー死亡時の処理
   */
  void onPlayerDeath() {
#ifdef DEBUG
    aout << "Game Over - Player Defeated!" << std::endl;
#endif
    // ここでゲームオーバー処理を追加
  }

  /**
   * @brief プレイヤー勝利時の処理
   */
  void onPlayerVictory() {
#ifdef DEBUG
    aout << "Victory - All Enemies Defeated!" << std::endl;
#endif
    // ここで勝利処理を追加
  }
};

#endif // SIMULATION_GAME_GAME_FACADE_H
