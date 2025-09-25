#ifndef SIMULATION_GAME_CAMERA_CONTROL_USECASE_H
#define SIMULATION_GAME_CAMERA_CONTROL_USECASE_H

#include <functional>

/**
 * @brief カメラの状態情報
 */
struct CameraState {
    float offsetX;    // X軸オフセット（ワールド座標）
    float offsetY;    // Y軸オフセット（ワールド座標）
    float zoomLevel;  // ズームレベル（1.0が標準）
    
    CameraState() : offsetX(0.0f), offsetY(0.0f), zoomLevel(1.0f) {}
    CameraState(float x, float y, float zoom) : offsetX(x), offsetY(y), zoomLevel(zoom) {}
};

/**
 * @brief カメラ制御に関するユースケース
 * 
 * カメラのズームとパン（移動）を管理します。
 * ロングタップによるパン操作とピンチジェスチャーによるズーム操作を提供します。
 * 
 * 責任:
 * - カメラの位置とズームレベルの管理
 * - ズーム/パン操作の制限とバリデーション
 * - カメラ状態変更の通知
 */
class CameraControlUseCase {
public:
    /**
     * @brief カメラ状態変更イベントのコールバック型
     */
    using CameraStateChangeCallback = std::function<void(const CameraState& newState)>;

    /**
     * @brief コンストラクタ
     */
    CameraControlUseCase();

    /**
     * @brief カメラ状態変更コールバックを設定
     * @param callback カメラ状態変更時に呼び出されるコールバック
     */
    void setCameraStateChangeCallback(CameraStateChangeCallback callback);

    /**
     * @brief カメラの初期状態を設定
     * @param initialState 設定する初期カメラ状態
     */
    void setCameraInitialState(const CameraState& initialState);

    /**
     * @brief カメラの現在位置を更新（ズーム処理用）
     * @param offsetX 現在のX位置
     * @param offsetY 現在のY位置
     */
    void updateCurrentPosition(float offsetX, float offsetY);

    /**
     * @brief ロングタップによるカメラパン操作
     * @param worldX タップされたワールド座標のX位置
     * @param worldY タップされたワールド座標のY位置
     */
    void panCameraToPosition(float worldX, float worldY);

    /**
     * @brief ピンチジェスチャーによるズーム操作
     * @param scale ズームスケール（1.0が変化なし、1.0より大きいとズームイン、小さいとズームアウト）
     * @param centerX ピンチの中心スクリーン座標X
     * @param centerY ピンチの中心スクリーン座標Y
     */
    void zoomCamera(float scale, float centerX, float centerY);

    /**
     * @brief 相対的なカメラパン操作
     * @param deltaX X軸方向の移動量
     * @param deltaY Y軸方向の移動量
     */
    void panCameraBy(float deltaX, float deltaY);

    /**
     * @brief 現在のカメラ状態を取得
     * @return 現在のカメラ状態
     */
    const CameraState& getCurrentState() const;

    /**
     * @brief カメラ状態をリセット（初期位置・ズームに戻す）
     */
    void resetCamera();

    /**
     * @brief ズームレベルの制限を設定
     * @param minZoom 最小ズームレベル
     * @param maxZoom 最大ズームレベル
     */
    void setZoomLimits(float minZoom, float maxZoom);

    /**
     * @brief パン範囲の制限を設定
     * @param minX 最小X座標
     * @param maxX 最大X座標
     * @param minY 最小Y座標
     * @param maxY 最大Y座標
     */
    void setPanLimits(float minX, float maxX, float minY, float maxY);

private:
    CameraState currentState_;
    CameraStateChangeCallback stateChangeCallback_;
    
    // ズーム制限
    float minZoomLevel_;
    float maxZoomLevel_;
    
    // パン制限
    float minPanX_, maxPanX_;
    float minPanY_, maxPanY_;
    bool panLimitsEnabled_;
    
    /**
     * @brief ズームレベルを制限内に収める
     * @param zoomLevel 制限を適用するズームレベル
     * @return 制限内に収められたズームレベル
     */
    float clampZoomLevel(float zoomLevel) const;
    
    /**
     * @brief パン位置を制限内に収める
     * @param x 制限を適用するX座標
     * @param y 制限を適用するY座標
     */
    void clampPanPosition(float& x, float& y) const;
    
    /**
     * @brief カメラ状態が変更されたことを通知
     */
    void notifyStateChange();
};

#endif // SIMULATION_GAME_CAMERA_CONTROL_USECASE_H