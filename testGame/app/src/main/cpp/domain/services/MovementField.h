#ifndef SIMULATION_GAME_MOVEMENT_FIELD_H
#define SIMULATION_GAME_MOVEMENT_FIELD_H

#include "../value_objects/Position.h"
#include <vector>

class MovementField {
public:
    struct CircleObstacle {
        Position center;
        float radius;
    };

    MovementField(float minX, float minY, float maxX, float maxY)
        : minX_(minX), minY_(minY), maxX_(maxX), maxY_(maxY) {}

    void addCircleObstacle(const Position& center, float radius) {
        obstacles_.push_back({center, radius});
    }

    bool isInsideBounds(const Position& p) const {
        return p.getX() >= minX_ && p.getX() <= maxX_ && p.getY() >= minY_ && p.getY() <= maxY_;
    }

    // clearance: required distance from obstacle centers (e.g. movingRadius + otherRadius)
    bool isWalkable(const Position& p, float clearance = 0.0f) const {
        if (!isInsideBounds(p)) return false;
        for (const auto& o : obstacles_) {
            float dx = p.getX() - o.center.getX();
            float dy = p.getY() - o.center.getY();
            float dist2 = dx*dx + dy*dy;
            float minDist = (o.radius + clearance);
            if (dist2 < (minDist * minDist)) return false;
        }
        return true;
    }

    // Snap position into bounds (clamp)
    Position snapInside(const Position& p) const {
        float x = std::max(minX_, std::min(maxX_, p.getX()));
        float y = std::max(minY_, std::min(maxY_, p.getY()));
        return Position(x, y);
    }

private:
    float minX_, minY_, maxX_, maxY_;
    std::vector<CircleObstacle> obstacles_;
};

#endif // SIMULATION_GAME_MOVEMENT_FIELD_H
