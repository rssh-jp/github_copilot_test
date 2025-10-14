#include "TouchInputHandler.h"
#include "AndroidOut.h"

TouchInputHandler::TouchInputHandler()
    : touchEventCallback_(nullptr), isLongTapCandidate_(false),
      initialTouchX_(0.0f), initialTouchY_(0.0f), isPinching_(false),
      lastPinchDistance_(0.0f), initialPinchDistance_(0.0f),
      longTapTriggered_(false), pinchStartTriggered_(false) {}

void TouchInputHandler::setTouchEventCallback(TouchEventCallback callback) {
  touchEventCallback_ = callback;
}

void TouchInputHandler::handleMotionEvent(
    const GameActivityMotionEvent &motionEvent) {
  auto action = motionEvent.action;
  auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                      AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

  auto &pointer = motionEvent.pointers[pointerIndex];
  float x = GameActivityPointerAxes_getX(&pointer);
  float y = GameActivityPointerAxes_getY(&pointer);
  int pointerId = pointer.id;

  // デバッグログ：タッチイベントが到達していることを確認
  aout << "TouchInputHandler received event - action: "
       << (action & AMOTION_EVENT_ACTION_MASK)
       << ", pointerCount: " << motionEvent.pointerCount
       << ", activeCount: " << activeTouches_.size() << std::endl;

  switch (action & AMOTION_EVENT_ACTION_MASK) {
  case AMOTION_EVENT_ACTION_DOWN:
  case AMOTION_EVENT_ACTION_POINTER_DOWN: {
    // 新しいタッチポイントを追加
    activeTouches_.emplace_back(pointerId, x, y);

    if (activeTouches_.size() == 1) {
      // 最初のタッチ - ショートタップ/ロングタップの候補
      lastTouchStart_ = std::chrono::steady_clock::now();
      isLongTapCandidate_ = true;
      longTapTriggered_ = false;
      initialTouchX_ = x;
      initialTouchY_ = y;
      isPinching_ = false;
      pinchStartTriggered_ = false;

      aout << "Touch started at (" << x << ", " << y << ")" << std::endl;
    } else if (activeTouches_.size() == 2) {
      // 2本目の指 - ピンチ開始の可能性
      aout << "DEBUG: Pinch mode detected! Starting pinch processing..."
           << std::endl;
      isLongTapCandidate_ = false; // ロングタップはキャンセル
      longTapTriggered_ = false;   // ロングタップフラグもリセット
      isPinching_ = true;
      pinchStartTriggered_ = false; // ピンチ開始フラグをリセット

      // 初期ピンチ距離を計算
      initialPinchDistance_ =
          calculateDistance(activeTouches_[0], activeTouches_[1]);
      lastPinchDistance_ = initialPinchDistance_;

      // ピンチ開始イベントを即座に送信（ユニット移動を無効化するため）
      if (touchEventCallback_ && !pinchStartTriggered_) {
        float centerX, centerY;
        calculatePinchCenter(activeTouches_[0], activeTouches_[1], centerX,
                             centerY);
        aout << "DEBUG: Pinch start event - center: (" << centerX << ", "
             << centerY << "), scale: 1.0" << std::endl;
        TouchEvent event(TouchInputType::PINCH_ZOOM, centerX, centerY,
                         1.0f); // スケール1.0で開始
        touchEventCallback_(event);
        pinchStartTriggered_ = true;
      }

      aout << "Pinch started, initial distance: " << initialPinchDistance_
           << std::endl;
    } else {
      // 3本目以降は無視（現在の実装では）
      isLongTapCandidate_ = false;
      longTapTriggered_ = false;
      isPinching_ = false;
      pinchStartTriggered_ = false;
    }
    break;
  }

  case AMOTION_EVENT_ACTION_MOVE: {
    if (isPinching_ && activeTouches_.size() >= 2) {
      // ピンチ中の処理
      // 最新の2つのタッチポイントを更新
      for (auto index = 0; index < motionEvent.pointerCount && index < 2;
           index++) {
        auto &movePointer = motionEvent.pointers[index];
        TouchPoint *touchPoint = findTouchPoint(movePointer.id);
        if (touchPoint) {
          touchPoint->x = GameActivityPointerAxes_getX(&movePointer);
          touchPoint->y = GameActivityPointerAxes_getY(&movePointer);
        }
      }

      // 現在の距離を計算
      float currentDistance =
          calculateDistance(activeTouches_[0], activeTouches_[1]);
      aout << "DEBUG: Pinch move - currentDist: " << currentDistance
           << ", lastDist: " << lastPinchDistance_
           << ", threshold: " << PINCH_THRESHOLD << std::endl;
      if (std::abs(currentDistance - lastPinchDistance_) > PINCH_THRESHOLD) {
        // ピンチズームイベントを発生（前回距離からの相対的な変化）
        float scale = currentDistance / lastPinchDistance_; // 前回距離との比率
        aout << "DEBUG: Sending PINCH_ZOOM event - scale: " << scale
             << " (currentDist: " << currentDistance
             << ", lastDist: " << lastPinchDistance_ << ")" << std::endl;
        float centerX, centerY;
        calculatePinchCenter(activeTouches_[0], activeTouches_[1], centerX,
                             centerY);

        if (touchEventCallback_) {
          TouchEvent event(TouchInputType::PINCH_ZOOM, centerX, centerY, scale);
          aout << "CALLBACK_DEBUG: Calling touchEventCallback_ for PINCH_ZOOM"
               << std::endl;
          touchEventCallback_(event);
          aout << "CALLBACK_DEBUG: touchEventCallback_ call completed"
               << std::endl;
        } else {
          aout << "CALLBACK_DEBUG: touchEventCallback_ is null!" << std::endl;
        }

        lastPinchDistance_ = currentDistance;
        aout << "Pinch zoom, scale: " << scale << " center: (" << centerX
             << ", " << centerY << ")" << std::endl;
      }
    } else if (isLongTapCandidate_ && activeTouches_.size() == 1) {
      // ロングタップ候補中の移動チェック
      float dx = x - initialTouchX_;
      float dy = y - initialTouchY_;
      float distance = std::sqrt(dx * dx + dy * dy);

      if (distance > TOUCH_MOVE_THRESHOLD) {
        // 移動が大きすぎるのでロングタップではない
        isLongTapCandidate_ = false;
        aout << "Long tap cancelled due to movement: " << distance << std::endl;
      }
    }
    break;
  }

  case AMOTION_EVENT_ACTION_UP:
  case AMOTION_EVENT_ACTION_POINTER_UP:
  case AMOTION_EVENT_ACTION_CANCEL: {
    TouchPoint *releasedTouch = findTouchPoint(pointerId);
    if (!releasedTouch)
      break;

    if (activeTouches_.size() == 1 && !isPinching_) {
      // 最後の指が離された
      if (touchEventCallback_) {
        if (longTapTriggered_) {
          // ロングタップは既にトリガー済み - 終了イベントを送信
          TouchEvent event(TouchInputType::LONG_TAP_END, initialTouchX_,
                           initialTouchY_);
          touchEventCallback_(event);
          aout << "Long tap ended, finger released" << std::endl;
        } else if (isLongTapCandidate_) {
          // ロングタップ時間に達する前に離された - ショートタップ
          aout << "DEBUG: Generating SHORT_TAP event at (" << initialTouchX_
               << ", " << initialTouchY_ << ")" << std::endl;
          if (touchEventCallback_) {
            TouchEvent event(TouchInputType::SHORT_TAP, initialTouchX_,
                             initialTouchY_);
            aout << "DEBUG: Calling touchEventCallback_ for SHORT_TAP"
                 << std::endl;
            touchEventCallback_(event);
            aout << "DEBUG: touchEventCallback_ call completed for SHORT_TAP"
                 << std::endl;
          } else {
            aout << "ERROR: touchEventCallback_ is null for SHORT_TAP!"
                 << std::endl;
          }
          aout << "Short tap detected at (" << initialTouchX_ << ", "
               << initialTouchY_ << ")" << std::endl;
        }
      }

      // 状態をリセット
      isLongTapCandidate_ = false;
      longTapTriggered_ = false;
      isPinching_ = false;
    } else if (isPinching_ && activeTouches_.size() == 2) {
      // ピンチ中に1本の指が離された - ピンチ終了
      isPinching_ = false;
      pinchStartTriggered_ = false;

      // ピンチ終了イベントを送信（ユニット移動を再有効化するため）
      if (touchEventCallback_) {
        TouchEvent event(TouchInputType::PINCH_END, 0, 0, 1.0f);
        touchEventCallback_(event);
      }

      aout << "Pinch ended" << std::endl;
    }

    removeTouchPoint(pointerId);
    break;
  }
  }
}

void TouchInputHandler::update() {
  // ロングタップの判定（時間ベース）
  if (isLongTapCandidate_ && !isPinching_) {
    checkLongTap();
  }
}

float TouchInputHandler::calculateDistance(const TouchPoint &p1,
                                           const TouchPoint &p2) const {
  float dx = p1.x - p2.x;
  float dy = p1.y - p2.y;
  return std::sqrt(dx * dx + dy * dy);
}

void TouchInputHandler::calculatePinchCenter(const TouchPoint &p1,
                                             const TouchPoint &p2,
                                             float &centerX,
                                             float &centerY) const {
  centerX = (p1.x + p2.x) / 2.0f;
  centerY = (p1.y + p2.y) / 2.0f;
}

void TouchInputHandler::checkLongTap() {
  if (!isLongTapCandidate_ || longTapTriggered_) {
    return; // ロングタップ候補でないか、既にトリガー済み
  }

  auto now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now - lastTouchStart_)
                      .count();

  if (duration >= LONG_TAP_THRESHOLD_MS && touchEventCallback_) {
    // ロングタップが確定（まだ指は離されていない）
    TouchEvent event(TouchInputType::LONG_TAP, initialTouchX_, initialTouchY_);
    touchEventCallback_(event);
    longTapTriggered_ = true; // ロングタップをトリガー済みにマーク
    isLongTapCandidate_ = false; // 重複発生を防ぐ
    aout << "Long tap triggered while holding at (" << initialTouchX_ << ", "
         << initialTouchY_ << ")" << std::endl;
  }
}

TouchPoint *TouchInputHandler::findTouchPoint(int pointerId) {
  for (auto &touch : activeTouches_) {
    if (touch.id == pointerId) {
      return &touch;
    }
  }
  return nullptr;
}

void TouchInputHandler::removeTouchPoint(int pointerId) {
  activeTouches_.erase(std::remove_if(activeTouches_.begin(),
                                      activeTouches_.end(),
                                      [pointerId](const TouchPoint &touch) {
                                        return touch.id == pointerId;
                                      }),
                       activeTouches_.end());
}