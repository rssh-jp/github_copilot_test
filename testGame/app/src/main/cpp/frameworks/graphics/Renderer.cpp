#include "Renderer.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <android/imagedecoder.h>
#include <chrono>
#include <limits>

#include "android/AndroidOut.h"
#include "Shader.h"
#include "utils/Utility.h"
#include "../third_party/json.hpp"
#include <android/asset_manager.h>
#include "TextureAsset.h"
#include "Model.h"

// JNI関数の前方宣言
extern "C" void setRendererReference(Renderer* renderer);
 
static const char *vertex = R"vertex(#version 300 es
// 入力頂点属性 - 明示的なロケーションを指定
layout(location = 0) in vec3 inPosition;  // 頂点位置
layout(location = 1) in vec2 inUV;        // テクスチャ座標

// フラグメントシェーダーへの出力
out vec2 fragUV;

// 変換行列
uniform mat4 uProjection;  // 投影行列
uniform mat4 uModel;       // モデル行列

void main() {
    // テクスチャ座標をそのまま出力（必ず使うようにする）
    fragUV = inUV;
    
    // 頂点位置を計算
    gl_Position = uProjection * uModel * vec4(inPosition, 1.0);
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
 * Half the height of the projection matrix. この値を大きくして、より広いエリアを描画可能にする
 * 5.0fに設定すると、縦方向が-5から5までの範囲で描画可能になる
 */
static constexpr float kProjectionHalfHeight = 5.0f;

/*!
 * The near plane distance for the projection matrix. Since this is an orthographic projection
 * matrix, it's convenient to have negative values for sorting (and avoiding z-fighting at 0).
 * 奥行き方向の描画範囲も拡張
 */
static constexpr float kProjectionNearPlane = -10.f;

/*!
 * The far plane distance for the projection matrix. Since this is an orthographic porjection
 * matrix, it's convenient to have the far plane equidistant from 0 as the near plane.
 * 奥行き方向の描画範囲も拡張
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

void Renderer::render() {
    // フレーム時間の計算（簡易的な実装）
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    
    // デルタタイムの上限を設定（フレームレート低下時の不具合を防ぐ）
    deltaTime = std::min(deltaTime, 0.016f); // 最大60FPS相当
    
    // ゲームロジックの更新
    if (movementUseCase_) {
        movementUseCase_->updateMovements(deltaTime);
    }
    
    if (combatUseCase_) {
        combatUseCase_->executeAutoCombat();
        combatUseCase_->removeDeadUnits();
    }

    // Check to see if the surface has changed size. This is _necessary_ to do every frame when
    // using immersive mode as you'll get no other notification that your renderable area has
    // changed.
    updateRenderArea();

    // When the renderable area changes, the projection matrix has to also be updated. This is true
    // even if you change from the sample orthographic projection matrix as your aspect ratio has
    // likely changed.
    if (shaderNeedsNewProjectionMatrix_) {
        // a placeholder projection matrix allocated on the stack. Column-major memory layout
        float projectionMatrix[16] = {0};

        // build an orthographic projection matrix for 2d rendering
        Utility::buildOrthographicMatrix(
                projectionMatrix,
                kProjectionHalfHeight,
                float(width_) / height_,
                kProjectionNearPlane,
                kProjectionFarPlane);

        // send the matrix to the shader
        // Note: the shader must be active for this to work. Since we only have one shader for this
        // demo, we can assume that it's active.
        shader_->setProjectionMatrix(projectionMatrix);

        // make sure the matrix isn't generated every frame
        shaderNeedsNewProjectionMatrix_ = false;
    }

    // すでに設定した色で背景をクリア（initRendererで設定した色）
    glClear(GL_COLOR_BUFFER_BIT);

    // 単位行列を作成してシェーダーに設定（デフォルトの変換なし）
    float identityMatrix[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    // シェーダーがアクティブか確認し、モデル行列を設定
    shader_->activate();
    shader_->setModelMatrix(identityMatrix);

    // デバッグログ
    aout << "Begin rendering frame..." << std::endl;

    // 背景モデルのレンダリング
    if (!models_.empty()) {
        aout << "Drawing " << models_.size() << " background models" << std::endl;
        for (const auto &model: models_) {
            shader_->drawModel(model);
        }
    } else {
        aout << "No background models to draw!" << std::endl;
    }
    
    // ユニットを描画（更新はスキップしてデバッグ）
    if (unitRenderer_) {
        aout << "Drawing units..." << std::endl;
        
        // 射程ベースの戦闘システム - 全ユニットペアをチェック
        for (size_t i = 0; i < units_.size(); ++i) {
            for (size_t j = i + 1; j < units_.size(); ++j) {
                auto& unit1 = units_[i];
                auto& unit2 = units_[j];
                
                // 両方のユニットが生きている場合のみチェック
                if (unit1->isAlive() && unit2->isAlive()) {
                    // ユニット間の距離を計算
                    auto pos1 = unit1->getPosition();
                    auto pos2 = unit2->getPosition();
                    float dx = pos2.getX() - pos1.getX();
                    float dy = pos2.getY() - pos1.getY();
                    float distance = std::sqrt(dx * dx + dy * dy);
                    
                    // 射程内チェック（どちらかのユニットの射程内に入っている場合）
                    bool unit1InRange = distance <= unit1->getStats().getAttackRange();
                    bool unit2InRange = distance <= unit2->getStats().getAttackRange();
                    
                    // いずれかが射程内で、適切な状態の場合は戦闘開始
                    if ((unit1InRange || unit2InRange)) {
                        // ユニット1が射程内にいて、待機状態なら戦闘状態に移行
                        // 移動状態のユニットは移動を優先し、戦闘状態に移行しない
                        if (unit1InRange && unit1->getState() == UnitState::IDLE) {
                            // TODO: UnitEntityには setCombatTarget メソッドがないため一時的にコメントアウト
                            // unit1->setCombatTarget(unit2);
                            unit1->setState(UnitState::COMBAT);
                            aout << unit1->getName() << " entering combat with " << unit2->getName() << std::endl;
                        }
                        
                        // ユニット2が射程内にいて、待機状態なら戦闘状態に移行
                        // 移動状態のユニットは移動を優先し、戦闘状態に移行しない
                        if (unit2InRange && unit2->getState() == UnitState::IDLE) {
                            // TODO: UnitEntityには setCombatTarget メソッドがないため一時的にコメントアウト
                            // unit2->setCombatTarget(unit1);
                            unit2->setState(UnitState::COMBAT);
                            aout << unit2->getName() << " entering combat with " << unit1->getName() << std::endl;
                        }
                    }
                }
            }
        }
        
        // 各ユニットを更新
        float deltaTime = 0.016f; // 仮の値: 60FPS想定で約16ms
        unitRenderer_->updateUnits(deltaTime);
        
        // すべてのユニットを描画
        unitRenderer_->render(shader_.get());
    } else {
        aout << "unitRenderer_ is null!" << std::endl;
    }
    
    aout << "Frame rendering complete" << std::endl;

    // Present the rendered image. This is an implicit glFlush.
    auto swapResult = eglSwapBuffers(display_, surface_);
    assert(swapResult == EGL_TRUE);
}

void Renderer::initRenderer() {
    // Choose your render attributes
    constexpr EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };

    // The default display is probably what you want on Android
    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    // figure out how many configs there are
    EGLint numConfigs;
    eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);

    // get the list of configurations
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);

    // Find a config we like.
    // Could likely just grab the first if we don't care about anything else in the config.
    // Otherwise hook in your own heuristic
    auto config = *std::find_if(
            supportedConfigs.get(),
            supportedConfigs.get() + numConfigs,
            [&display](const EGLConfig &config) {
                EGLint red, green, blue, depth;
                if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red)
                    && eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green)
                    && eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue)
                    && eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth)) {

                    aout << "Found config with " << red << ", " << green << ", " << blue << ", "
                         << depth << std::endl;
                    return red == 8 && green == 8 && blue == 8 && depth == 24;
                }
                return false;
            });

    aout << "Found " << numConfigs << " configs" << std::endl;
    aout << "Chose " << config << std::endl;

    // create the proper window surface
    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    EGLSurface surface = eglCreateWindowSurface(display, config, app_->window, nullptr);

    // Create a GLES 3 context
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttribs);

    // get some window metrics
    auto madeCurrent = eglMakeCurrent(display, surface, surface, context);
    assert(madeCurrent);

    display_ = display;
    surface_ = surface;
    context_ = context;

    // make width and height invalid so it gets updated the first frame in @a updateRenderArea()
    width_ = -1;
    height_ = -1;

    // Log basic GL strings (avoid undefined macro usage)
    const char* glVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* glRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    aout << "GL_VENDOR: " << (glVendor ? glVendor : "unknown") << std::endl;
    aout << "GL_RENDERER: " << (glRenderer ? glRenderer : "unknown") << std::endl;
    aout << "GL_VERSION: " << (glVersion ? glVersion : "unknown") << std::endl;

    // Print extensions using glGetStringi where available (GLES3)
    GLint numExt = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
    aout << "GL_EXTENSIONS (" << numExt << "):";
    for (GLint ei = 0; ei < numExt; ++ei) {
        const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, ei));
        if (ext) aout << " " << ext;
    }
    aout << std::endl;

    // OpenGLのエラーをクリア
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        aout << "OpenGL error before shader load: " << err << std::endl;
    }
    
    // シェーダーの読み込み時にエラーチェックを強化
    shader_ = std::unique_ptr<Shader>(
            Shader::loadShader(vertex, fragment, "inPosition", "inUV", "uProjection", "uModel"));
    
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
    // 画面いっぱいに表示するための四角形
    std::vector<Vertex> vertices = {
            Vertex(Vector3{1, 1, 0}, Vector2{1, 0}),     // 右上
            Vertex(Vector3{-1, 1, 0}, Vector2{0, 0}),    // 左上
            Vertex(Vector3{-1, -1, 0}, Vector2{0, 1}),   // 左下
            Vertex(Vector3{1, -1, 0}, Vector2{1, 1})     // 右下
    };
    std::vector<Index> indices = {
            0, 1, 2, 0, 2, 3
    };

    // 背景用に暗い青色のテクスチャを作成
    auto bgTexture = TextureAsset::createSolidColorTexture(0.1f, 0.1f, 0.3f);
    aout << "Created background texture with ID: " << bgTexture->getTextureID() << std::endl;

    // 画面全体を覆う背景モデルを作成して追加
    models_.emplace_back(vertices, indices, bgTexture);
    
    // ユニットレンダラーを初期化（背景テクスチャを再利用）
    unitRenderer_ = std::make_unique<UnitRenderer>(bgTexture);
    // デバッグ用途: 当たり判定ワイヤーフレームを常に表示
    unitRenderer_->setShowCollisionWireframes(true);
    
    // Try to load unit spawn configuration from assets/unit_spawns.json
    bool loadedFromJson = false;
    if (app_ && app_->activity && app_->activity->assetManager) {
        AAssetManager* mgr = app_->activity->assetManager;
        AAsset* asset = AAssetManager_open(mgr, "unit_spawns.json", AASSET_MODE_STREAMING);
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
                            auto& arr = it->second->arrayValue;
                            for (auto& item : arr) {
                                if (!item || !item->isObject()) continue;
                                auto& obj = item->objectValue;
                                int id = 0;
                                std::string name = "Unit";
                                float x = 0.0f, y = 0.0f;
                                int faction = 0;
                                int maxHp = 100, currentHp = 100, minAtk = 1, maxAtk = 1;
                                float moveSpeed = 1.0f, attackSpeed = 1.0f, defense = 0.0f, collisionRadius = 0.25f;

                                if (obj.find("id") != obj.end() && obj["id"]->isNumber()) id = static_cast<int>(obj["id"]->numberValue);
                                if (obj.find("name") != obj.end() && obj["name"]->isString()) name = obj["name"]->stringValue;
                                if (obj.find("x") != obj.end() && obj["x"]->isNumber()) x = static_cast<float>(obj["x"]->numberValue);
                                if (obj.find("y") != obj.end() && obj["y"]->isNumber()) y = static_cast<float>(obj["y"]->numberValue);
                                if (obj.find("faction") != obj.end() && obj["faction"]->isNumber()) faction = static_cast<int>(obj["faction"]->numberValue);

                                auto sIt = obj.find("stats");
                                if (sIt != obj.end() && sIt->second->isObject()) {
                                    auto& sObj = sIt->second->objectValue;
                                    if (sObj.find("maxHp") != sObj.end() && sObj["maxHp"]->isNumber()) maxHp = static_cast<int>(sObj["maxHp"]->numberValue);
                                    if (sObj.find("currentHp") != sObj.end() && sObj["currentHp"]->isNumber()) currentHp = static_cast<int>(sObj["currentHp"]->numberValue);
                                    if (sObj.find("minAttack") != sObj.end() && sObj["minAttack"]->isNumber()) minAtk = static_cast<int>(sObj["minAttack"]->numberValue);
                                    if (sObj.find("maxAttack") != sObj.end() && sObj["maxAttack"]->isNumber()) maxAtk = static_cast<int>(sObj["maxAttack"]->numberValue);
                                    if (sObj.find("moveSpeed") != sObj.end() && sObj["moveSpeed"]->isNumber()) moveSpeed = static_cast<float>(sObj["moveSpeed"]->numberValue);
                                    if (sObj.find("attackSpeed") != sObj.end() && sObj["attackSpeed"]->isNumber()) attackSpeed = static_cast<float>(sObj["attackSpeed"]->numberValue);
                                    if (sObj.find("defense") != sObj.end() && sObj["defense"]->isNumber()) defense = static_cast<float>(sObj["defense"]->numberValue);
                                    if (sObj.find("collisionRadius") != sObj.end() && sObj["collisionRadius"]->isNumber()) collisionRadius = static_cast<float>(sObj["collisionRadius"]->numberValue);
                                }

                                UnitStats stats(maxHp, currentHp, minAtk, maxAtk, moveSpeed, attackSpeed, defense, collisionRadius);
                                auto u = std::make_shared<UnitEntity>(id, name, Position(x, y), stats, faction);
                                units_.push_back(u);

                                if (faction == 1) unitRenderer_->registerUnitWithColor(u, 1.0f, 0.3f, 0.3f);
                                else if (faction == 2) unitRenderer_->registerUnitWithColor(u, 0.3f, 0.3f, 1.0f);
                                else unitRenderer_->registerUnitWithColor(u, 0.6f, 0.6f, 0.6f);
                            }
                            loadedFromJson = true;
                        }
                    }
                } catch (const std::exception& e) {
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
        std::array<float,3> yOffsets = {1.5f, 0.0f, -1.5f};
        int idCounter = 1;

        for (int i = 0; i < 3; ++i) {
            auto u = std::make_shared<UnitEntity>(idCounter++, "PlayerUnit" + std::to_string(i+1),
                                                  Position(-2.0f, yOffsets[i]),
                                                  UnitStats(120, 120, 6, 10, 1.0f, 0.6f, 0.5f, 0.25f),
                                                  1); // faction 1
            units_.push_back(u);
            faction1Units.push_back(u);
            unitRenderer_->registerUnitWithColor(u, 1.0f, 0.3f, 0.3f); // 赤
        }

        for (int i = 0; i < 3; ++i) {
            auto u = std::make_shared<UnitEntity>(idCounter++, "EnemyUnit" + std::to_string(i+1),
                                                  Position(2.0f, yOffsets[i]),
                                                  UnitStats(100, 100, 4, 8, 1.0f, 0.7f, 0.4f, 0.25f),
                                                  2); // faction 2
            units_.push_back(u);
            faction2Units.push_back(u);
            unitRenderer_->registerUnitWithColor(u, 0.3f, 0.3f, 1.0f); // 青
        }
    }
    
    // If we have at least 3 units, compute a sample distance between units[1] and units[2]
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
    
    // MovementField を作成（ワールド座標 -5..5 x -5..5）
    movementField_ = std::make_unique<MovementField>(-5.0f, -5.0f, 5.0f, 5.0f);
    // 視覚的にわかりやすい障害物を追加
    movementField_->addCircleObstacle(Position(-1.5f, 0.5f), 0.5f);
    movementField_->addCircleObstacle(Position(1.2f, -0.5f), 0.6f);

    // 障害物を視覚化するために簡易的な円ポリゴンを models_ に追加（線で描画されるが目立つよう色をつける）
    auto addCircleModel = [&](const Position& center, float radius, int segments, float r, float g, float b) {
        std::vector<Vertex> verts;
        std::vector<Index> inds;
        verts.reserve(segments);
        inds.reserve(segments);
        for (int i = 0; i < segments; ++i) {
            float theta = (2.0f * 3.14159265358979323846f * i) / segments;
            float x = std::cos(theta) * radius + center.getX();
            float y = std::sin(theta) * radius + center.getY();
            verts.emplace_back(Vector3{ x, y, 0.0f }, Vector2{0,0});
            inds.push_back(static_cast<Index>(i));
        }
        auto tex = TextureAsset::createSolidColorTexture(r, g, b);
        models_.emplace_back(verts, inds, tex);
    };

    addCircleModel(Position(-1.5f, 0.5f), 0.5f, 32, 1.0f, 0.3f, 0.3f); // 赤い障害物
    addCircleModel(Position(1.2f, -0.5f), 0.6f, 32, 0.3f, 0.3f, 1.0f); // 青い障害物

    // ユースケースを初期化（MovementField を注入）
    combatUseCase_ = std::make_unique<CombatUseCase>(units_);
    movementUseCase_ = std::make_unique<MovementUseCase>(units_, movementField_.get());
    
    // 戦闘イベントのコールバックを設定
    combatUseCase_->setCombatEventCallback([this](const UnitEntity& attacker, const UnitEntity& target, const CombatDomainService::CombatResult& result) {
        aout << "Combat: Unit " << attacker.getId() << " attacked Unit " << target.getId() 
             << " for " << result.damageDealt << " damage" << std::endl;
        if (result.targetKilled) {
            aout << "Unit " << target.getId() << " was killed!" << std::endl;
        }
        if (result.attackerKilled) {
            aout << "Unit " << attacker.getId() << " was killed by counter attack!" << std::endl;
        }
    });
    
    // 移動イベントのコールバックを設定
    movementUseCase_->setMovementEventCallback([this](const UnitEntity& unit, const Position& from, const Position& to) {
        aout << "Movement: Unit " << unit.getId() << " moved from (" << from.getX() << ", " << from.getY() 
             << ") to (" << to.getX() << ", " << to.getY() << ")" << std::endl;
    });
    
    movementUseCase_->setMovementFailedCallback([this](const UnitEntity& unit, const Position& target, const std::string& reason) {
        aout << "Movement Failed: Unit " << unit.getId() << " could not move to (" 
             << target.getX() << ", " << target.getY() << ") - " << reason << std::endl;
    });
}

void Renderer::handleInput() {
    // handle all queued inputs
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) {
        // no inputs yet.
        return;
    }

    // handle motion events (motionEventsCounts can be 0).
    for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
        auto &motionEvent = inputBuffer->motionEvents[i];
        auto action = motionEvent.action;

        // Find the pointer index, mask and bitshift to turn it into a readable value.
        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        aout << "Pointer(s): ";

        // get the x and y position of this event if it is not ACTION_MOVE.
        auto &pointer = motionEvent.pointers[pointerIndex];
        auto x = GameActivityPointerAxes_getX(&pointer);
        auto y = GameActivityPointerAxes_getY(&pointer);

        // determine the action type and process the event accordingly.
        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                aout << "(" << pointer.id << ", " << x << ", " << y << ") "
                     << "Pointer Down";
                {
                    // タッチされた場所にユニットを移動
                    float worldX, worldY;
                    screenToWorldCoordinates(x, y, worldX, worldY);
                    moveUnitToPosition(worldX, worldY);
                }
                break;

            case AMOTION_EVENT_ACTION_CANCEL:
                // treat the CANCEL as an UP event: doing nothing in the app, except
                // removing the pointer from the cache if pointers are locally saved.
                // code pass through on purpose.
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
                aout << "(" << pointer.id << ", " << x << ", " << y << ") "
                     << "Pointer Up";
                break;

            case AMOTION_EVENT_ACTION_MOVE:
                // There is no pointer index for ACTION_MOVE, only a snapshot of
                // all active pointers; app needs to cache previous active pointers
                // to figure out which ones are actually moved.
                for (auto index = 0; index < motionEvent.pointerCount; index++) {
                    pointer = motionEvent.pointers[index];
                    x = GameActivityPointerAxes_getX(&pointer);
                    y = GameActivityPointerAxes_getY(&pointer);
                    aout << "(" << pointer.id << ", " << x << ", " << y << ")";

                    if (index != (motionEvent.pointerCount - 1)) aout << ",";
                    aout << " ";
                }
                aout << "Pointer Move";
                break;
            default:
                aout << "Unknown MotionEvent Action: " << action;
        }
        aout << std::endl;
    }
    // clear the motion input count in this buffer for main thread to re-use.
    android_app_clear_motion_events(inputBuffer);

    // handle input key events.
    for (auto i = 0; i < inputBuffer->keyEventsCount; i++) {
        auto &keyEvent = inputBuffer->keyEvents[i];
        aout << "Key: " << keyEvent.keyCode <<" ";
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

void Renderer::screenToWorldCoordinates(float screenX, float screenY, float& worldX, float& worldY) const {
    // 画面の幅と高さに基づいて座標変換
    // 画面座標系: 左上が(0,0)、右下が(width,height)
    // ワールド座標系: 中心が(0,0)、画面範囲は変わらないが、制限なし
    
    // スケールファクターを大きくして、より広い範囲を利用可能にする
    // 投影行列のkProjectionHalfHeightと一致させる
    const float WORLD_SCALE = 5.0f;
    
    // Xの変換: [0, width] -> [-WORLD_SCALE, WORLD_SCALE]
    worldX = ((screenX / width_) * 2.0f - 1.0f) * WORLD_SCALE;
    
    // Yの変換: [0, height] -> [WORLD_SCALE, -WORLD_SCALE]（Y軸は反転）
    worldY = (1.0f - (screenY / height_) * 2.0f) * WORLD_SCALE;
    
    // アスペクト比を考慮
    float aspectRatio = static_cast<float>(width_) / height_;
    worldX *= aspectRatio;
    
    aout << "Screen (" << screenX << ", " << screenY << ") -> World (" << worldX << ", " << worldY << ")" << std::endl;
}

void Renderer::moveUnitToPosition(float x, float y) {
    if (units_.empty() || !unitRenderer_ || !movementUseCase_) {
        return;
    }
    
    // ユーザーがユニットを直接タップしているかチェック
    std::shared_ptr<UnitEntity> tappedUnit = nullptr;
    float closestDistance = std::numeric_limits<float>::max();
    
    // ユニットのタップ判定用のヒットボックスサイズ
    const float UNIT_HITBOX_SIZE = 0.25f;
    
    for (const auto& unit : units_) {
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
        bool moveSuccess = movementUseCase_->moveUnitTo(tappedUnit->getId(), targetPos);
        if (moveSuccess) {
            aout << "Moving " << tappedUnit->getName() << " to position (" << x << ", " << y << ") with collision avoidance" << std::endl;
        } else {
            aout << "Failed to move " << tappedUnit->getName() << " to position (" << x << ", " << y << ")" << std::endl;
        }
    } else {
        // 空の場所をタップした場合、プレイヤー陣営（faction == 1）の最初の生存ユニットのみを移動
        std::shared_ptr<UnitEntity> firstPlayerUnit = nullptr;
        for (const auto& unit : units_) {
            if (unit && unit->getFaction() == 1 && unit->isAlive()) {
                firstPlayerUnit = unit;
                break;
            }
        }

        if (firstPlayerUnit) {
            Position targetPos(x, y);
            bool moveSuccess = movementUseCase_->moveUnitTo(firstPlayerUnit->getId(), targetPos);
            if (moveSuccess) {
                aout << "Moving " << firstPlayerUnit->getName() << " to empty space at (" << x << ", " << y << ") with collision avoidance" << std::endl;
            } else {
                aout << "Failed to move " << firstPlayerUnit->getName() << " to empty space at (" << x << ", " << y << ")" << std::endl;
            }
        }
    }
}

UnitRenderer* Renderer::getUnitRenderer() const {
    return unitRenderer_.get();
}