#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include <EGL/egl.h>
#include <memory>

#include "Model.h"
#include "Shader.h"
#include "entities/UnitEntity.h"
#include "UnitRenderer.h"
#include "../../usecases/CombatUseCase.h"
#include "../../usecases/MovementUseCase.h"
// MovementField is used by Renderer as a concrete type for the movement field instance
#include "../../domain/services/MovementField.h"

struct android_app;

class Renderer {
public:
    /*!
     * @param pApp the android_app this Renderer belongs to, needed to configure GL
     */
    inline Renderer(android_app *pApp) :
            app_(pApp),
            display_(EGL_NO_DISPLAY),
            surface_(EGL_NO_SURFACE),
            context_(EGL_NO_CONTEXT),
            width_(0),
            height_(0),
            shaderNeedsNewProjectionMatrix_(true) {
        initRenderer();
    }

    virtual ~Renderer();

    /**
     * 入力イベントを処理します。
     *
     * このメソッドはフレームごとに呼ばれ、android_app のイベントキューからタッチ／キー／センサ入力を
     * 取り出して解釈します。Queue を消費するため、呼び出す側は同フレーム内で別途入力を再処理する必要はあり
     * ません。ハイレベルな動作は以下の通りです：
     *  - タッチ座標を screenToWorldCoordinates でゲーム座標に変換
     *  - ユニット選択や移動コマンドをユースケース（MovementUseCase / CombatUseCase）へ橋渡し
     *  - HUD ボタン（カメラ操作など）の領域判定
     *
     * 副作用: ユニットの選択状態や移動目標、カメラターゲットが変更される可能性があります。
     */
    void handleInput();

    /**
     * 描画処理（レンダループの1フレーム分）を実行します。
     *
     * 実装の責務:
     *  - ビューポートと射影行列の更新（必要時）
     *  - シェーダのバインドと共通マトリクスの設定
     *  - シーン内モデルと UnitRenderer の描画呼び出し
     *  - HUD（画面上レイヤ）の描画
     *
     * 副作用: elapsedTime_ の進行や、レンダリングに伴うデバッグログ出力が行われます。
     */
    void render();

    /**
     * ユニット描画用の UnitRenderer オブジェクトへの生ポインタを返します。
     *
     * 注意: 所有権は Renderer が持っています（返り値は借用ポインタ）。呼び出し側はポインタを保持
     * する際、Renderer のライフサイクルに注意してください（デストラクタ後は無効になります）。
     *
     * @return UnitRenderer* (借用ポインタ)
     */
    UnitRenderer* getUnitRenderer() const;

private:
    /**
     * OpenGL / EGL の初期化を行います。
     *
     * このメソッドは Renderer のコンストラクタから呼ばれ、EGLDisplay/EGLContext/EGLSurface
     * の作成、GL 初期状態（ブレンド、深度テスト、シェーダのプリロードなど）を行います。
     * 実装を変更することでレンダリングバックエンドやコンテキスト属性（バージョン、サンプル数）を
     * 調整できます。
     */
    void initRenderer();

    /**
     * フレーム毎にフレームバッファサイズを検査し、サイズが変化していればビューポートと射影行列を更新します。
     *
     * 変更が検出された場合は shaderNeedsNewProjectionMatrix_ をセットしてシェーダに新しい
     * 射影行列を伝播させます。ウィンドウ回転やウィンドウサイズ変更に対して正しい表示を保つため重要です。
     */
    void updateRenderArea();

    /**
     * サンプルシーン用のモデルを生成します。
     *
     * 実際のアプリではファイル（シーン定義、アセットバンドル等）からロードする処理を置く
     * 場所です。本サンプルでは簡易的に Model を構築して models_ に追加します。
     */
    void createModels();
    
    /**
     * 画面（ピクセル）座標をゲーム世界座標（ワールド座標）へ変換します。
     *
     * 変換は現在の cameraOffsetX_/cameraOffsetY_ やレンダリングスケール（projection）を考慮します。
     * タッチイベントの処理や UI からの座標問い合わせに使われます。
     *
     * @param screenX 画面X座標（ピクセル）
     * @param screenY 画面Y座標（ピクセル）
     * @param worldX 出力：ゲーム内X座標（参照渡し）
     * @param worldY 出力：ゲーム内Y座標（参照渡し）
     */
    void screenToWorldCoordinates(float screenX, float screenY, float& worldX, float& worldY) const;
    
    /**
     * 指定したゲーム内座標へプレイヤーユニット（または選択ユニット）を移動させるユースケース呼び出しを行います。
     *
     * 実装は MovementUseCase を通じてユニットの目標座標を設定し、必要ならカメラターゲットも更新します。
     * UI/Tap ハンドラが使用するエントリポイントです。
     *
     * @param x ゲーム内X座標
     * @param y ゲーム内Y座標
     */
    void moveUnitToPosition(float x, float y);

private:
    android_app *app_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    bool shaderNeedsNewProjectionMatrix_;

    std::unique_ptr<Shader> shader_;
    std::vector<Model> models_;
    
    // ユニット管理
    std::unique_ptr<UnitRenderer> unitRenderer_;
    std::vector<std::shared_ptr<UnitEntity>> units_;
    
    // ユースケース
    std::unique_ptr<CombatUseCase> combatUseCase_;
    std::unique_ptr<MovementUseCase> movementUseCase_;
    // Movement field for walkability and obstacles
    std::unique_ptr<class MovementField> movementField_;

    // Camera / view offset (world coordinates). These allow panning the view.
    float cameraOffsetX_ = 0.0f;
    float cameraOffsetY_ = 0.0f;

    // Smooth camera target and speed
    float cameraTargetX_ = 0.0f;
    float cameraTargetY_ = 0.0f;
    // units per second camera speed
    float cameraSpeed_ = 3.0f;

    // accumulated elapsed time since renderer start (seconds)
    float elapsedTime_ = 0.0f;

    // Simple HUD button rectangles (screen coordinates) for camera control.
    // Each button is represented as: x, y, width, height in pixels
    struct ButtonRect { int x, y, w, h; };
    ButtonRect btnUp_{0,0,0,0};
    ButtonRect btnDown_{0,0,0,0};
    ButtonRect btnLeft_{0,0,0,0};
    ButtonRect btnRight_{0,0,0,0};

    // HUD models drawn on top of everything
    std::vector<Model> hudModels_;
public:
    // Expose some read-only getters so JNI/UI can query status
    float getCameraOffsetX() const { return cameraOffsetX_; }
    float getCameraOffsetY() const { return cameraOffsetY_; }
    float getElapsedTime() const { return elapsedTime_; }
    // Public wrapper to convert screen coordinates (pixels) to world/game coordinates
    // Uses the existing private screenToWorldCoordinates implementation.
    void screenToWorld(float screenX, float screenY, float& worldX, float& worldY) const {
        screenToWorldCoordinates(screenX, screenY, worldX, worldY);
    }
    // Pan camera by dx, dy in world coordinates (adjusts camera target so smoothing applies)
    void panCameraBy(float dx, float dy) {
        cameraTargetX_ += dx;
        cameraTargetY_ += dy;
    }
};

#endif //ANDROIDGLINVESTIGATIONS_RENDERER_H