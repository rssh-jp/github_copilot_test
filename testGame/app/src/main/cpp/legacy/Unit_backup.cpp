#include "Unit.h"
#include "AndroidOut.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <vector>

Unit::Unit(const std::string& name, int id, float x, float y, float speed,
           int maxHP, int minAttack, int maxAttack, int defense, float attackSpeed, float attackRange)
    : name_(name)
    , id_(id)
    , x_(x)
    , y_(y)
    , speed_(speed)
    , targetX_(x)
    , targetY_(y)
    , hasTarget_(false)
    , state_(UnitState::IDLE)
    , combatTarget_(nullptr)
    , isBlocked_(false)
    , isColliding_(false)
    , isAttacking_(false)
    , inCombat_(false)
    , maxHP_(maxHP)
    , currentHP_(maxHP)
    , minAttack_(minAttack)
    , maxAttack_(maxAttack)
    , defense_(defense)
    , attackSpeed_(attackSpeed)
    , attackCooldown_(0.0f)
    , attackRange_(attackRange) {
    
    // 新しいユニットが作成されたことをログに記録
    aout << "Unit created: " << name_ << " (ID: " << id_ << ") at position (" 
         << x_ << ", " << y_ << ") with speed " << speed_ << std::endl;
    aout << "Combat stats - HP: " << currentHP_ << "/" << maxHP_ 
         << ", Attack: " << minAttack_ << "-" << maxAttack_ 
         << ", Defense: " << defense_ 
         << ", Attack Speed: " << attackSpeed_ << "/sec"
         << ", Attack Range: " << attackRange_ << std::endl;
}

Unit::~Unit() {
    // ユニットが破棄されたことをログに記録
    aout << "Unit destroyed: " << name_ << " (ID: " << id_ << ")" << std::endl;
}

void Unit::update(float deltaTime) {
    // フラグをリセット
    isColliding_ = false;
    isAttacking_ = false;
    
    // 攻撃クールダウンを更新
    updateAttackCooldown(deltaTime);
    
    // ユニットが死亡している場合は処理をスキップ
    if (!isAlive()) {
        setState(UnitState::IDLE);
        return;
    }
    
    // 状態に応じた処理を実行
    switch (state_) {
        case UnitState::IDLE:
            updateIdleState(deltaTime);
            break;
            
        case UnitState::MOVING:
            updateMovingState(deltaTime);
            break;
            
        case UnitState::COMBAT:
            updateCombatState(deltaTime);
            break;
    }
}

void Unit::update(float deltaTime, const std::vector<std::shared_ptr<Unit>>& allUnits) {
    // フラグをリセット
    isColliding_ = false;
    isAttacking_ = false;
    
    // 攻撃クールダウンを更新
    updateAttackCooldown(deltaTime);
    
    // ユニットが死亡している場合は処理をスキップ
    if (!isAlive()) {
        setState(UnitState::IDLE);
        return;
    }
    
    // 状態に応じた処理を実行
    switch (state_) {
        case UnitState::IDLE:
            updateIdleState(deltaTime);
            break;
            
        case UnitState::MOVING:
            updateMovingState(deltaTime, allUnits);  // 衝突予測版を使用
            break;
            
        case UnitState::COMBAT:
            updateCombatState(deltaTime);
            break;
    }
}
