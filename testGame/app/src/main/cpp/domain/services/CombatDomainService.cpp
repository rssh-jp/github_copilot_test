#include "CombatDomainService.h"

/*
 * CombatDomainService.cpp
 *
 * Purpose:
 * - Provide stateless utility functions for combat-related domain checks and calculations.
 * - Responsibilities include: damage calculation, range checks and simple combat helpers.
 *
 * Notes:
 * - This module is intended to be deterministic and side-effect free (except optional debug logging).
 * - Higher-level orchestration (when to call executeCombat) belongs to use-cases / systems.
 */
#include <algorithm>
#include <cmath>
#include "../android/AndroidOut.h"

// 静的メンバの初期化
std::random_device CombatDomainService::rd_;
std::mt19937 CombatDomainService::gen_(CombatDomainService::rd_());
std::uniform_real_distribution<float> CombatDomainService::dis_(0.8f, 1.2f);

CombatDomainService::CombatResult CombatDomainService::executeCombat(
    UnitEntity& attacker, UnitEntity& target) {
    /**
     * Execute a single combat exchange between attacker and target.
     *
     * Preconditions / Guards:
     *  - Attacker must be in a state that allows performing an attack (IDLE or COMBAT).
     *  - Attacker must be within effective attack range (attacker.attackRange + target.collisionRadius).
     *
     * Behavior:
     *  - Compute damage and apply it to the target.
     *  - If the target survives and can counter-attack (in range), perform a counter-attack.
     *  - Return a CombatResult summarizing damage and deaths.
     */
    // Guard conditions: attacker readiness
    if (attacker.getState() != UnitState::IDLE && attacker.getState() != UnitState::COMBAT) {
        return CombatResult();
    }

    // Guard conditions: effective range
    if (!isInAttackRange(attacker, target)) {
        return CombatResult();
    }

    // Damage calculation and application
    int damage = calculateDamage(attacker.getStats(), target.getStats());
    target.takeDamage(damage);

    // Check for death and potential counter-attack
    bool targetKilled = target.getStats().getCurrentHp() <= 0;
    bool attackerKilled = false;
    if (!targetKilled && isInAttackRange(target, attacker)) {
        int counterDamage = calculateDamage(target.getStats(), attacker.getStats());
        attacker.takeDamage(counterDamage);
        attackerKilled = attacker.getStats().getCurrentHp() <= 0;
    }

    return CombatResult(damage, targetKilled, attackerKilled);
}

int CombatDomainService::getRandomAttackPower(const UnitStats& stats) {
    // Return a uniformly random attack power between min and max (inclusive).
    if (stats.getMinAttackPower() == stats.getMaxAttackPower()) return stats.getMinAttackPower();
    return stats.getMinAttackPower() + (rand() % (stats.getMaxAttackPower() - stats.getMinAttackPower() + 1));
}

int CombatDomainService::calculateDamage(
    const UnitStats& attackerStats, const UnitStats& targetStats) {
    // Compute damage using attack power with a small random multiplier. This is intentionally
    // simple; defense and other modifiers can be added here later.
    float baseDamage = getRandomAttackPower(attackerStats) * dis_(gen_);
    return std::max(1, static_cast<int>(baseDamage));
}

bool CombatDomainService::isInAttackRange(
    const UnitEntity& attacker, const UnitEntity& target) {
    
    float dx = attacker.getPosition().getX() - target.getPosition().getX();
    float dy = attacker.getPosition().getY() - target.getPosition().getY();
    float distance = std::sqrt(dx * dx + dy * dy);
    
    // Effective range = attacker's attackRange + target's collision radius.
    float effectiveRange = attacker.getStats().getAttackRange() + target.getStats().getCollisionRadius();
    bool inRange = distance <= effectiveRange;
    // Debug only when not in range to aid QA investigations without spamming logs.
    if (!inRange) {
        aout << "isInAttackRange: attacker=" << attacker.getId() << " target=" << target.getId()
             << " distance=" << distance << " effectiveRange=" << effectiveRange << std::endl;
    }
    return inRange;
}

bool CombatDomainService::isColliding(
    const UnitEntity& unit1, const UnitEntity& unit2) {
    
    float dx = unit1.getPosition().getX() - unit2.getPosition().getX();
    float dy = unit1.getPosition().getY() - unit2.getPosition().getY();
    float distance = std::sqrt(dx * dx + dy * dy);
    
    // Two units collide when the distance between centers is less than the sum of their
    // collision radii (here using a symmetric getCollisionRadius helper). This is a
    // conservative test suitable for simple circle-based collisions.
    float collisionDistance = getCollisionRadius() * 2.0f;
    return distance < collisionDistance;
}
