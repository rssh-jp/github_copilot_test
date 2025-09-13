#include "CombatDomainService.h"
#include <algorithm>
#include <cmath>

// 静的メンバの初期化
std::random_device CombatDomainService::rd_;
std::mt19937 CombatDomainService::gen_(CombatDomainService::rd_());
std::uniform_real_distribution<float> CombatDomainService::dis_(0.8f, 1.2f);

CombatDomainService::CombatResult CombatDomainService::executeCombat(
    UnitEntity& attacker, UnitEntity& target) {
    
    // 攻撃範囲チェック
    if (!isInAttackRange(attacker, target)) {
        return CombatResult();
    }

    // ダメージ計算
    int damage = calculateDamage(attacker.getStats(), target.getStats());
    
    // ダメージ適用
    target.takeDamage(damage);
    
    // 相手を倒したかチェック
    bool targetKilled = target.getStats().getCurrentHp() <= 0;
    
    // 反撃処理（相手が生きている場合のみ）
    bool attackerKilled = false;
    if (!targetKilled && isInAttackRange(target, attacker)) {
        int counterDamage = calculateDamage(target.getStats(), attacker.getStats());
        attacker.takeDamage(counterDamage);
        attackerKilled = attacker.getStats().getCurrentHp() <= 0;
    }
    
    return CombatResult(damage, targetKilled, attackerKilled);
}

int CombatDomainService::getRandomAttackPower(const UnitStats& stats) {
    if (stats.getMinAttackPower() == stats.getMaxAttackPower()) return stats.getMinAttackPower();
    return stats.getMinAttackPower() + (rand() % (stats.getMaxAttackPower() - stats.getMinAttackPower() + 1));
}

int CombatDomainService::calculateDamage(
    const UnitStats& attackerStats, const UnitStats& targetStats) {
    // 基本ダメージ = 攻撃力（乱数）× ランダム補正
    float baseDamage = getRandomAttackPower(attackerStats) * dis_(gen_);
    // 将来的に防御力を実装する場合はここで計算
    // float finalDamage = baseDamage - targetStats.defensePower;
    // 最低1ダメージは与える
    return std::max(1, static_cast<int>(baseDamage));
}

bool CombatDomainService::isInAttackRange(
    const UnitEntity& attacker, const UnitEntity& target) {
    
    float dx = attacker.getPosition().getX() - target.getPosition().getX();
    float dy = attacker.getPosition().getY() - target.getPosition().getY();
    float distance = std::sqrt(dx * dx + dy * dy);
    
    return distance <= attacker.getStats().getAttackRange();
}

bool CombatDomainService::isColliding(
    const UnitEntity& unit1, const UnitEntity& unit2) {
    
    float dx = unit1.getPosition().getX() - unit2.getPosition().getX();
    float dy = unit1.getPosition().getY() - unit2.getPosition().getY();
    float distance = std::sqrt(dx * dx + dy * dy);
    
    // 2つのユニットの衝突半径の合計より近い場合は衝突
    float collisionDistance = getCollisionRadius() * 2.0f;
    return distance < collisionDistance;
}
