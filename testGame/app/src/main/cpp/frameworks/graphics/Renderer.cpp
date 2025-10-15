#include "Renderer.h"

/*
 * Renderer.cpp - engine-level rendering loop and scene orchestration.
 *
 * Responsibilities:
 * - Manage GL context, projection/model matrices and present frames.
 * - Coordinate use-cases (movement/combat updates) and call renderers
 * (UnitRenderer, models).
 * - Maintain camera offsets and expose them via JNI for UI overlays to query.
 *
 * Notes:
 * - This file mixes platform-specific GL initialization (EGL) and high-level
 * scene orchestration.
 * - Performance-sensitive: avoid heavy allocations per-frame.
 */

#include <GLES3/gl3.h>
#include <android/imagedecoder.h>
#include <chrono>
#include <cmath>
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <limits>
#include <memory>
#include <vector>

#include "../../domain/entities/GameMap.h"
#include "../android/TouchInputHandler.h"
#include "../third_party/json.hpp"
#include "../usecases/CameraControlUseCase.h"
#include "Model.h"
#include "Shader.h"
#include "TextureAsset.h"
#include "TileMapLoader.h"
#include "android/AndroidOut.h"
#include "utils/Utility.h"
#include <android/asset_manager.h>

// JNI関数の前方宣言
extern "C" void setRendererReference(Renderer *renderer);

// Ensure panCameraBy is available if not inlined in header (no-op if already
// provided) (The method is implemented inline in Renderer.h; this symbol is
// here just in case of link expectations.) No additional implementation
// required in the cpp file.

static const char *vertex = R"vertex(#version 300 es
// 入力頂点属性 - 明示的なロケーションを指定
layout(location = 0) in vec3 inPosition;  // 頂点位置
layout(location = 1) in vec2 inUV;        // テクスチャ座標

// フラグメントシェーダーへの出力
out vec2 fragUV;

// 変換行列
uniform mat4 uProjection;  // 投影行列
uniform mat4 uView;        // ビュー行列（カメラ変換）
uniform mat4 uModel;       // モデル行列

void main() {
    // テクスチャ座標をそのまま出力（必ず使うようにする）
    fragUV = inUV;
    
    // 頂点位置を計算（正しいMVP変換）
    gl_Position = uProjection * uView * uModel * vec4(inPosition, 1.0);
}
)vertex";

// Fragment shader, you'd typically load this from assets
static const char *fragment = R"fragment(#version 300 es
precision mediump float;

// 頂点シェーダーから入力として受け取る変数
in vec2 fragUV;

// 出力色
out vec4 outColor;

// テクスチャサンプラー
uniform sampler2D uTexture;

void main() {
    // テクスチャから色を取得
    vec4 texColor = texture(uTexture, fragUV);
    
    // アルファブレンドを適用（テクスチャのアルファを尊重）
    outColor = texColor;
}
)fragment";

/*!
 * Half the height of the projection matrix.
 * この値を大きくして、より広いエリアを描画可能にする
 * 5.0fに設定すると、縦方向が-5から5までの範囲で描画可能になる
 */
static constexpr float kProjectionHalfHeight = 5.0f;

/*!
 * The near plane distance for the projection matrix. Since this is an
 * orthographic projection matrix, it's convenient to have negative values for
 * sorting (and avoiding z-fighting at 0). 奥行き方向の描画範囲も拡張
 */
static constexpr float kProjectionNearPlane = -10.f;

/*!
 * The far plane distance for the projection matrix. Since this is an
 * orthographic porjection matrix, it's convenient to have the far plane
 * equidistant from 0 as the near plane. 奥行き方向の描画範囲も拡張
 */
static constexpr float kProjectionFarPlane = 10.f;

Renderer::~Renderer() {
  if (display_ != EGL_NO_DISPLAY) {
    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (context_ != EGL_NO_CONTEXT) {
      eglDestroyContext(display_, context_);
      context_ = EGL_NO_CONTEXT;
    }
    if (surface_ != EGL_NO_SURFACE) {
      eglDestroySurface(display_, surface_);
      surface_ = EGL_NO_SURFACE;
    }
    eglTerminate(display_);
    display_ = EGL_NO_DISPLAY;
  }
}

float Renderer::calculateDeltaTime() {
  // フレーム間隔を測定し、極端な大きさを避ける
  static auto lastTime = std::chrono::high_resolution_clock::now();
  const auto currentTime = std::chrono::high_resolution_clock::now();
  const float rawDelta =
      std::chrono::duration<float>(currentTime - lastTime).count();
  lastTime = currentTime;
  return std::min(rawDelta, 0.016f);
}

void Renderer::updateGameState(float deltaTime) {
  if (movementUseCase_) {
    movementUseCase_->updateMovements(deltaTime);
  }

  if (combatUseCase_) {
    combatUseCase_->executeAutoCombat();
    combatUseCase_->removeDeadUnits();
  }

  resolveCombatEngagements();

  if (unitRenderer_) {
    unitRenderer_->updateUnits(deltaTime);
  }

  updateCameraSmoothing(deltaTime);

  elapsedTime_ += deltaTime;
}

void Renderer::updateCameraSmoothing(float deltaTime) {
  // カメラターゲットへ滑らかに追従する
  const float toX = cameraTargetX_ - cameraOffsetX_;
  const float toY = cameraTargetY_ - cameraOffsetY_;
  const float maxStep = cameraSpeed_ * deltaTime;
  const float dist = std::sqrt(toX * toX + toY * toY);

  if (cameraTargetX_ == 0.0f && cameraTargetY_ == 0.0f &&
      (cameraOffsetX_ != 0.0f || cameraOffsetY_ != 0.0f)) {
    aout << "WARNING: Camera target unexpectedly reset to (0,0)! Current "
         "offset: (" << cameraOffsetX_ << ", " << cameraOffsetY_ << ")"
         << std::endl;
  }

  if (dist <= maxStep || dist == 0.0f) {
    cameraOffsetX_ = cameraTargetX_;
    cameraOffsetY_ = cameraTargetY_;
  } else if (dist > 0.0f) {
    cameraOffsetX_ += toX / dist * maxStep;
    cameraOffsetY_ += toY / dist * maxStep;
  }
}

void Renderer::resolveCombatEngagements() {
  // 全ユニット間の距離を測り、射程内であれば戦闘状態に遷移させる
  if (units_.size() < 2) {
    return;
  }

  for (size_t i = 0; i < units_.size(); ++i) {
    for (size_t j = i + 1; j < units_.size(); ++j) {
      auto &unit1 = units_[i];
      auto &unit2 = units_[j];

      if (!unit1->isAlive() || !unit2->isAlive()) {
        continue;
      }

      const auto pos1 = unit1->getPosition();
      const auto pos2 = unit2->getPosition();
      const float dx = pos2.getX() - pos1.getX();
      const float dy = pos2.getY() - pos1.getY();
      const float distance = std::sqrt(dx * dx + dy * dy);

      const bool unit1InRange =
          distance <= (unit1->getStats().getAttackRange() +
                       unit2->getStats().getCollisionRadius());
      const bool unit2InRange =
          distance <= (unit2->getStats().getAttackRange() +
                       unit1->getStats().getCollisionRadius());

      if (!unit1InRange && !unit2InRange) {
        continue;
      }

      if (unit1InRange && (unit1->getState() == UnitState::IDLE ||
                           unit1->getState() == UnitState::COMBAT)) {
        unit1->setState(UnitState::COMBAT);
        aout << unit1->getName() << " entering combat with "
             << unit2->getName() << std::endl;
      }

      if (unit2InRange && (unit2->getState() == UnitState::IDLE ||
                           unit2->getState() == UnitState::COMBAT)) {
        unit2->setState(UnitState::COMBAT);
        aout << unit2->getName() << " entering combat with "
             << unit1->getName() << std::endl;
      }
    }
  }
}

/**
 * 描画ループの1フレーム分を実行します。
 *
 * この関数はゲームロジック（movement/combat のアップデート）をトリガーし、
 * プロジェクションの更新、シェーダの準備、シーンの描画（背景、ユニット、HUD）を順に行います。
 * 副作用として cameraOffset_ の滑らかな補間や elapsedTime_
 * の増分、ユニットの状態遷移を行います。
 */
void Renderer::render() {
  const float deltaTime = calculateDeltaTime();

  updateGameState(deltaTime);

  // Check to see if the surface has changed size. This is _necessary_ to do
  // every frame when using immersive mode as you'll get no other notification
  // that your renderable area has changed.
  updateRenderArea();

  // When the renderable area changes, the projection matrix has to also be
  // updated. This is true even if you change from the sample orthographic
  // projection matrix as your aspect ratio has likely changed.
  if (shaderNeedsNewProjectionMatrix_) {
    // a placeholder projection matrix allocated on the stack. Column-major
    // memory layout
    float projectionMatrix[16] = {0};

    // build an orthographic projection matrix for 2d rendering (with zoom
    // support)
    Utility::buildOrthographicMatrix(
        projectionMatrix,
        kProjectionHalfHeight / cameraZoom_, // ズームレベルで除算
        float(width_) / height_, kProjectionNearPlane, kProjectionFarPlane);

    // send the matrix to the shader
    // Note: the shader must be active for this to work. Since we only have one
    // shader for this demo, we can assume that it's active.
    shader_->setProjectionMatrix(projectionMatrix);

    // make sure the matrix isn't generated every frame
    shaderNeedsNewProjectionMatrix_ = false;
  }

  // すでに設定した色で背景をクリア（initRendererで設定した色）
  glClear(GL_COLOR_BUFFER_BIT);

  // 単位行列を作成してシェーダーに設定（デフォルトの変換なし）
  float identityMatrix[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

  // シェーダーがアクティブか確認
  shader_->activate();

  // ビュー行列を使用してカメラオフセットを適用
  float viewMatrix[16];
  for (int i = 0; i < 16; ++i)
    viewMatrix[i] = identityMatrix[i];
  // カメラの逆移動を適用（カメラが右に移動 = 世界が左に見える）
  viewMatrix[12] = -cameraOffsetX_;
  viewMatrix[13] = -cameraOffsetY_;

  // モデル行列は単位行列のまま
  float modelMatrix[16];
  for (int i = 0; i < 16; ++i)
    modelMatrix[i] = identityMatrix[i];

  // シェーダーに行列を設定
  shader_->setViewMatrix(viewMatrix);
  shader_->setModelMatrix(modelMatrix);

  // デバッグログ
  aout << "Begin rendering frame..." << std::endl;

  // 背景モデルのレンダリング
  if (!models_.empty()) {
    aout << "Drawing " << models_.size() << " background models" << std::endl;
    for (const auto &model : models_) {
      shader_->drawModel(model);
    }
  } else {
    aout << "No background models to draw!" << std::endl;
  }

  // ユニットを描画（更新処理は描画前に完了済み）
  if (unitRenderer_) {
    aout << "Drawing units..." << std::endl;
    unitRenderer_->render(shader_.get(), cameraZoom_);
  } else {
    aout << "unitRenderer_ is null!" << std::endl;
  }

  // HUDモデルを最後に描画（画面固定位置で表示）
  if (!hudModels_.empty()) {
    // HUD用に単位ビュー行列を設定（カメラオフセットなし）
    float identityView[16];
    for (int i = 0; i < 16; ++i)
      identityView[i] = identityMatrix[i];

    shader_->setViewMatrix(identityView);

    for (const auto &m : hudModels_) {
      shader_->drawModel(m);
    }

    // ワールド用ビュー行列を復元
    float worldViewMatrix[16];
    for (int i = 0; i < 16; ++i)
      worldViewMatrix[i] = identityMatrix[i];
    worldViewMatrix[12] = -cameraOffsetX_;
    worldViewMatrix[13] = -cameraOffsetY_;

    shader_->setViewMatrix(worldViewMatrix);
  }

  aout << "Frame rendering complete" << std::endl;

  // Present the rendered image. This is an implicit glFlush.
  auto swapResult = eglSwapBuffers(display_, surface_);
  assert(swapResult == EGL_TRUE);
}

void Renderer::initRenderer() {
  // Choose your render attributes
  constexpr EGLint attribs[] = {EGL_RENDERABLE_TYPE,
                                EGL_OPENGL_ES3_BIT,
                                EGL_SURFACE_TYPE,
                                EGL_WINDOW_BIT,
                                EGL_BLUE_SIZE,
                                8,
                                EGL_GREEN_SIZE,
                                8,
                                EGL_RED_SIZE,
                                8,
                                EGL_DEPTH_SIZE,
                                24,
                                EGL_NONE};

  // The default display is probably what you want on Android
  auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  eglInitialize(display, nullptr, nullptr);

  // figure out how many configs there are
  EGLint numConfigs;
  eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);

  // get the list of configurations
  std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
  eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs,
                  &numConfigs);

  // Find a config we like.
  // Could likely just grab the first if we don't care about anything else in
  // the config. Otherwise hook in your own heuristic
  auto config = *std::find_if(
      supportedConfigs.get(), supportedConfigs.get() + numConfigs,
      [&display](const EGLConfig &config) {
        EGLint red, green, blue, depth;
        if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red) &&
            eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green) &&
            eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue) &&
            eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth)) {

          aout << "Found config with " << red << ", " << green << ", " << blue
               << ", " << depth << std::endl;
          return red == 8 && green == 8 && blue == 8 && depth == 24;
        }
        return false;
      });

  aout << "Found " << numConfigs << " configs" << std::endl;
  aout << "Chose " << config << std::endl;

  // create the proper window surface
  EGLint format;
  eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
  EGLSurface surface =
      eglCreateWindowSurface(display, config, app_->window, nullptr);

  // Create a GLES 3 context
  EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
  EGLContext context =
      eglCreateContext(display, config, nullptr, contextAttribs);

  // get some window metrics
  auto madeCurrent = eglMakeCurrent(display, surface, surface, context);
  assert(madeCurrent);

  display_ = display;
  surface_ = surface;
  context_ = context;

  // make width and height invalid so it gets updated the first frame in @a
  // updateRenderArea()
  width_ = -1;
  height_ = -1;

  // Log basic GL strings (avoid undefined macro usage)
  const char *glVendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
  const char *glRenderer =
      reinterpret_cast<const char *>(glGetString(GL_RENDERER));
  const char *glVersion =
      reinterpret_cast<const char *>(glGetString(GL_VERSION));
  aout << "GL_VENDOR: " << (glVendor ? glVendor : "unknown") << std::endl;
  aout << "GL_RENDERER: " << (glRenderer ? glRenderer : "unknown") << std::endl;
  aout << "GL_VERSION: " << (glVersion ? glVersion : "unknown") << std::endl;

  // Print extensions using glGetStringi where available (GLES3)
  GLint numExt = 0;
  glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
  aout << "GL_EXTENSIONS (" << numExt << "):";
  for (GLint ei = 0; ei < numExt; ++ei) {
    const char *ext =
        reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, ei));
    if (ext)
      aout << " " << ext;
  }
  aout << std::endl;

  // OpenGLのエラーをクリア
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    aout << "OpenGL error before shader load: " << err << std::endl;
  }

  // シェーダーの読み込み時にエラーチェックを強化
  shader_ = std::unique_ptr<Shader>(Shader::loadShader(
      vertex, fragment, "inPosition", "inUV", "uProjection", "uModel"));

  if (!shader_) {
    aout << "ERROR: Failed to load shader program!" << std::endl;

    // エラーの詳細を取得
    while ((err = glGetError()) != GL_NO_ERROR) {
      aout << "OpenGL error during shader creation: " << err << std::endl;
    }

    assert(shader_);
  } else {
    aout << "Shader program loaded successfully" << std::endl;

    // テクスチャサンプラーのUniformロケーションを設定
    GLuint program = shader_->getProgramID();
    GLint textureLoc = glGetUniformLocation(program, "uTexture");
    if (textureLoc != -1) {
      shader_->activate();
      glUniform1i(textureLoc, 0); // テクスチャユニット0に設定
      aout << "Set uTexture uniform to texture unit 0" << std::endl;
    } else {
      aout << "Warning: Could not find uTexture uniform in shader" << std::endl;
    }
  }

  // シェーダーをアクティブ化
  shader_->activate();

  // 明確に見える背景色を設定（明るい緑色）
  glClearColor(0.0f, 0.8f, 0.0f, 1.0f); // 明るい緑色で明らかに見えるように

  // enable alpha globally for now, you probably don't want to do this in a game
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // get some demo models into memory
  createModels();

  // JNI用のRenderer参照を設定
  setRendererReference(this);

  // 新しいタッチ入力ハンドラーの初期化
  touchInputHandler_ = std::make_unique<TouchInputHandler>();
  touchInputHandler_->setTouchEventCallback(
      [this](const TouchEvent &event) { handleTouchEvent(event); });

  // カメラ制御ユースケースの初期化
  cameraControlUseCase_ = std::make_unique<CameraControlUseCase>();

  // コールバックを先に設定
  cameraControlUseCase_->setCameraStateChangeCallback(
      [this](const CameraState &newState) { updateCameraFromState(newState); });

  // レンダラーの現在のカメラ状態をユースケースに同期（コールバック設定後）
  CameraState currentRendererState(cameraOffsetX_, cameraOffsetY_, cameraZoom_);
  cameraControlUseCase_->setCameraInitialState(currentRendererState);

  aout << "Touch input and camera control systems initialized" << std::endl;
}

void Renderer::updateRenderArea() {
  EGLint width;
  eglQuerySurface(display_, surface_, EGL_WIDTH, &width);

  EGLint height;
  eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);

  if (width != width_ || height != height_) {
    width_ = width;
    height_ = height;
    glViewport(0, 0, width, height);

    // make sure that we lazily recreate the projection matrix before we render
    shaderNeedsNewProjectionMatrix_ = true;
  }

  // Initialize HUD button rectangles (screen-space) at bottom-right corner
  // Button size in pixels (square)
  const int btnSize = 96;
  const int padding = 16;
  // Place a 2x2 grid in bottom-right: [Up] above [Down], [Left] left of [Right]
  btnRight_ = {width_ - padding - btnSize, height_ - padding - btnSize, btnSize,
               btnSize};
  btnLeft_ = {width_ - padding - 2 * btnSize - 8, height_ - padding - btnSize,
              btnSize, btnSize};
  btnUp_ = {width_ - padding - btnSize, height_ - padding - 2 * btnSize - 8,
            btnSize, btnSize};
  btnDown_ = {width_ - padding - btnSize, height_ - padding, btnSize, btnSize};
}

/**
 * @brief Create any demo models we want for this demo.
 */
void Renderer::createModels() {
  /*
   * This is a square that covers the entire screen:
   * 0 --- 1
   * | \   |
   * |  \  |
   * |   \ |
   * 3 --- 2
   */
  models_.clear();

  std::shared_ptr<TextureAsset> fallbackTexture =
      TextureAsset::createSolidColorTexture(0.1f, 0.1f, 0.3f);
  std::shared_ptr<TextureAsset> mapTexture;

  if (app_ && app_->activity && app_->activity->assetManager) {
    constexpr float kTileSize = 1.0f;
    auto mapResult = TileMapLoader::loadFromPng(app_->activity->assetManager,
                                                "maps/demo_map.png", kTileSize);
    if (mapResult) {
      gameMap_ = mapResult->map;
      mapTexture = mapResult->texture;

      std::vector<Vertex> mapVertices = {
          Vertex(Vector3{gameMap_->getMaxX(), gameMap_->getMaxY(), 0.0f},
                 Vector2{1.0f, 1.0f}),
          Vertex(Vector3{gameMap_->getMinX(), gameMap_->getMaxY(), 0.0f},
                 Vector2{0.0f, 1.0f}),
          Vertex(Vector3{gameMap_->getMinX(), gameMap_->getMinY(), 0.0f},
                 Vector2{0.0f, 0.0f}),
          Vertex(Vector3{gameMap_->getMaxX(), gameMap_->getMinY(), 0.0f},
                 Vector2{1.0f, 0.0f})};
      std::vector<Index> mapIndices = {0, 1, 2, 0, 2, 3};
      models_.emplace_back(mapVertices, mapIndices, mapTexture);

      movementField_ = std::make_unique<MovementField>(
          gameMap_->getMinX(), gameMap_->getMinY(), gameMap_->getMaxX(),
          gameMap_->getMaxY());

      const float centerX = (gameMap_->getMinX() + gameMap_->getMaxX()) * 0.5f;
      const float centerY = (gameMap_->getMinY() + gameMap_->getMaxY()) * 0.5f;
      cameraOffsetX_ = cameraTargetX_ = centerX;
      cameraOffsetY_ = cameraTargetY_ = centerY;
      shaderNeedsNewProjectionMatrix_ = true;

      aout << "Renderer: map loaded with bounds X(" << gameMap_->getMinX()
           << ", " << gameMap_->getMaxX() << ") Y(" << gameMap_->getMinY()
           << ", " << gameMap_->getMaxY() << ")" << std::endl;
    } else {
      gameMap_.reset();
      aout << "Renderer: failed to load maps/demo_map.png, falling back to "
              "solid background"
           << std::endl;
    }
  }

  if (!movementField_) {
    movementField_ = std::make_unique<MovementField>(-6.0f, -6.0f, 6.0f, 6.0f);
  }

  if (models_.empty()) {
    std::vector<Vertex> fallbackVertices = {
        Vertex(Vector3{1, 1, 0}, Vector2{1, 0}),
        Vertex(Vector3{-1, 1, 0}, Vector2{0, 0}),
        Vertex(Vector3{-1, -1, 0}, Vector2{0, 1}),
        Vertex(Vector3{1, -1, 0}, Vector2{1, 1})};
    std::vector<Index> fallbackIndices = {0, 1, 2, 0, 2, 3};
    models_.emplace_back(fallbackVertices, fallbackIndices, fallbackTexture);
  }

  // ユニットレンダラーを初期化（単色テクスチャ）
  unitRenderer_ = std::make_unique<UnitRenderer>(
      TextureAsset::createSolidColorTexture(0.6f, 0.6f, 0.6f));
  // デバッグ用途: 当たり判定ワイヤーフレームを常に表示
  unitRenderer_->setShowCollisionWireframes(true);
  // デバッグ用途: 攻撃範囲も表示
  unitRenderer_->setShowAttackRanges(true);

  // Try to load unit spawn configuration from assets/unit_spawns.json
  bool loadedFromJson = false;
  if (app_ && app_->activity && app_->activity->assetManager) {
    AAssetManager *mgr = app_->activity->assetManager;
    AAsset *asset =
        AAssetManager_open(mgr, "unit_spawns.json", AASSET_MODE_STREAMING);
    if (asset) {
      off_t size = AAsset_getLength(asset);
      std::string content;
      content.resize(size);
      int read = AAsset_read(asset, &content[0], size);
      AAsset_close(asset);
      if (read > 0) {
        try {
          auto root = mini_json::parseString(content);
          if (root && root->isObject()) {
            auto it = root->objectValue.find("units");
            if (it != root->objectValue.end() && it->second->isArray()) {
              auto &arr = it->second->arrayValue;
              for (auto &item : arr) {
                if (!item || !item->isObject())
                  continue;
                auto &obj = item->objectValue;
                int id = 0;
                std::string name = "Unit";
                float x = 0.0f, y = 0.0f;
                int faction = 0;
                int maxHp = 100, currentHp = 100, minAtk = 1, maxAtk = 1;
                float moveSpeed = 1.0f, attackSpeed = 1.0f, defense = 0.0f,
                      collisionRadius = 0.25f;

                if (obj.find("id") != obj.end() && obj["id"]->isNumber())
                  id = static_cast<int>(obj["id"]->numberValue);
                if (obj.find("name") != obj.end() && obj["name"]->isString())
                  name = obj["name"]->stringValue;
                if (obj.find("x") != obj.end() && obj["x"]->isNumber())
                  x = static_cast<float>(obj["x"]->numberValue);
                if (obj.find("y") != obj.end() && obj["y"]->isNumber())
                  y = static_cast<float>(obj["y"]->numberValue);
                if (obj.find("faction") != obj.end() &&
                    obj["faction"]->isNumber())
                  faction = static_cast<int>(obj["faction"]->numberValue);

                auto sIt = obj.find("stats");
                if (sIt != obj.end() && sIt->second->isObject()) {
                  auto &sObj = sIt->second->objectValue;
                  if (sObj.find("maxHp") != sObj.end() &&
                      sObj["maxHp"]->isNumber())
                    maxHp = static_cast<int>(sObj["maxHp"]->numberValue);
                  if (sObj.find("currentHp") != sObj.end() &&
                      sObj["currentHp"]->isNumber())
                    currentHp =
                        static_cast<int>(sObj["currentHp"]->numberValue);
                  if (sObj.find("minAttack") != sObj.end() &&
                      sObj["minAttack"]->isNumber())
                    minAtk = static_cast<int>(sObj["minAttack"]->numberValue);
                  if (sObj.find("maxAttack") != sObj.end() &&
                      sObj["maxAttack"]->isNumber())
                    maxAtk = static_cast<int>(sObj["maxAttack"]->numberValue);
                  if (sObj.find("moveSpeed") != sObj.end() &&
                      sObj["moveSpeed"]->isNumber())
                    moveSpeed =
                        static_cast<float>(sObj["moveSpeed"]->numberValue);
                  if (sObj.find("attackSpeed") != sObj.end() &&
                      sObj["attackSpeed"]->isNumber())
                    attackSpeed =
                        static_cast<float>(sObj["attackSpeed"]->numberValue);
                  if (sObj.find("defense") != sObj.end() &&
                      sObj["defense"]->isNumber())
                    defense = static_cast<float>(sObj["defense"]->numberValue);
                  if (sObj.find("collisionRadius") != sObj.end() &&
                      sObj["collisionRadius"]->isNumber())
                    collisionRadius = static_cast<float>(
                        sObj["collisionRadius"]->numberValue);
                }

                UnitStats stats(maxHp, currentHp, minAtk, maxAtk, moveSpeed,
                                attackSpeed, defense, collisionRadius);
                auto u = std::make_shared<UnitEntity>(id, name, Position(x, y),
                                                      stats, faction);
                units_.push_back(u);

                if (faction == 1)
                  unitRenderer_->registerUnitWithColor(u, 1.0f, 0.3f, 0.3f);
                else if (faction == 2)
                  unitRenderer_->registerUnitWithColor(u, 0.3f, 0.3f, 1.0f);
                else
                  unitRenderer_->registerUnitWithColor(u, 0.6f, 0.6f, 0.6f);
              }
              loadedFromJson = true;
            }
          }
        } catch (const std::exception &e) {
          aout << "Failed to parse unit_spawns.json: " << e.what() << std::endl;
        }
      }
    }
  }

  if (!loadedFromJson) {
    // Fallback: hardcoded spawn (keeps previous behaviour)
    std::vector<std::shared_ptr<UnitEntity>> faction1Units;
    std::vector<std::shared_ptr<UnitEntity>> faction2Units;

    // 配置パターン（左右に分ける）
    std::array<float, 3> yOffsets = {1.5f, 0.0f, -1.5f};
    int idCounter = 1;

    for (int i = 0; i < 3; ++i) {
      auto u = std::make_shared<UnitEntity>(
          idCounter++, "PlayerUnit" + std::to_string(i + 1),
          Position(-2.0f, yOffsets[i]),
          UnitStats(120, 120, 6, 10, 1.0f, 0.6f, 0.5f, 1.0f),
          1); // faction 1
      units_.push_back(u);
      faction1Units.push_back(u);
      unitRenderer_->registerUnitWithColor(u, 1.0f, 0.3f, 0.3f); // 赤
    }

    for (int i = 0; i < 3; ++i) {
      auto u = std::make_shared<UnitEntity>(
          idCounter++, "EnemyUnit" + std::to_string(i + 1),
          Position(2.0f, yOffsets[i]),
          UnitStats(100, 100, 4, 8, 1.0f, 0.7f, 0.4f, 1.0f),
          2); // faction 2
      units_.push_back(u);
      faction2Units.push_back(u);
      unitRenderer_->registerUnitWithColor(u, 0.3f, 0.3f, 1.0f); // 青
    }
  }

  // If we have at least 3 units, compute a sample distance between units[1] and
  // units[2]
  if (units_.size() >= 3 && units_[1] && units_[2]) {
    auto pos2 = units_[1]->getPosition();
    auto pos3 = units_[2]->getPosition();
    float dx = pos3.getX() - pos2.getX();
    float dy = pos3.getY() - pos2.getY();
    float distance = std::sqrt(dx * dx + dy * dy);

    // Compute a reasonable combat distance for logging
    const float COLLISION_RADIUS = 0.1f;
    float combatDistance = COLLISION_RADIUS * 2.0f + 0.01f;

    aout << "Initial distance between sample units: " << distance << std::endl;
    aout << "Combat distance: " << combatDistance << std::endl;
    aout << "Collision radius: " << COLLISION_RADIUS << std::endl;
  } else {
    aout << "Not enough units to compute sample pair distance" << std::endl;
  }

  aout << "Created " << units_.size() << " units" << std::endl;
  aout << "Spawned 2 factions with 3 units each" << std::endl;

  if (!movementField_) {
    // フォールバック:
    // 広い移動フィールドを用意（マップが読み込めなかったケース）
    movementField_ =
        std::make_unique<MovementField>(-100.0f, -100.0f, 100.0f, 100.0f);
  }

  // NOTE: Per request, we remove all circular obstacles from the field (no
  // addCircleObstacle calls)
  // この変更により、フィールド上の障害物は存在しません。

  // HUD：カメラパン用の簡易ボタンモデルを追加します（画面右下に上下左右）
  // ボタンは画面空間で判定するため、ここでは単純なワールド座標の四角を作成しておきます
  auto addButtonModel = [&](float centerX, float centerY, float size, float r,
                            float g, float b) {
    float half = size * 0.5f;
    std::vector<Vertex> verts = {
        Vertex(Vector3{centerX + half, centerY + half, 0}, Vector2{1, 0}),
        Vertex(Vector3{centerX - half, centerY + half, 0}, Vector2{0, 0}),
        Vertex(Vector3{centerX - half, centerY - half, 0}, Vector2{0, 1}),
        Vertex(Vector3{centerX + half, centerY - half, 0}, Vector2{1, 1})};
    std::vector<Index> inds = {0, 1, 2, 0, 2, 3};
    auto tex = TextureAsset::createSolidColorTexture(r, g, b);
    models_.emplace_back(verts, inds, tex);
  };

  // ワールド空間上に小さな四角を並べる（見た目用）
  addButtonModel(3.5f, -3.5f, 0.6f, 0.8f, 0.8f,
                 0.8f); // placeholder for up/down/left/right group

  // 実際の画面領域でのボタン矩形は updateRenderArea()
  // 後に決定するので、Renderer.h の ButtonRect を使って handleInput
  // 内でスクリーン座標判定を行います。

  // ユースケースを初期化（MovementField を注入）
  combatUseCase_ = std::make_unique<CombatUseCase>(units_);
  movementUseCase_ = std::make_unique<MovementUseCase>(
      units_, movementField_.get(), gameMap_.get());

  // 戦闘イベントのコールバックを設定
  combatUseCase_->setCombatEventCallback(
      [this](const UnitEntity &attacker, const UnitEntity &target,
             const CombatDomainService::CombatResult &result) {
        aout << "Combat: Unit " << attacker.getId() << " attacked Unit "
             << target.getId() << " for " << result.damageDealt << " damage"
             << std::endl;
        if (result.targetKilled) {
          aout << "Unit " << target.getId() << " was killed!" << std::endl;
        }
        if (result.attackerKilled) {
          aout << "Unit " << attacker.getId()
               << " was killed by counter attack!" << std::endl;
        }
      });

  // 移動イベントのコールバックを設定
  movementUseCase_->setMovementEventCallback(
      [this](const UnitEntity &unit, const Position &from, const Position &to) {
        aout << "Movement: Unit " << unit.getId() << " moved from ("
             << from.getX() << ", " << from.getY() << ") to (" << to.getX()
             << ", " << to.getY() << ")" << std::endl;
      });

  movementUseCase_->setMovementFailedCallback(
      [this](const UnitEntity &unit, const Position &target,
             const std::string &reason) {
        aout << "Movement Failed: Unit " << unit.getId()
             << " could not move to (" << target.getX() << ", " << target.getY()
             << ") - " << reason << std::endl;
      });
}

/**
 * 入力キューを読み取り、新しいタッチ入力システムを通じて処理します。
 *
 * 新しいシステムでは TouchInputHandler
 * がジェスチャーを識別し、適切なユースケース （MovementUseCase,
 * CameraControlUseCase）に振り分けます。
 *
 * 注意: この関数は inputBuffer
 * を消費するため、呼び出し元は別途同入力を参照しません。
 */
void Renderer::handleInput() {
  // TouchInputHandler の更新（タイマーベースの処理のため）
  if (touchInputHandler_) {
    touchInputHandler_->update();
  }

  // handle all queued inputs
  auto *inputBuffer = android_app_swap_input_buffers(app_);
  if (!inputBuffer) {
    // no inputs yet.
    return;
  }

  // デバッグ：入力バッファの状態を確認
  if (inputBuffer->motionEventsCount > 0) {
    aout << "INPUT_DEBUG: Processing " << inputBuffer->motionEventsCount
         << " motion events" << std::endl;
  }

  // handle motion events through the new touch input system
  for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
    auto &motionEvent = inputBuffer->motionEvents[i];

    // 新しいタッチ入力ハンドラーで処理
    if (touchInputHandler_) {
      touchInputHandler_->handleMotionEvent(motionEvent);
    }
  }
  // clear the motion input count in this buffer for main thread to re-use.
  android_app_clear_motion_events(inputBuffer);

  // handle input key events (キーイベント処理は従来どおり)
  for (auto i = 0; i < inputBuffer->keyEventsCount; i++) {
    auto &keyEvent = inputBuffer->keyEvents[i];
    aout << "Key: " << keyEvent.keyCode << " ";
    switch (keyEvent.action) {
    case AKEY_EVENT_ACTION_DOWN:
      aout << "Key Down";
      break;
    case AKEY_EVENT_ACTION_UP:
      aout << "Key Up";
      break;
    case AKEY_EVENT_ACTION_MULTIPLE:
      // Deprecated since Android API level 29.
      aout << "Multiple Key Actions";
      break;
    default:
      aout << "Unknown KeyEvent Action: " << keyEvent.action;
    }
    aout << std::endl;
  }
  // clear the key input count too.
  android_app_clear_key_events(inputBuffer);
}

/**
 * スクリーン（ピクセル）座標をワールド座標へ変換します。
 *
 * @param screenX スクリーンX（px）
 * @param screenY スクリーンY（px）
 * @param worldX 変換後のワールドX（参照返し）
 * @param worldY 変換後のワールドY（参照返し）
 *
 * 実装は現在のプロジェクション設定（kProjectionHalfHeight
 * とアスペクト比）を使い、
 * カメラオフセットを差し引いてワールド座標を復元します。
 */
void Renderer::screenToWorldCoordinates(float screenX, float screenY,
                                        float &worldX, float &worldY) const {
  // Convert screen pixel coords -> normalized device coords (NDC)
  // screen: (0,0) top-left, (width_, height_) bottom-right
  // NDC: x in [-1,1], y in [-1,1] with +y up
  if (width_ <= 0 || height_ <= 0) {
    worldX = 0.0f;
    worldY = 0.0f;
    return;
  }

  // Screen to NDC conversion
  float ndcX = (screenX / static_cast<float>(width_)) * 2.0f - 1.0f;
  float ndcY = 1.0f - (screenY / static_cast<float>(height_)) * 2.0f;

  // 投影行列と同じパラメータを使用
  const float halfHeight = kProjectionHalfHeight / cameraZoom_;
  const float aspect = static_cast<float>(width_) / static_cast<float>(height_);
  const float halfWidth = halfHeight * aspect;

  // 投影行列の逆変換: NDC → ビュー座標
  // 投影行列: [1/halfWidth, 0, 0, 0; 0, 1/halfHeight, 0, 0; ...]
  // 逆変換: viewX = ndcX * halfWidth, viewY = ndcY * halfHeight
  float viewX = ndcX * halfWidth;
  float viewY = ndcY * halfHeight;

  // ビュー行列の逆変換: ビュー座標 → ワールド座標
  // ビュー行列: viewPos = worldPos - cameraOffset
  // 逆変換: worldPos = viewPos + cameraOffset
  worldX = viewX + cameraOffsetX_;
  worldY = viewY + cameraOffsetY_;

  // デバッグ情報（簡略化）
  aout << "Screen(" << screenX << ", " << screenY << ") -> World(" << worldX
       << ", " << worldY << ")" << std::endl;
}

/**
 * 指定したワールド座標にユニットを移動させるためのユースケース呼び出しを行います。
 *
 * 処理の流れ:
 *  - まず座標近傍のユニットをヒット判定（簡易ヒットボックス）で検索
 *  -
 * ユニットが見つかればそのユニットを移動させる。見つからなければプレイヤー陣営の
 *    最初の生存ユニットを移動させる
 *
 * @param x ワールドX
 * @param y ワールドY
 */
void Renderer::moveUnitToPosition(float x, float y) {
  if (units_.empty() || !unitRenderer_ || !movementUseCase_) {
    return;
  }

  // ユーザーがユニットを直接タップしているかチェック
  std::shared_ptr<UnitEntity> tappedUnit = nullptr;
  float closestDistance = std::numeric_limits<float>::max();

  // ユニットのタップ判定用のヒットボックスサイズ
  const float UNIT_HITBOX_SIZE = 0.25f;

  for (const auto &unit : units_) {
    auto position = unit->getPosition();
    float unitX = position.getX();
    float unitY = position.getY();

    // ユニットとタッチ位置の距離を計算
    float dx = unitX - x;
    float dy = unitY - y;
    float distance = std::sqrt(dx * dx + dy * dy);

    // ヒットボックス内にタッチがあれば、そのユニットが選択されたと判定
    if (distance < UNIT_HITBOX_SIZE && distance < closestDistance) {
      closestDistance = distance;
      tappedUnit = unit;
    }
  }

  // タッチ処理の優先順位：
  // 1. ユニットをタップした場合は、そのユニットを移動
  // 2. 空の場所をタップした場合は、Unit1（RedUnit）を移動
  if (tappedUnit) {
    // タップされたユニットを移動（衝突判定とユースケースを使用）
    Position targetPos(x, y);
    bool moveSuccess =
        movementUseCase_->moveUnitTo(tappedUnit->getId(), targetPos);
    if (moveSuccess) {
      aout << "Moving " << tappedUnit->getName() << " to position (" << x
           << ", " << y << ") with collision avoidance" << std::endl;
    } else {
      aout << "Failed to move " << tappedUnit->getName() << " to position ("
           << x << ", " << y << ")" << std::endl;
    }
  } else {
    // 空の場所をタップした場合、プレイヤー陣営（faction ==
    // 1）の最初の生存ユニットのみを移動
    std::shared_ptr<UnitEntity> firstPlayerUnit = nullptr;
    for (const auto &unit : units_) {
      if (unit && unit->getFaction() == 1 && unit->isAlive()) {
        firstPlayerUnit = unit;
        break;
      }
    }

    if (firstPlayerUnit) {
      Position targetPos(x, y);
      bool moveSuccess =
          movementUseCase_->moveUnitTo(firstPlayerUnit->getId(), targetPos);
      if (moveSuccess) {
        aout << "Moving " << firstPlayerUnit->getName()
             << " to empty space at (" << x << ", " << y
             << ") with collision avoidance" << std::endl;
      } else {
        aout << "Failed to move " << firstPlayerUnit->getName()
             << " to empty space at (" << x << ", " << y << ")" << std::endl;
      }
    }
  }
}

std::shared_ptr<UnitEntity> Renderer::findUnitAtPosition(float worldX,
                                                         float worldY) const {
  aout << "UNIT_SEARCH: Searching for unit at world position (" << worldX
       << ", " << worldY << ")" << std::endl;
  aout << "UNIT_SEARCH: Total units to check: " << units_.size() << std::endl;

  std::shared_ptr<UnitEntity> closestUnit = nullptr;
  float closestDistance = std::numeric_limits<float>::max();

  for (size_t i = 0; i < units_.size(); ++i) {
    const auto &unit = units_[i];
    if (!unit || !unit->isAlive()) {
      aout << "UNIT_SEARCH: Unit[" << i << "] is null or dead, skipping"
           << std::endl;
      continue; // 死亡ユニットは無視
    }

    // ユニットとタッチ位置の距離を計算
    Position unitPos = unit->getPosition();
    float dx = worldX - unitPos.getX();
    float dy = worldY - unitPos.getY();
    float distance = std::sqrt(dx * dx + dy * dy);

    // ユニットの衝突半径内にタッチ位置があるかチェック
    float collisionRadius = unit->getStats().getCollisionRadius();

    aout << "UNIT_SEARCH: Unit[" << i << "] " << unit->getName() << " at ("
         << unitPos.getX() << ", " << unitPos.getY()
         << "), distance=" << distance << ", radius=" << collisionRadius
         << std::endl;

    if (distance <= collisionRadius) {
      aout << "UNIT_SEARCH: Unit[" << i << "] is within collision radius!"
           << std::endl;
      if (distance < closestDistance) {
        closestDistance = distance;
        closestUnit = unit;
        aout << "UNIT_SEARCH: Unit[" << i << "] is now the closest unit"
             << std::endl;
      }
    }
  }

  if (closestUnit) {
    aout << "UNIT_SEARCH: Found unit: " << closestUnit->getName()
         << " (distance: " << closestDistance << ")" << std::endl;
  } else {
    aout << "UNIT_SEARCH: No unit found at position (" << worldX << ", "
         << worldY << ")" << std::endl;
  }

  return closestUnit;
}

void Renderer::notifyUnitSelectedToAndroid(int unitId) {
  // グローバル変数を直接設定する（UnitStatusJNI.cppで定義されている）
  extern int g_selectedUnitId;
  extern int g_persistSelectedUnitId;
  g_selectedUnitId = unitId;
  g_persistSelectedUnitId = unitId;
  aout << "Notified Android of unit selection: " << unitId << std::endl;
}

// 単純なアクセサ実装（借用ポインタを返す）
UnitRenderer *Renderer::getUnitRenderer() const { return unitRenderer_.get(); }

void Renderer::resetGameToInitialState() {
  aout << "RESET: Starting game reset to initial state..." << std::endl;

  // 1. カメラを初期状態にリセット
  cameraOffsetX_ = 0.0f;
  cameraOffsetY_ = 0.0f;
  cameraZoom_ = 1.0f;

  // カメラ制御ユースケースでカメラをリセット
  if (cameraControlUseCase_) {
    cameraControlUseCase_->resetCamera();
  }

  aout << "RESET: Camera reset to initial position (0, 0) with zoom 1.0"
       << std::endl;

  // 2. 全ユニットのHPと状態を初期状態にリセット
  for (auto &unit : units_) {
    if (unit) {
      unit->resetToInitialState();
      aout << "RESET: Unit " << unit->getName()
           << " reset to initial state (HP: " << unit->getStats().getCurrentHp()
           << ")" << std::endl;
    }
  }

  // 3. 全ユニットの位置を初期位置にリセット
  if (unitRenderer_) {
    unitRenderer_->resetAllUnitsToInitialPositions();
    aout << "RESET: All unit positions reset to initial state" << std::endl;
  }

  // 4. プロジェクション行列の再計算をトリガー
  shaderNeedsNewProjectionMatrix_ = true;

  aout << "RESET: Game reset to initial state completed!" << std::endl;
}

/**
 * タッチイベントを処理します（TouchInputHandlerからのコールバック）。
 * タッチの種類に応じて適切なユースケースに振り分けます。
 */
void Renderer::handleTouchEvent(const TouchEvent &event) {
  aout << "RENDERER_DEBUG: TouchEvent received - type: "
       << static_cast<int>(event.type) << ", pos: (" << event.x << ", "
       << event.y << "), scale: " << event.scale << std::endl;

  switch (event.type) {
  case TouchInputType::SHORT_TAP: {
    // ショートタップ：ユニット移動を実行
    aout << "Short tap detected at screen (" << event.x << ", " << event.y
         << ")" << std::endl;

    // HUDボタンのチェック（従来の機能を維持）
    bool handledByHud = false;
    if (event.x >= btnUp_.x && event.x <= btnUp_.x + btnUp_.w &&
        event.y >= btnUp_.y && event.y <= btnUp_.y + btnUp_.h) {
      if (cameraControlUseCase_) {
        cameraControlUseCase_->panCameraBy(0.0f, 0.5f);
      }
      handledByHud = true;
    } else if (event.x >= btnDown_.x && event.x <= btnDown_.x + btnDown_.w &&
               event.y >= btnDown_.y && event.y <= btnDown_.y + btnDown_.h) {
      if (cameraControlUseCase_) {
        cameraControlUseCase_->panCameraBy(0.0f, -0.5f);
      }
      handledByHud = true;
    } else if (event.x >= btnLeft_.x && event.x <= btnLeft_.x + btnLeft_.w &&
               event.y >= btnLeft_.y && event.y <= btnLeft_.y + btnLeft_.h) {
      if (cameraControlUseCase_) {
        cameraControlUseCase_->panCameraBy(-0.5f, 0.0f);
      }
      handledByHud = true;
    } else if (event.x >= btnRight_.x && event.x <= btnRight_.x + btnRight_.w &&
               event.y >= btnRight_.y && event.y <= btnRight_.y + btnRight_.h) {
      if (cameraControlUseCase_) {
        cameraControlUseCase_->panCameraBy(0.5f, 0.0f);
      }
      handledByHud = true;
    }

    if (!handledByHud) {
      aout << "DEBUG_TOUCH: Processing non-HUD touch at screen (" << event.x
           << ", " << event.y << ")" << std::endl;

      // スクリーン座標をワールド座標に変換
      float worldX, worldY;
      screenToWorldCoordinates(event.x, event.y, worldX, worldY);

      aout << "DEBUG_TOUCH: Converted to world coordinates (" << worldX << ", "
           << worldY << ")" << std::endl;

      // まずタッチした位置にユニットがあるかチェック
      std::shared_ptr<UnitEntity> touchedUnit =
          findUnitAtPosition(worldX, worldY);

      if (touchedUnit) {
        // ユニットがタッチされた場合：ステータス表示（移動制限に関係なく常に実行）
        aout << "SHORT_TAP: Unit touched - showing status for "
             << touchedUnit->getName() << " (ID: " << touchedUnit->getId()
             << ")" << std::endl;

        // Android側にユニット選択を通知
        aout << "DEBUG_TOUCH: Calling notifyUnitSelectedToAndroid with ID "
             << touchedUnit->getId() << std::endl;
        notifyUnitSelectedToAndroid(touchedUnit->getId());
        aout << "DEBUG_TOUCH: notifyUnitSelectedToAndroid completed"
             << std::endl;

      } else {
        // 空の場所をタッチした場合：ユニット移動（移動制限チェックあり）
        aout << "DEBUG_TOUCH: No unit found at touch position" << std::endl;
        if (movementUseCase_ && movementUseCase_->isMovementEnabled()) {
          aout << "SHORT_TAP: Empty space touched - moving unit to world "
                  "position ("
               << worldX << ", " << worldY << ")" << std::endl;
          moveUnitToPosition(worldX, worldY);
        } else {
          aout << "SHORT_TAP: Empty space touched but unit movement is "
                  "disabled during camera operations"
               << std::endl;
        }
      }
    } else {
      aout << "DEBUG_TOUCH: Touch handled by HUD" << std::endl;
    }
    break;
  }

  case TouchInputType::LONG_TAP: {
    // ロングタップ：カメラパン（カメラを指定位置に移動）
    aout << "Long tap detected at screen (" << event.x << ", " << event.y << ")"
         << std::endl;

    // スクリーン座標をワールド座標に変換（タッチした位置）
    float touchWorldX, touchWorldY;
    screenToWorldCoordinates(event.x, event.y, touchWorldX, touchWorldY);

    // 現在のカメラ中心位置をワールド座標で取得
    // 画面中央 (width_/2, height_/2) をワールド座標に変換
    float currentCameraWorldX, currentCameraWorldY;
    screenToWorldCoordinates(width_ / 2.0f, height_ / 2.0f, currentCameraWorldX,
                             currentCameraWorldY);

    // 座標変換の検証用ログ
    aout << "COORDINATE_TEST: Screen center(" << width_ / 2.0f << ", "
         << height_ / 2.0f << ") -> World(" << currentCameraWorldX << ", "
         << currentCameraWorldY << ")" << std::endl;
    aout << "COORDINATE_TEST: Touch screen(" << event.x << ", " << event.y
         << ") -> World(" << touchWorldX << ", " << touchWorldY << ")"
         << std::endl;
    aout << "COORDINATE_TEST: Camera state - Offset(" << cameraOffsetX_ << ", "
         << cameraOffsetY_ << ") Zoom(" << cameraZoom_ << ")" << std::endl;

    // 現在のカメラ位置からタッチした位置への移動ベクトルを計算
    float moveVectorX = touchWorldX - currentCameraWorldX;
    float moveVectorY = touchWorldY - currentCameraWorldY;

    aout << "LONG_TAP DEBUG: Current camera center (" << currentCameraWorldX
         << ", " << currentCameraWorldY << ") -> Touch world pos ("
         << touchWorldX << ", " << touchWorldY << ") -> Move vector ("
         << moveVectorX << ", " << moveVectorY << ")" << std::endl;

    // スムーズカメラ移動：ターゲット位置を設定してスムーズに移動
    // 即座に移動するのではなく、cameraTarget_を設定してスムーズカメラシステムに任せる
    cameraTargetX_ = cameraOffsetX_ + moveVectorX;
    cameraTargetY_ = cameraOffsetY_ + moveVectorY;

    aout << "Smooth camera movement: target set to (" << cameraTargetX_ << ", "
         << cameraTargetY_ << ")" << std::endl;

    // ロングタップ中はユニット移動を無効化
    if (movementUseCase_) {
      movementUseCase_->setMovementEnabled(false, "Long tap camera pan");
    }
    break;
  }

  case TouchInputType::LONG_TAP_END: {
    // ロングタップ終了：ユニット移動を再有効化
    aout << "Long tap ended - re-enabling unit movement" << std::endl;
    if (movementUseCase_) {
      movementUseCase_->setMovementEnabled(true, "Long tap camera pan ended");
    }
    break;
  }

  case TouchInputType::PINCH_ZOOM: {
    // ピンチズーム：カメラズーム
    aout << "PINCH_DEBUG: event.scale=" << event.scale
         << " centerX=" << event.centerX << " centerY=" << event.centerY
         << std::endl;

    if (std::abs(event.scale - 1.0f) < 0.01f) {
      // ピンチ開始 (scale が 1.0 に近い場合) - ユニット移動を無効化
      aout << "Pinch started - disabling unit movement" << std::endl;
      if (movementUseCase_) {
        movementUseCase_->setMovementEnabled(false, "Pinch zoom started");
      }
    } else {
      // 通常のピンチズーム処理
      aout << "Pinch zoom detected, scale: " << event.scale << " center: ("
           << event.centerX << ", " << event.centerY << ")" << std::endl;

      // カメラズームを実行前に現在位置を同期
      if (cameraControlUseCase_) {
        cameraControlUseCase_->updateCurrentPosition(cameraOffsetX_,
                                                     cameraOffsetY_);
        cameraControlUseCase_->zoomCamera(event.scale, event.centerX,
                                          event.centerY);
      }
    }
    break;
  }

  case TouchInputType::PINCH_END: {
    // ピンチ終了：ユニット移動を再有効化
    aout << "Pinch ended - re-enabling unit movement" << std::endl;
    if (movementUseCase_) {
      movementUseCase_->setMovementEnabled(true, "Pinch zoom ended");
    }
    break;
  }

  default:
    aout << "Unhandled touch event type" << std::endl;
    break;
  }
}

/**
 * カメラ状態の変更を反映します（CameraControlUseCaseからのコールバック）。
 */
void Renderer::updateCameraFromState(const CameraState &newState) {
  bool zoomChanged = (cameraZoom_ != newState.zoomLevel);

  cameraOffsetX_ = newState.offsetX;
  cameraOffsetY_ = newState.offsetY;
  cameraZoom_ = newState.zoomLevel;

  // スムーズカメラのターゲットも同時に更新
  cameraTargetX_ = newState.offsetX;
  cameraTargetY_ = newState.offsetY;

  // ズームレベルが変更された場合はプロジェクション行列を再生成
  if (zoomChanged) {
    shaderNeedsNewProjectionMatrix_ = true;
  }

  aout << "Camera updated: offset(" << cameraOffsetX_ << ", " << cameraOffsetY_
       << ") target(" << cameraTargetX_ << ", " << cameraTargetY_ << ") zoom("
       << cameraZoom_ << ")" << std::endl;
}