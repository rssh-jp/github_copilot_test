#include "Unit.h"
#include "AndroidOut.h"
#include <cmath>

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
    // 各フレームで状態フラグをリセット（必要に応じて再設定される）
    isColliding_ = false;
    isAttacking_ = false;
    
    // 攻撃クールダウンを更新
    updateAttackCooldown(deltaTime);
    
    // ユニットが死亡している場合は処理をスキップ
    if (!isAlive()) {
        return;
    }
    
    // ユニット1（RedUnit）以外は戦闘中の場合は位置を固定し、移動処理をスキップ
    if (inCombat_ && name_ != "RedUnit") {
        // 戦闘中は位置を変更しない
        isBlocked_ = false; // 衝突フラグをリセット
        return;
    }
    
    // 衝突でブロックされている場合も移動しない
    if (isBlocked_) {
        // 衝突状態をリセット（次のフレームで再計算される）
        isBlocked_ = false;
        return;
    }
    
    // 目標位置が設定されている場合、その方向に移動
    if (hasTarget_) {
        // 現在位置と目標位置の差分を計算
        float dirX = targetX_ - x_;
        float dirY = targetY_ - y_;
        
        // 距離を計算
        float distance = std::sqrt(dirX * dirX + dirY * dirY);
        
        // 目標位置に十分近づいた場合、移動を停止
        if (distance < 0.01f) {
            x_ = targetX_;
            y_ = targetY_;
            hasTarget_ = false;
            aout << name_ << " reached target position (" << x_ << ", " << y_ << ")" << std::endl;
            return;
        }
        
        // 移動処理
        move(dirX, dirY, deltaTime);
    }
}

bool Unit::isColliding(const std::shared_ptr<Unit>& other) const {
    if (other->getId() == id_) {
        return false; // 自分自身とは衝突しない
    }
    
    if (!isAlive() || !other->isAlive()) {
        return false; // 死亡ユニットは衝突判定を無視
    }
    
    float dx = other->getX() - x_;
    float dy = other->getY() - y_;
    float distanceSquared = dx * dx + dy * dy;
    
    // 2つのユニットの衝突半径の合計（めり込み防止のため少し大きめに）
    float collisionRadiusSum = COLLISION_RADIUS + COLLISION_RADIUS;
    
    // 距離が衝突半径の合計より小さければ衝突している
    return distanceSquared < (collisionRadiusSum * collisionRadiusSum);
}

void Unit::avoidCollisions(const std::vector<std::shared_ptr<Unit>>& units, float avoidanceStrength) {
    // 戦闘中なら衝突回避処理をスキップ
    if (inCombat_) {
        return;
    }
    
    // 衝突回避がほぼ0の場合は計算を省略
    if (avoidanceStrength < 0.001f) {
        return;
    }
    
    float avoidX = 0.0f;
    float avoidY = 0.0f;
    int collisionCount = 0;
    bool actualCollision = false;
    
    // 他のすべてのユニットとの衝突をチェック
    for (const auto& other : units) {
        // 自分自身はスキップ
        if (other->getId() == id_) {
            continue;
        }
        
        float dx = x_ - other->getX();
        float dy = y_ - other->getY();
        float distanceSquared = dx * dx + dy * dy;
        
        // 実際の衝突判定（ユニットの形状に合わせて円形で調整）
        float collisionRadiusSum = COLLISION_RADIUS * 2;
        float minDistanceSquared = collisionRadiusSum * collisionRadiusSum;
        
        if (distanceSquared < minDistanceSquared) {
            // 実際に衝突している場合
            actualCollision = true;
            isColliding_ = true;  // 衝突中フラグを設定（色変更のため）
            
            // 相手も生きているユニットなら戦闘状態に移行
            if (other->isAlive()) {
                // 移動を停止し、戦闘開始
                isBlocked_ = true;
                inCombat_ = true;  // 戦闘中フラグをオンに
                
                // 目標位置を調整 - 現在位置を目標に設定
                targetX_ = x_;
                targetY_ = y_;
                hasTarget_ = false;  // 移動目標をクリア
                
                // めり込みを防止するために位置を補正する
                if (distanceSquared > 0.001f) { // ゼロ除算防止
                    float distance = std::sqrt(distanceSquared);
                    // めり込んだ分だけ押し戻す
                    float pushBackDistance = collisionRadiusSum - distance;
                    if (pushBackDistance > 0) {
                        float pushRatio = pushBackDistance / distance;
                        
                        // 押し戻すベクトルを計算（相手から自分への方向に少し移動）
                        float pushX = dx * pushRatio * 0.5f; // 半分だけ自分が移動（相手も半分移動するため）
                        float pushY = dy * pushRatio * 0.5f;
                        
                        // 位置を調整
                        x_ += pushX;
                        y_ += pushY;
                        
                        // デバッグログ
                        aout << name_ << " adjusted position due to collision with " << other->getName() 
                             << " by (" << pushX << ", " << pushY << ")" << std::endl;
                    }
                }
                
                // 相手ユニットも戦闘状態にする
                if (auto otherPtr = other) {
                    otherPtr->setInCombat(true);
                }
                
                // 生きているユニット同士なら攻撃を行う
                if (isAlive() && other->isAlive() && canAttack()) {
                    attack(other); // 衝突相手を攻撃
                    aout << name_ << " is fighting with " << other->getName() 
                         << " at position (" << x_ << "," << y_ << ")" << std::endl;
                } else {
                    aout << name_ << " is blocked due to collision with " << other->getName() 
                         << " at position (" << x_ << "," << y_ << ")" << std::endl;
                }
            }
        }
        
        // 十分離れていれば無視（回避処理は衝突半径の4倍まで）
        if (distanceSquared > 4.0f * COLLISION_RADIUS * COLLISION_RADIUS) {
            continue;
        }
        
        // 距離に反比例する回避力
        float distance = std::sqrt(distanceSquared);
        if (distance > 0.001f) { // ゼロ除算を防ぐ
            float avoidFactor = COLLISION_RADIUS / distance;
            avoidX += dx * avoidFactor;
            avoidY += dy * avoidFactor;
            collisionCount++;
        } else {
            // ほぼ同じ位置にいる場合はランダムな方向に少し動かす
            avoidX += static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
            avoidY += static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
            collisionCount++;
            
            // 重なっている場合は確実に停止
            isBlocked_ = true;
        }
    }
    
    // 衝突していなくて、かつ他のユニットが近くにある場合のみ回避処理
    if (!actualCollision && collisionCount > 0) {
        // 平均化
        avoidX /= static_cast<float>(collisionCount);
        avoidY /= static_cast<float>(collisionCount);
        
        // 長さを正規化
        float len = std::sqrt(avoidX * avoidX + avoidY * avoidY);
        if (len > 0.001f) {
            avoidX /= len;
            avoidY /= len;
        }
        
        // 目標位置を回避方向に調整（制限なし）
        targetX_ += avoidX * avoidanceStrength;
        targetY_ += avoidY * avoidanceStrength;
    }
    
    // 実際に衝突している場合、ログ出力
    if (isBlocked_ && hasTarget_) {
        aout << name_ << " is blocked due to collision with other units" << std::endl;
    }
}

void Unit::move(float dirX, float dirY, float deltaTime) {
    // 方向ベクトルの正規化（単位ベクトル化）
    float length = std::sqrt(dirX * dirX + dirY * dirY);
    if (length > 0.001f) { // 0で割らないための小さな値チェック
        dirX /= length;
        dirY /= length;
    } else {
        // 方向が指定されていない場合は移動しない
        return;
    }
    
    // 速度と経過時間に基づいて位置を更新
    float dx = dirX * speed_ * deltaTime;
    float dy = dirY * speed_ * deltaTime;
    
    // 位置を更新
    x_ += dx;
    y_ += dy;
    
    // 詳細な移動ログはデバッグ時のみ出力するといい
    // aout << "Unit " << name_ << " moved to (" << x_ << ", " << y_ << ")" << std::endl;
}

void Unit::setPosition(float x, float y) {
    // 制限なしで直接座標を設定
    x_ = x;
    y_ = y;
    
    // 位置を直接設定した場合、目標位置もリセット
    targetX_ = x_;
    targetY_ = y_;
    hasTarget_ = false;
}

void Unit::setTargetPosition(float x, float y) {
    // ユニット1（RedUnit）は戦闘中でも移動可能、他のユニットは戦闘中は移動不可
    if (inCombat_ && name_ != "RedUnit") {
        aout << name_ << " is in combat and cannot move to new target position" << std::endl;
        return;
    }
    
    // ユニット1の場合は戦闘中でも移動可能であることをログに記録
    if (inCombat_ && name_ == "RedUnit") {
        aout << name_ << " is in combat but can still move to new target position (player controlled)" << std::endl;
    }
    
    targetX_ = x;
    targetY_ = y;
    hasTarget_ = true;
    
    // 目標位置の設定ログを出力
    aout << name_ << " targeting position (" << x << ", " << y << ")" << std::endl;
}

int Unit::attack(std::shared_ptr<Unit> target) {
    if (!target || !isAlive() || !target->isAlive() || !canAttack()) {
        return 0;
    }
    
    // 攻撃をしていることを示すフラグをセット
    isAttacking_ = true;
    
    // 攻撃力のランダム計算（最小～最大の間）
    int diceRoll = 0;
    if (maxAttack_ > minAttack_) {
        diceRoll = minAttack_ + rand() % (maxAttack_ - minAttack_ + 1);
    } else {
        diceRoll = minAttack_;
    }
    
    // 攻撃を実行
    int damage = target->takeDamage(diceRoll);
    
    // 攻撃ログを出力
    aout << name_ << " attacks " << target->getName() << " for " << damage 
         << " damage! (rolled " << diceRoll << ")" << std::endl;
    
    // 攻撃後、攻撃クールダウンを設定
    attackCooldown_ = 1.0f / attackSpeed_;
    
    return damage;
}

int Unit::takeDamage(int damage) {
    // 生きていなければダメージを受けない
    if (!isAlive()) {
        return 0;
    }
    
    // 防御力を適用（最低1ダメージ）
    int actualDamage = std::max(1, damage - defense_);
    
    // ヒットポイントを減らす
    currentHP_ -= actualDamage;
    
    // HPが0以下になったら0に制限
    if (currentHP_ < 0) {
        currentHP_ = 0;
        aout << name_ << " has been defeated!" << std::endl;
    } else {
        // ダメージログを出力
        aout << name_ << " took " << actualDamage << " damage! HP: " 
             << currentHP_ << "/" << maxHP_ << std::endl;
    }
    
    return actualDamage;
}

bool Unit::updateAttackCooldown(float deltaTime) {
    // クールダウンが0以下なら更新不要
    if (attackCooldown_ <= 0.0f) {
        return true;
    }
    
    // クールダウンを減少
    attackCooldown_ -= deltaTime;
    
    // クールダウンが完了したかどうかを返す
    return attackCooldown_ <= 0.0f;
}

std::shared_ptr<Unit> Unit::findTargetToAttack(const std::vector<std::shared_ptr<Unit>>& units) {
    // 自身が生存していない場合、攻撃できない
    if (!isAlive() || !canAttack()) {
        return nullptr;
    }
    
    // 戦闘中に攻撃対象が見つかった場合のフラグ
    bool foundCombatTarget = false;
    
    // 攻撃範囲の設定
    // 戦闘中または衝突中は広い攻撃範囲を使用（確実に攻撃できるように）
    float attackRangeSquared = inCombat_ || isColliding_ ? 
                               COLLISION_RADIUS * COLLISION_RADIUS * 6.0f : // 戦闘中は非常に広範囲
                               COLLISION_RADIUS * COLLISION_RADIUS * 1.5f;  // 通常範囲
    
    // 攻撃可能な最も近いユニットを探す
    std::shared_ptr<Unit> closestEnemy = nullptr;
    float closestDistanceSquared = attackRangeSquared;
    
    for (const auto& other : units) {
        // 自分自身または死亡ユニットはスキップ
        if (other->getId() == id_ || !other->isAlive()) {
            continue;
        }
        
        // 距離計算
        float dx = x_ - other->getX();
        float dy = y_ - other->getY();
        float distanceSquared = dx * dx + dy * dy;
        
        // 戦闘中のユニットは優先度が高い
        if (inCombat_ && other->inCombat()) {
            // 戦闘中の相手なら距離に関わらず攻撃対象に設定
            closestEnemy = other;
            foundCombatTarget = true;
            break; // 戦闘中の相手が見つかったらすぐに決定
        }
        // 戦闘中の相手が見つかっていない場合のみ、通常の距離チェック
        else if (!foundCombatTarget && distanceSquared <= closestDistanceSquared) {
            closestDistanceSquared = distanceSquared;
            closestEnemy = other;
        }
    }
    
    // 戦闘ターゲットを見つけた場合、それをログ出力
    if (closestEnemy && inCombat_) {
        aout << name_ << " targeting " << closestEnemy->getName() << " for attack in combat" << std::endl;
    }
    
    return closestEnemy;
}
