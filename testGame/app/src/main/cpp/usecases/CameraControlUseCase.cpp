#include "CameraControlUseCase.h"
#include "../frameworks/android/AndroidOut.h"
#include <algorithm>

CameraControlUseCase::CameraControlUseCase()
    : currentState_(), stateChangeCallback_(nullptr),
      minZoomLevel_(0.15f) // 15%まで縮小可能（従来の約3倍ズームアウト）
      ,
      maxZoomLevel_(3.0f) // 300%まで拡大可能
      ,
      minPanX_(-50.0f) // デフォルトのパン制限
      ,
      maxPanX_(50.0f), minPanY_(-50.0f), maxPanY_(50.0f),
      panLimitsEnabled_(true) {}

void CameraControlUseCase::setCameraStateChangeCallback(
    CameraStateChangeCallback callback) {
  stateChangeCallback_ = callback;
  // 初期状態の即座通知はしない（明示的な初期化で対応）
}

void CameraControlUseCase::setCameraInitialState(
    const CameraState &initialState) {
  currentState_ = initialState;
  aout << "Camera initial state set to: offset(" << currentState_.offsetX
       << ", " << currentState_.offsetY << ") zoom(" << currentState_.zoomLevel
       << ")" << std::endl;
}

void CameraControlUseCase::updateCurrentPosition(float offsetX, float offsetY) {
  currentState_.offsetX = offsetX;
  currentState_.offsetY = offsetY;
  aout << "DEBUG: Camera position updated to (" << offsetX << ", " << offsetY
       << ")" << std::endl;
}

void CameraControlUseCase::panCameraToPosition(float worldX, float worldY) {
  // 現在のカメラ位置からタッチした位置への相対移動
  // タッチした位置までの移動量を計算
  float deltaX =
      worldX - (-currentState_.offsetX); // 現在位置からタッチ位置への差分
  float deltaY =
      worldY - (-currentState_.offsetY); // 現在位置からタッチ位置への差分

  // 現在のオフセットに移動量を加算（世界座標系なので符号反転）
  float newOffsetX = currentState_.offsetX - deltaX;
  float newOffsetY = currentState_.offsetY - deltaY;

  aout << "DEBUG: panCameraToPosition - target world pos: (" << worldX << ", "
       << worldY << ") -> camera offset: (" << newOffsetX << ", " << newOffsetY
       << ")" << std::endl;

  clampPanPosition(newOffsetX, newOffsetY);

  if (currentState_.offsetX != newOffsetX ||
      currentState_.offsetY != newOffsetY) {
    float oldOffsetX = currentState_.offsetX;
    float oldOffsetY = currentState_.offsetY;

    currentState_.offsetX = newOffsetX;
    currentState_.offsetY = newOffsetY;

    aout << "Camera panned from (" << oldOffsetX << ", " << oldOffsetY
         << ") to (" << newOffsetX << ", " << newOffsetY << ")" << std::endl;
    notifyStateChange();
  }
}

void CameraControlUseCase::zoomCamera(float scale, float centerX,
                                      float centerY) {
  // スケール変化をズームレベルに適用
  // scale は相対的な変化量（1.0 = 変化なし）
  float deltaScale = scale - 1.0f;
  float newZoomLevel = currentState_.zoomLevel +
                       (deltaScale * 1.0f); // 感度を上げる（0.5f -> 1.0f）

  aout << "DEBUG: CameraControlUseCase::zoomCamera called - scale: " << scale
       << ", deltaScale: " << deltaScale << ", newZoomLevel: " << newZoomLevel
       << std::endl;
  aout << "DEBUG: Camera state BEFORE zoom - offsetX: " << currentState_.offsetX
       << ", offsetY: " << currentState_.offsetY
       << ", zoomLevel: " << currentState_.zoomLevel << std::endl;

  newZoomLevel = clampZoomLevel(newZoomLevel);

  if (std::abs(currentState_.zoomLevel - newZoomLevel) >
      0.01f) { // 微小変化を無視
    currentState_.zoomLevel = newZoomLevel;

    aout << "DEBUG: Camera state AFTER zoom - offsetX: "
         << currentState_.offsetX << ", offsetY: " << currentState_.offsetY
         << ", zoomLevel: " << currentState_.zoomLevel << std::endl;
    aout << "Camera zoom changed to " << newZoomLevel
         << " (scale factor: " << scale << ")" << std::endl;
    notifyStateChange();
  }
}

void CameraControlUseCase::panCameraBy(float deltaX, float deltaY) {
  float newOffsetX = currentState_.offsetX + deltaX;
  float newOffsetY = currentState_.offsetY + deltaY;

  clampPanPosition(newOffsetX, newOffsetY);

  if (currentState_.offsetX != newOffsetX ||
      currentState_.offsetY != newOffsetY) {
    currentState_.offsetX = newOffsetX;
    currentState_.offsetY = newOffsetY;

    aout << "Camera panned by (" << deltaX << ", " << deltaY << ") to ("
         << newOffsetX << ", " << newOffsetY << ")" << std::endl;
    notifyStateChange();
  }
}

const CameraState &CameraControlUseCase::getCurrentState() const {
  return currentState_;
}

void CameraControlUseCase::resetCamera() {
  currentState_ = CameraState(); // デフォルトコンストラクタでリセット

  aout << "Camera reset to initial state" << std::endl;
  notifyStateChange();
}

void CameraControlUseCase::setZoomLimits(float minZoom, float maxZoom) {
  minZoomLevel_ = std::max(0.05f, minZoom); // 最小値は0.05f
  maxZoomLevel_ = std::min(10.0f, maxZoom); // 最大値は10.0f

  // 現在のズームレベルも制限内に収める
  float clampedZoom = clampZoomLevel(currentState_.zoomLevel);
  if (clampedZoom != currentState_.zoomLevel) {
    currentState_.zoomLevel = clampedZoom;
    notifyStateChange();
  }

  aout << "Zoom limits set to [" << minZoomLevel_ << ", " << maxZoomLevel_
       << "]" << std::endl;
}

void CameraControlUseCase::setPanLimits(float minX, float maxX, float minY,
                                        float maxY) {
  minPanX_ = minX;
  maxPanX_ = maxX;
  minPanY_ = minY;
  maxPanY_ = maxY;
  panLimitsEnabled_ = true;

  // 現在のパン位置も制限内に収める
  float clampedX = currentState_.offsetX;
  float clampedY = currentState_.offsetY;
  clampPanPosition(clampedX, clampedY);

  if (clampedX != currentState_.offsetX || clampedY != currentState_.offsetY) {
    currentState_.offsetX = clampedX;
    currentState_.offsetY = clampedY;
    notifyStateChange();
  }

  aout << "Pan limits set to X[" << minPanX_ << ", " << maxPanX_ << "] Y["
       << minPanY_ << ", " << maxPanY_ << "]" << std::endl;
}

float CameraControlUseCase::clampZoomLevel(float zoomLevel) const {
  return std::max(minZoomLevel_, std::min(maxZoomLevel_, zoomLevel));
}

void CameraControlUseCase::clampPanPosition(float &x, float &y) const {
  if (panLimitsEnabled_) {
    x = std::max(minPanX_, std::min(maxPanX_, x));
    y = std::max(minPanY_, std::min(maxPanY_, y));
  }
}

void CameraControlUseCase::notifyStateChange() {
  if (stateChangeCallback_) {
    stateChangeCallback_(currentState_);
  }
}