#ifndef SIMULATION_GAME_POSITION_H
#define SIMULATION_GAME_POSITION_H

/**
 * @brief 位置情報を表す値オブジェクト
 * 
 * 設計方針：
 * - 不変オブジェクト（Immutable）
 * - 値としての等価性を持つ
 * - ドメイン知識を表現（座標計算、距離計算）
 */
class Position {
public:
    /**
     * @brief コンストラクタ
     * @param x X座標
     * @param y Y座標
     */
    Position(float x, float y) : x_(x), y_(y) {}
    
    /**
     * @brief デフォルトコンストラクタ（原点を作成）
     */
    Position() : x_(0.0f), y_(0.0f) {}
    
    // コピー可能、ムーブ可能
    Position(const Position&) = default;
    Position& operator=(const Position&) = default;
    Position(Position&&) = default;
    Position& operator=(Position&&) = default;
    
    // ゲッター（値オブジェクトなのでconstのみ）
    float getX() const { return x_; }
    float getY() const { return y_; }
    
    /**
     * @brief 他の位置との距離を計算
     * @param other 比較対象の位置
     * @return 距離
     */
    float distanceTo(const Position& other) const {
        float dx = x_ - other.x_;
        float dy = y_ - other.y_;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    /**
     * @brief 指定したベクトル分移動した新しい位置を返す
     * @param deltaX X方向の移動量
     * @param deltaY Y方向の移動量
     * @return 新しい位置
     */
    Position moveBy(float deltaX, float deltaY) const {
        return Position(x_ + deltaX, y_ + deltaY);
    }
    
    /**
     * @brief 他の位置との中点を計算
     * @param other もう一つの位置
     * @return 中点の位置
     */
    Position midpointWith(const Position& other) const {
        return Position((x_ + other.x_) / 2.0f, (y_ + other.y_) / 2.0f);
    }
    
    // 等価性演算子
    bool operator==(const Position& other) const {
        return std::abs(x_ - other.x_) < EPSILON && 
               std::abs(y_ - other.y_) < EPSILON;
    }
    
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
    
    // 算術演算子（新しいPositionを返す）
    Position operator+(const Position& other) const {
        return Position(x_ + other.x_, y_ + other.y_);
    }
    
    Position operator-(const Position& other) const {
        return Position(x_ - other.x_, y_ - other.y_);
    }

private:
    float x_;
    float y_;
    
    static constexpr float EPSILON = 1e-6f; // 浮動小数点比較用
};

#endif // SIMULATION_GAME_POSITION_H
