#include "MovementUseCase.h"
#include "../domain/services/MovementField.h"
#include "../frameworks/android/AndroidOut.h"
#include <algorithm>
#include <chrono>
#include <cmath>

MovementUseCase::MovementUseCase(
    std::vector<std::shared_ptr<UnitEntity>> &units,
    const MovementField *movementField, const GameMap *gameMap)
    : units_(units), movementField_(movementField), gameMap_(gameMap),
      movementEnabled_(true) {}

void MovementUseCase::setMovementEventCallback(MovementEventCallback callback) {
  movementEventCallback_ = callback;
}

void MovementUseCase::setMovementFailedCallback(
    MovementFailedCallback callback) {
  movementFailedCallback_ = callback;
}

bool MovementUseCase::moveUnitTo(int unitId, const Position &targetPosition) {
  if (!movementEnabled_) {
    if (movementFailedCallback_) {
      auto unit = findUnitById(unitId);
      if (unit) {
        movementFailedCallback_(*unit, targetPosition,
                                "Unit movement is currently disabled");
      }
    }
    return false;
  }

  auto unit = findUnitById(unitId);
  if (!unit) {
    return false;
  }

  if (unit->getStats().getCurrentHp() <= 0) {
    if (movementFailedCallback_) {
      movementFailedCallback_(*unit, targetPosition, "Unit is dead");
    }
    return false;
  }

  auto otherUnits = getOtherUnits(*unit);

  Position boundedTarget = applyBounds(*unit, targetPosition);
  Position terrainAwareTarget = boundedTarget;
  
  // レイキャストで途中の停止タイルをチェック
  if (gameMap_) {
    const float radius = unit->getStats().getCollisionRadius();
    auto rayResult = gameMap_->clipMovementRaycast(unit->getPosition(),
                                                   boundedTarget, radius);
    terrainAwareTarget = rayResult.position;
    
    if (rayResult.hitBlocking) {
      // 停止タイルに遮られた場合、到達可能な位置を最終目標とする
      if (movementFailedCallback_) {
        movementFailedCallback_(*unit, targetPosition,
                                "Path blocked by terrain");
      }
      // 遮られてもその手前までは移動可能なので続行
    }
    
    if (!gameMap_->isWalkable(terrainAwareTarget, radius)) {
      if (movementFailedCallback_) {
        movementFailedCallback_(*unit, targetPosition,
                                "Target blocked by terrain");
      }
      return false;
    }
  }

  Position avoidanceTarget = CollisionDomainService::calculateAvoidancePosition(
      *unit, terrainAwareTarget, otherUnits);

  avoidanceTarget = applyBounds(*unit, avoidanceTarget);
  if (gameMap_) {
    const float radius = unit->getStats().getCollisionRadius();
    auto rayResult = gameMap_->clipMovementRaycast(unit->getPosition(),
                                                   avoidanceTarget, radius);
    avoidanceTarget = rayResult.position;
  }

  Position fromPosition = unit->getPosition();
  const float travelDistance = fromPosition.distanceTo(avoidanceTarget);
  if (travelDistance <= 1e-4f) {
    if (movementFailedCallback_) {
      movementFailedCallback_(*unit, targetPosition, "No viable path found");
    }
    return false;
  }

  // 移動命令前の状態をログ出力
  aout << "MovementUseCase::moveUnitTo Unit " << unit->getId()
       << " state=" << unit->getStateString()
       << " from=(" << fromPosition.getX() << ", " << fromPosition.getY() << ")"
       << " to=(" << avoidanceTarget.getX() << ", " << avoidanceTarget.getY() << ")"
       << std::endl;

  bool setResult = unit->setTargetPosition(avoidanceTarget);
  
  // 新しい移動命令を受け取ったら、1秒間は攻撃意思を抑制
  // これにより移動中に敵を無視して通過できる
  if (setResult) {
    auto now = std::chrono::high_resolution_clock::now();
    float nowSec = std::chrono::duration<float>(now.time_since_epoch()).count();
    unit->suppressAttackFor(nowSec, 1.0f); // 1秒間攻撃意思を抑制
    
    aout << "MovementUseCase::moveUnitTo attack suppressed for 1 second" << std::endl;
  }
  
  aout << "MovementUseCase::moveUnitTo setTargetPosition result=" 
       << (setResult ? "success" : "failed")
       << " newState=" << unit->getStateString()
       << std::endl;

  if (!setResult) {
    if (movementFailedCallback_) {
      movementFailedCallback_(*unit, targetPosition, 
                              "setTargetPosition failed - unit cannot move in current state");
    }
    return false;
  }

  if (movementEventCallback_) {
    movementEventCallback_(*unit, fromPosition, avoidanceTarget);
  }

  if (gameMap_ && targetPosition.distanceTo(avoidanceTarget) > 0.05f) {
    aout << "MovementUseCase: adjusted target due to terrain from ("
         << targetPosition.getX() << ", " << targetPosition.getY() << ") to ("
         << avoidanceTarget.getX() << ", " << avoidanceTarget.getY() << ")"
         << std::endl;
  }

  return true;
}

void MovementUseCase::updateMovements(float deltaTime) {
  // 現在時刻を取得（攻撃意思のチェックに使用）
  auto now = std::chrono::high_resolution_clock::now();
  float nowSec = std::chrono::duration<float>(now.time_since_epoch()).count();
  
  for (auto &unit : units_) {
    if (!unit || unit->getStats().getCurrentHp() <= 0) {
      continue;
    }
    // 移動が必要かチェック：目標位置と現在位置が異なる場合のみ処理
    // これにより、MOVING状態でもCOMBAT状態でも、移動命令があれば移動できる
    if (unit->getPosition() == unit->getTargetPosition()) {
      continue; // 移動の必要なし
    }
    
    // デバッグ：移動処理開始
    bool wantsAttack = unit->wantsToAttack(nowSec);
    aout << "MovementUseCase::updateMovements: Unit " << unit->getId()
         << " state=" << unit->getStateString()
         << " wantsAttack=" << (wantsAttack ? "YES" : "NO")
         << " pos=(" << unit->getPosition().getX() << ", " << unit->getPosition().getY() << ")"
         << " target=(" << unit->getTargetPosition().getX() << ", " << unit->getTargetPosition().getY() << ")"
         << std::endl;

    // 敵が攻撃範囲に入った時の自動停止は、MOVING状態で攻撃意思がある場合のみ適用
    // 攻撃したくない状態の場合は、敵を無視して移動を継続
    if (unit->getState() == UnitState::MOVING && wantsAttack) {
      auto enemyInRange = findEnemyInAttackRange(*unit);
      if (enemyInRange) {
        // 攻撃範囲ギリギリの位置を計算（現在位置を返す）
        Position attackRangePos = calculateAttackRangePosition(*unit, *enemyInRange);
        Position currentPos = unit->getPosition();
        float distanceToEnemy = currentPos.distanceTo(enemyInRange->getPosition());
        float distanceToStop = currentPos.distanceTo(attackRangePos);
        
        // 現在位置で停止するため、目標位置を現在位置に設定してからCOMBAT状態に遷移
        // setTargetPosition()を使わず、直接状態を変更
        unit->enterCombat(); // 戦闘状態に遷移（これによりその場で停止）
        
        aout << "MovementUseCase: Unit " << unit->getId() 
             << " AUTO-STOPPED - enemy " << enemyInRange->getId() 
             << " in attack range (distance to enemy: " << distanceToEnemy << ")"
             << std::endl
             << "  Current pos: (" << currentPos.getX() << ", " << currentPos.getY() << ")"
             << " -> Stop pos: (" << attackRangePos.getX() << ", " << attackRangePos.getY() << ")"
             << " (distance to stop: " << distanceToStop << ")"
             << std::endl
             << "  Attack range: " << unit->getStats().getAttackRange()
             << ", State changed to COMBAT"
             << std::endl;
        
        // 移動処理をスキップ（その場で停止）
        continue;
      }
    }

    unit->setTargetPosition(applyBounds(*unit, unit->getTargetPosition()));

    Position currentPos = unit->getPosition();
    Position nextPos = calculateNextPosition(*unit, deltaTime);

    auto logFinalMovement = [&](const Position &destination,
                                const char *reason) {
      if (currentPos == destination) {
        return;
      }
      const float baseSpeed = unit->getStats().getMoveSpeed();
    const float multiplier =
      terrainSpeedMultiplier(*unit, currentPos);
      const float effectiveSpeed = baseSpeed * multiplier;

      aout << "MovementUseCase::updateMovements unit=" << unit->getId()
           << " reason=" << reason << " from=(" << currentPos.getX() << ", "
           << currentPos.getY() << ")" << " to=(" << destination.getX() << ", "
           << destination.getY() << ")" << " baseSpeed=" << baseSpeed
           << " terrainMultiplier=" << multiplier
           << " effectiveSpeed=" << effectiveSpeed << " deltaTime=" << deltaTime
           << std::endl;
    };

    auto otherUnits = getOtherUnits(*unit);
    float movingRadius = unit->getStats().getCollisionRadius();
    Position contactPos = currentPos;
    const UnitEntity *contactUnit = nullptr;
    bool hasContact = CollisionDomainService::findFirstContactOnPath(
        currentPos, nextPos, otherUnits, contactPos, contactUnit, unit.get(),
        movingRadius);

    if (hasContact) {
      constexpr float kEpsT = 1e-3f;
      constexpr float kBackoffDistance = 0.02f;

      float segmentLen = currentPos.distanceTo(nextPos);
      float t = 0.0f;
      if (segmentLen > 1e-6f) {
        t = currentPos.distanceTo(contactPos) / segmentLen;
      }

      float dirX = 0.0f;
      float dirY = 0.0f;
      if (segmentLen > 1e-6f) {
        dirX = (nextPos.getX() - currentPos.getX()) / segmentLen;
        dirY = (nextPos.getY() - currentPos.getY()) / segmentLen;
      }

      bool currentlyOverlapping = CollisionDomainService::hasCollisionAt(
          currentPos, otherUnits, unit.get(), movingRadius);

      if (currentlyOverlapping) {
        Position backPos = currentPos.moveBy(-dirX * kBackoffDistance,
                                             -dirY * kBackoffDistance);
        backPos = resolveTerrainConstraints(*unit, currentPos, backPos);
        if (CollisionDomainService::hasCollisionAt(backPos, otherUnits,
                                                   unit.get(), movingRadius)) {
          Position safePos = CollisionDomainService::calculateAvoidancePosition(
              *unit, currentPos, otherUnits);
          safePos = resolveTerrainConstraints(*unit, currentPos, safePos);
          unit->setTargetPosition(safePos);
          unit->updatePosition(safePos);
          logFinalMovement(safePos, "avoidance");
        } else {
          unit->setTargetPosition(backPos);
          unit->updatePosition(backPos);
          logFinalMovement(backPos, "backoff");
        }
      } else if (t <= kEpsT) {
        Position stopBefore = contactPos.moveBy(-dirX * kBackoffDistance,
                                                -dirY * kBackoffDistance);
        stopBefore = resolveTerrainConstraints(*unit, currentPos, stopBefore);
        unit->setTargetPosition(stopBefore);
        unit->updatePosition(stopBefore);
        logFinalMovement(stopBefore, "collision-stop");
      } else {
        Position adjustedContact =
            resolveTerrainConstraints(*unit, currentPos, contactPos);
        unit->setTargetPosition(adjustedContact);
        unit->updatePosition(adjustedContact);
        logFinalMovement(adjustedContact, "collision-adjusted");
      }
    } else {
      Position constrainedNext = nextPos;
      const char *moveReason = "direct-move";
      if (gameMap_) {
        Position clipped = clipMovementToTerrain(*unit, currentPos, nextPos);
        if (clipped != nextPos) {
          constrainedNext = clipped;
          moveReason = "terrain-contact";
          unit->setTargetPosition(constrainedNext);
        }
      }

      unit->updatePosition(constrainedNext);
      logFinalMovement(constrainedNext, moveReason);
    }
  }
}

Position MovementUseCase::calculateNextPosition(const UnitEntity &unit,
                                                float deltaTime) const {
  Position currentPos = unit.getPosition();
  Position targetPos = applyBounds(unit, unit.getTargetPosition());

  if (gameMap_) {
    targetPos = gameMap_->resolveMovementTarget(
        currentPos, targetPos, unit.getStats().getCollisionRadius());
  }

  const float distance = currentPos.distanceTo(targetPos);
  if (distance <= 0.001f || deltaTime <= 0.0f) {
      Position clipped = targetPos;
      if (gameMap_) {
        clipped = clipMovementToTerrain(unit, currentPos, targetPos);
      }
      return clipped;
  }

  // 現在位置の地形倍率を取得（ユニットが立っている地形が移動速度を決める）
  const float speedMultiplier = terrainSpeedMultiplier(unit, currentPos);
  const float baseSpeed = unit.getStats().getMoveSpeed();
  const float effectiveSpeed = baseSpeed * speedMultiplier;
  
  if (effectiveSpeed <= 0.0f) {
    return currentPos;
  }

  const float maxDistance = std::max(0.0f, effectiveSpeed * deltaTime);
  if (maxDistance <= 0.0f) {
    return currentPos;
  }

  // travelDistance は今回のフレームで進める距離（地形倍率を反映済み）
  const float travelDistance = std::min(distance, maxDistance);
  if (travelDistance <= 0.0f) {
    return currentPos;
  }

  if (travelDistance >= distance - 1e-5f) {
    if (gameMap_) {
      Position clipped = clipMovementToTerrain(unit, currentPos, targetPos);
      return applyBounds(unit, clipped);
    }
    return applyBounds(unit, targetPos);
  }

  const float stepRatio = travelDistance / distance;
  Position candidate(
      currentPos.getX() + (targetPos.getX() - currentPos.getX()) * stepRatio,
      currentPos.getY() + (targetPos.getY() - currentPos.getY()) * stepRatio);

  if (gameMap_) {
    candidate = gameMap_->resolveMovementTarget(
        currentPos, candidate, unit.getStats().getCollisionRadius());
    candidate = clipMovementToTerrain(unit, currentPos, candidate);
  }

  return applyBounds(unit, candidate);
}

size_t MovementUseCase::getMovingUnitsCount() const {
  return std::count_if(units_.begin(), units_.end(),
                       [](const std::shared_ptr<UnitEntity> &unit) {
                         return unit->getStats().getCurrentHp() > 0 &&
                                unit->getState() == UnitState::MOVING;
                       });
}

bool MovementUseCase::canMoveToPosition(int unitId,
                                        const Position &targetPosition) const {
  auto unit = findUnitById(unitId);
  if (!unit) {
    return false;
  }

  if (unit->getStats().getCurrentHp() <= 0) {
    return false; // 死亡ユニットは移動できない
  }

  auto otherUnits = getOtherUnits(*unit);

  Position boundedTarget = applyBounds(*unit, targetPosition);
  if (gameMap_) {
    boundedTarget =
        gameMap_->resolveMovementTarget(unit->getPosition(), boundedTarget,
                                        unit->getStats().getCollisionRadius());
    if (!gameMap_->isWalkable(boundedTarget,
                              unit->getStats().getCollisionRadius())) {
      return false;
    }
  }

  return CollisionDomainService::canMoveTo(*unit, boundedTarget, otherUnits);
}

std::shared_ptr<UnitEntity> MovementUseCase::findUnitById(int unitId) {
  auto it = std::find_if(units_.begin(), units_.end(),
                         [unitId](const std::shared_ptr<UnitEntity> &unit) {
                           return unit->getId() == unitId;
                         });

  return (it != units_.end()) ? *it : nullptr;
}

std::shared_ptr<UnitEntity> MovementUseCase::findUnitById(int unitId) const {
  auto it = std::find_if(units_.begin(), units_.end(),
                         [unitId](const std::shared_ptr<UnitEntity> &unit) {
                           return unit->getId() == unitId;
                         });

  return (it != units_.end()) ? *it : nullptr;
}

std::vector<std::shared_ptr<UnitEntity>>
MovementUseCase::getOtherUnits(const UnitEntity &excludeUnit) const {
  std::vector<std::shared_ptr<UnitEntity>> otherUnits;

  for (const auto &unit : units_) {
    if (unit->getId() != excludeUnit.getId() &&
        unit->getStats().getCurrentHp() > 0) {
      otherUnits.push_back(unit);
    }
  }

  return otherUnits;
}

void MovementUseCase::setMovementEnabled(bool enabled,
                                         const std::string &reason) {
  if (movementEnabled_ != enabled) {
    movementEnabled_ = enabled;
    // ログ出力は AndroidOut を使用しないため、コンソール出力はしない
    // 必要に応じて movementFailedCallback_ 等で通知
  }
}

bool MovementUseCase::isMovementEnabled() const { return movementEnabled_; }

Position MovementUseCase::applyBounds(const UnitEntity &unit,
                                      const Position &desired) const {
  Position bounded = desired;
  if (movementField_) {
    bounded = movementField_->snapInside(bounded);
  }
  if (gameMap_) {
    bounded =
        gameMap_->clampInside(bounded, unit.getStats().getCollisionRadius());
  }
  return bounded;
}

float MovementUseCase::terrainSpeedMultiplier(const UnitEntity &unit,
                                              const Position &position) const {
  if (!gameMap_) {
    return 1.0f;
  }
  // ユニットの衝突半径が侵入する全タイルを評価し、最も厳しい地形倍率を採用する
  const float radius = unit.getStats().getCollisionRadius();
  return std::max(0.0f, gameMap_->getMovementMultiplier(position, radius));
}

Position
MovementUseCase::resolveTerrainConstraints(const UnitEntity &unit,
                                           const Position &start,
                                           const Position &desired) const {
  Position bounded = applyBounds(unit, desired);
  if (gameMap_) {
    return gameMap_->resolveMovementTarget(
        start, bounded, unit.getStats().getCollisionRadius());
  }
  return bounded;
}

Position MovementUseCase::clipMovementToTerrain(const UnitEntity &unit,
                                                const Position &start,
                                                const Position &desired) const {
  if (!gameMap_) {
    return desired;
  }
  const float radius = unit.getStats().getCollisionRadius();
  auto rayResult = gameMap_->clipMovementRaycast(start, desired, radius);
  return rayResult.position;
}

std::shared_ptr<UnitEntity>
MovementUseCase::findEnemyInAttackRange(const UnitEntity &unit) const {
  // 攻撃範囲内の敵ユニットを検索
  for (const auto &otherUnit : units_) {
    if (!otherUnit || otherUnit->getId() == unit.getId()) {
      continue; // 自分自身は除外
    }

    if (otherUnit->getStats().getCurrentHp() <= 0) {
      continue; // 死亡ユニットは除外
    }

    // 同陣営は攻撃対象外
    if (otherUnit->getFaction() == unit.getFaction()) {
      continue;
    }

    // 攻撃範囲チェック（衝突半径を考慮）
    if (unit.isInAttackRange(*otherUnit)) {
      return otherUnit;
    }
  }

  return nullptr;
}

Position MovementUseCase::calculateAttackRangePosition(
    const UnitEntity &unit, const UnitEntity &enemy) const {
  // 攻撃範囲の判定は既に isInAttackRange() で行われている
  // 条件: 距離 <= 攻撃範囲 + 敵の衝突半径
  // この条件を満たした時点で停止すればよいので、現在位置をそのまま返す
  
  Position currentPos = unit.getPosition();
  Position enemyPos = enemy.getPosition();
  
  float distance = currentPos.distanceTo(enemyPos);
  float attackRange = unit.getStats().getAttackRange();
  float enemyRadius = enemy.getStats().getCollisionRadius();
  
  aout << "calculateAttackRangePosition: distance=" << distance
       << ", attackRange=" << attackRange 
       << ", enemyRadius=" << enemyRadius
       << ", threshold=" << (attackRange + enemyRadius)
       << std::endl;
  
  // 既に攻撃範囲内なので、現在位置で停止
  return currentPos;
}
