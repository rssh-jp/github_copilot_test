#ifndef SIMULATION_GAME_TOUCH_INPUT_HANDLER_H
#define SIMULATION_GAME_TOUCH_INPUT_HANDLER_H

#include <chrono>
#include <cmath>
#include <functional>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <vector>

/**
 * @brief タッチ入力の種類
 */
enum class TouchInputType {
  SHORT_TAP,    // ショートタップ（ユニット移動用）
  LONG_TAP,     // ロングタップ（カメラパン用）
  LONG_TAP_END, // ロングタップ終了（ユニット移動再有効化用）
  PINCH_ZOOM,   // ピンチズーム（カメラズーム用）
  PINCH_END,    // ピンチ終了（ユニット移動再有効化用）
  MOVE_GESTURE // ドラッグ移動（現在未使用）
};

/**
 * @brief タッチポイントの情報
 */
struct TouchPoint {
  int id;
  float x, y;
  std::chrono::steady_clock::time_point timestamp;

  TouchPoint(int pointerId, float posX, float posY)
      : id(pointerId), x(posX), y(posY),
        timestamp(std::chrono::steady_clock::now()) {}
};

/**
 * @brief タッチイベントの詳細情報
 */
struct TouchEvent {
  TouchInputType type;
  float x, y; // 主要な座標（ショートタップ、ロングタップ時）
  float centerX, centerY; // ピンチの中心座標
  float scale;            // ピンチのスケール変化量

  TouchEvent(TouchInputType t, float posX, float posY)
      : type(t), x(posX), y(posY), centerX(0), centerY(0), scale(1.0f) {}

  TouchEvent(TouchInputType t, float cX, float cY, float s)
      : type(t), x(0), y(0), centerX(cX), centerY(cY), scale(s) {}
};

/**
 * @brief タッチ入力を処理し、ジェスチャーを識別するクラス
 *
 * 責任:
 * - Android native input eventsを受け取り、ゲーム固有のジェスチャーに分類
 * - ショートタップ/ロングタップ/ピンチズームの判定
 * - 適切なコールバックへの振り分け
 */
class TouchInputHandler {
public:
  /**
   * @brief タッチイベントコールバック型
   */
  using TouchEventCallback = std::function<void(const TouchEvent &)>;

  /**
   * @brief コンストラクタ
   */
  TouchInputHandler();

  /**
   * @brief タッチイベントコールバックを設定
   * @param callback 処理するコールバック関数
   */
  void setTouchEventCallback(TouchEventCallback callback);

  /**
   * @brief Android motion eventを処理する
   * @param motionEvent Android native motion event
   */
  void handleMotionEvent(const GameActivityMotionEvent &motionEvent);

  /**
   * @brief タッチ状態をリセット（フレーム更新時などに呼び出し）
   */
  void update();

private:
  TouchEventCallback touchEventCallback_;

  // タッチ状態管理
  std::vector<TouchPoint> activeTouches_;
  std::chrono::steady_clock::time_point lastTouchStart_;
  bool isLongTapCandidate_;
  float initialTouchX_, initialTouchY_;

  // ピンチ状態管理
  bool isPinching_;
  float lastPinchDistance_;
  float initialPinchDistance_;

  // 設定可能な閾値
  static constexpr float LONG_TAP_THRESHOLD_MS =
      500.0f; // ロングタップ判定時間（ミリ秒）
  static constexpr float TOUCH_MOVE_THRESHOLD =
      20.0f; // タッチ移動判定の閾値（ピクセル）
  static constexpr float PINCH_THRESHOLD =
      2.0f; // ピンチ判定の閾値（ピクセル）- 敏感に調整

  // 状態管理フラグ
  bool longTapTriggered_; // ロングタップが既にトリガーされたか
  bool pinchStartTriggered_; // ピンチ開始が既にトリガーされたか

  /**
   * @brief 2点間の距離を計算
   */
  float calculateDistance(const TouchPoint &p1, const TouchPoint &p2) const;

  /**
   * @brief ピンチの中心点を計算
   */
  void calculatePinchCenter(const TouchPoint &p1, const TouchPoint &p2,
                            float &centerX, float &centerY) const;

  /**
   * @brief ロングタップの判定を行う
   */
  void checkLongTap();

  /**
   * @brief 指定IDのタッチポイントを検索
   */
  TouchPoint *findTouchPoint(int pointerId);

  /**
   * @brief タッチポイントを削除
   */
  void removeTouchPoint(int pointerId);
};

#endif // SIMULATION_GAME_TOUCH_INPUT_HANDLER_H