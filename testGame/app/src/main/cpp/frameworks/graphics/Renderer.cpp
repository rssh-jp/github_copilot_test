#include "Renderer.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <android/imagedecoder.h>

#include "android/AndroidOut.h"
#include "Shader.h"
#include "utils/Utility.h"
#include "TextureAsset.h"
#include "Model.h"

// JNI関数の前方宣言
extern "C" void setRendererReference(Renderer* renderer);

//! executes glGetString and outputs the result to logcat
#define PRINT_GL_STRING(s) {aout << #s": "<< glGetString(s) << std::endl;}

/*!
 * @brief if glGetString returns a space separated list of elements, prints each one on a new line
 *
 * This works by creating an istringstream of the input c-style string. Then that is used to create
 * a vector -- each element of the vector is a new element in the input string. Finally a foreach
 * loop consumes this and outputs it to logcat using @a aout
 */
#define PRINT_GL_STRING_AS_LIST(s) { \
std::istringstream extensionStream((const char *) glGetString(s));\
std::vector<std::string> extensionList(\
        std::istream_iterator<std::string>{extensionStream},\
        std::istream_iterator<std::string>());\
aout << #s":\n";\
for (auto& extension: extensionList) {\
    aout << extension << "\n";\
}\
aout << std::endl;\
}

//! Color for cornflower blue. Can be sent directly to glClearColor
#define CORNFLOWER_BLUE 100 / 255.f, 149 / 255.f, 237 / 255.f, 1

// Vertex shader, you'd typically load this from assets
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
        
        // ユニット2と3がお互いに向かって移動するように設定
        if (units_.size() >= 3) {
            auto& unit2 = units_[1];
            auto& unit3 = units_[2];
            
            // ユニット2と3が生きている場合のみ自動戦闘処理を実行
            if (unit2->isAlive() && unit3->isAlive()) {
                // ユニット間のベクトルを計算
                auto pos2 = unit2->getPosition();
                auto pos3 = unit3->getPosition();
                float dx = pos3.getX() - pos2.getX();
                float dy = pos3.getY() - pos2.getY();
                float distance = std::sqrt(dx * dx + dy * dy);
                
                // 両方のユニットがすでに戦闘中の場合は攻撃を継続
                if (unit2->getState() == UnitState::COMBAT && unit3->getState() == UnitState::COMBAT) {
                    // 距離が離れすぎた場合は戦闘解除
                    const float COLLISION_RADIUS = 0.1f; // UnitEntityの衝突半径
                    if (distance > COLLISION_RADIUS * 2.5f) {
                        unit2->setState(UnitState::IDLE);
                        unit3->setState(UnitState::IDLE);
                        aout << "Units moved too far apart, ending combat" << std::endl;
                    } else {
                        // どちらかが死んだら戦闘終了
                        if (!unit2->isAlive() || !unit3->isAlive()) {
                            unit2->setState(UnitState::IDLE);
                            unit3->setState(UnitState::IDLE);
                            aout << "Combat ended - one unit defeated" << std::endl;
                        } else {
                            // 戦闘継続 - 攻撃処理を実行
                            
                            // TODO: UnitEntityには攻撃メソッドがないため一時的にコメントアウト
                            // ユニット2がユニット3を攻撃
                            // if (unit2->canAttack()) {
                            //     unit2->attack(unit3);
                            // }
                            // ユニット3がユニット2を攻撃
                            // if (unit3->canAttack()) {
                            //     unit3->attack(unit2);
                            // }
                        }
                    }
                    return;
                }
                
                // 適切な戦闘距離（衝突半径の2倍＋少し余裕）
                const float COLLISION_RADIUS = 0.1f; // UnitEntityの衝突半径
                float combatDistance = COLLISION_RADIUS * 2.0f + 0.05f;
                
                // 距離のログを出力（デバッグ用）
                aout << "Current distance between units: " << distance << ", combat distance: " << combatDistance << std::endl;
                
                // 現在の距離が衝突判定距離より小さい場合、両方のユニットを戦闘状態にして停止させる
                if (distance <= COLLISION_RADIUS * 2.0f) {
                    aout << "Units are colliding at distance " << distance << ", setting combat mode" << std::endl;
                    
                    // 方向ベクトルを計算（ゼロ除算防止）
                    float dirX = 0.0f;
                    float dirY = 0.0f;
                    if (distance > 0.001f) {
                        dirX = dx / distance;
                        dirY = dy / distance;
                    }
                    
                    // 両方のユニットの位置を明確に設定して、衝突を確実にする
                    float halfDistance = COLLISION_RADIUS * 0.9f; // 少し近づける
                    
                    // ユニット2とユニット3の位置は変更しない（固定位置を維持）
                    // float midX = (unit2->getX() + unit3->getX()) / 2.0f;
                    // float midY = (unit2->getY() + unit3->getY()) / 2.0f;
                    
                    // unit2->setPosition(midX - dirX * halfDistance, midY - dirY * halfDistance);
                    // unit3->setPosition(midX + dirX * halfDistance, midY + dirY * halfDistance);
                    
                    // 戦闘状態を設定（移動停止）
                    unit2->setState(UnitState::COMBAT);
                    unit3->setState(UnitState::COMBAT);
                    
                    // 目標位置は設定しない（固定位置を維持）
                    // unit2->setTargetPosition(unit2->getX(), unit2->getY());
                    // unit3->setTargetPosition(unit3->getX(), unit3->getY());
                } 
                // まだ戦闘中でなく、距離が十分ではない場合は移動を続ける
                else if (unit2->getState() != UnitState::COMBAT && unit3->getState() != UnitState::COMBAT) {
                    if (distance > 0.001f) { // ゼロ除算防止
                        // 方向ベクトルを正規化
                        float dirX = dx / distance;
                        float dirY = dy / distance;
                        
                        // 衝突判定の半径を考慮して、ちょうど良い距離で止まるように目標位置を設定
                        float safeDistance = combatDistance;
                        
                        // 距離によって異なる処理を行う
                        if (distance > combatDistance * 1.2f) {
                            // ユニット2とユニット3は移動させない（固定位置を維持）
                            // float targetX2 = unit3->getX();
                            // float targetY2 = unit3->getY();
                            // unit2->setTargetPosition(targetX2, targetY2);
                            
                            // float targetX3 = unit2->getX();
                            // float targetY3 = unit2->getY();
                            // unit3->setTargetPosition(targetX3, targetY3);
                            
                            aout << "Units moving directly towards each other, distance: " << distance << std::endl;
                        } else {
                            // 距離が適切になったら戦闘状態に設定し、位置を固定
                            
                            // ユニット2とユニット3の位置は変更しない（固定位置を維持）
                            // float halfDistance = combatDistance / 2.0f;
                            // float midX = (unit2->getX() + unit3->getX()) / 2.0f;
                            // float midY = (unit2->getY() + unit3->getY()) / 2.0f;
                            
                            // 中心点から適切な距離に配置
                            // unit2->setPosition(midX - dirX * halfDistance, midY - dirY * halfDistance);
                            // unit3->setPosition(midX + dirX * halfDistance, midY + dirY * halfDistance);
                            
                            // 戦闘状態に設定
                            unit2->setState(UnitState::COMBAT);
                            unit3->setState(UnitState::COMBAT);
                            
                            // 目標位置は設定しない（固定位置を維持）
                            // unit2->setTargetPosition(unit2->getX(), unit2->getY());
                            // unit3->setTargetPosition(unit3->getX(), unit3->getY());
                            
                            aout << "Units reached combat distance, stopping movement and adjusting positions" << std::endl;
                        }
                    } else {
                        // ほぼ同じ位置にいる場合は適切な距離に再配置
                        // TODO: UnitEntityには直接位置設定するメソッドがないため一時的にコメントアウト
                        // unit2->setPosition(unit2->getX() - 0.4f, unit2->getY());
                        // unit3->setPosition(unit3->getX() + 0.4f, unit3->getY());
                        
                        // 戦闘状態に設定
                        unit2->setState(UnitState::COMBAT);
                        unit3->setState(UnitState::COMBAT);
                    }
                }
            }
        }
        
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

    PRINT_GL_STRING(GL_VENDOR);
    PRINT_GL_STRING(GL_RENDERER);
    PRINT_GL_STRING(GL_VERSION);
    PRINT_GL_STRING_AS_LIST(GL_EXTENSIONS);

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
    
    // テスト用にいくつかのユニットを作成（異なる色と移動速度、戦闘パラメータを設定）
    // UnitEntityとUnitStatsを使用してユニットを作成
    auto unit1 = std::make_shared<UnitEntity>(1, "RedUnit", 
                                             Position(0.0f, 2.0f), 
                                             UnitStats(150, 150, 8, 1.0f, 0.8f));  // 高火力・高HP・赤ユニット
    
    auto unit2 = std::make_shared<UnitEntity>(2, "BlueUnit", 
                                             Position(-1.0f, 0.0f), 
                                             UnitStats(100, 100, 5, 0.6f, 0.4f));  // 中火力・高防御・青ユニット
    
    auto unit3 = std::make_shared<UnitEntity>(3, "GreenUnit", 
                                             Position(1.0f, 0.0f), 
                                             UnitStats(80, 80, 3, 0.5f, 1.2f));   // 低火力・高速・緑ユニット
    
    // ユニットをリストに追加
    units_.push_back(unit1);
    units_.push_back(unit2);
    units_.push_back(unit3);
    
    // ユニットを鮮やかな色で登録（背景との対比を明確に）
    unitRenderer_->registerUnitWithColor(unit1, 1.0f, 0.3f, 0.3f); // 明るい赤
    unitRenderer_->registerUnitWithColor(unit2, 0.3f, 0.3f, 1.0f); // 明るい青
    unitRenderer_->registerUnitWithColor(unit3, 0.3f, 1.0f, 0.3f); // 明るい緑
    
    // ユニット2と3は初期位置に固定（移動しない）
    // unit2->setTargetPosition(0.0f, 0.0f);   // ユニット2は右（中央）に向かって移動
    // unit3->setTargetPosition(0.0f, 0.0f);   // ユニット3は左（中央）に向かって移動
    
    // 現在の距離を計算
    auto pos2 = unit2->getPosition();
    auto pos3 = unit3->getPosition();
    float dx = pos3.getX() - pos2.getX();
    float dy = pos3.getY() - pos2.getY();
    float distance = std::sqrt(dx * dx + dy * dy);
    
    // ちょうど良い戦闘距離を計算 (衝突半径×2 + 小さな余裕)
    const float COLLISION_RADIUS = 0.1f; // UnitEntityの衝突半径
    float combatDistance = COLLISION_RADIUS * 2.0f + 0.01f;
    
    // ログ出力で確認
    aout << "Initial distance between units: " << distance << std::endl;
    aout << "Combat distance: " << combatDistance << std::endl;
    aout << "Collision radius: " << COLLISION_RADIUS << std::endl;
    
    // 正規化した方向ベクトル
    float dirX = dx / distance;
    float dirY = dy / distance;
    
    // ユニット2とユニット3は移動させない（初期位置に固定）
    // unit2->setTargetPosition(unit3->getX() - dirX * (Unit::getCollisionRadius() * 1.8f), unit3->getY() - dirY * (Unit::getCollisionRadius() * 1.8f));
    // unit3->setTargetPosition(unit2->getX() + dirX * (Unit::getCollisionRadius() * 1.8f), unit2->getY() + dirY * (Unit::getCollisionRadius() * 1.8f));
    
    aout << "Units initialized at fixed positions (distance: " << distance << ")" << std::endl;
    
    aout << "Created " << units_.size() << " units" << std::endl;
    aout << "Unit2 and Unit3 are fixed at their initial positions" << std::endl;
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
    if (units_.empty() || !unitRenderer_) {
        return;
    }
    
    // ユーザーがユニットを直接タップしているかチェック
    std::shared_ptr<UnitEntity> tappedUnit = nullptr;
    float closestDistance = std::numeric_limits<float>::max();
    
    // ユニットのタップ判定用のヒットボックスサイズ
    // ユニットサイズを0.2に変更したので、それに合わせて小さくする
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
    
    // ユニット1（RedUnit）のみをタッチで移動させる
    if (!units_.empty()) {
        auto unit1 = units_[0];  // ユニット1は最初の要素
        if (unit1->getName() == "RedUnit") {
            // ユニット1をタッチした位置に移動
            Position targetPos(x, y);
            unit1->setTargetPosition(targetPos);
            aout << "Moving Unit1 (RedUnit) to position (" << x << ", " << y << ")" << std::endl;
        }
    }
}

UnitRenderer* Renderer::getUnitRenderer() const {
    return unitRenderer_.get();
}