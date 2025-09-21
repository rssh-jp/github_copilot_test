#include "UnitRenderer.h"
#include "android/AndroidOut.h"
#include "../domain/services/CollisionDomainService.h"
#include <cmath>

/*
 * UnitRenderer.cpp
 *
 * Responsibilities:
 * - Visualize UnitEntity instances: model, HP bars, debug wireframes (collision / attack ranges).
 * - Provide a registration API for units and lightweight per-frame update/render calls.
 *
 * Notes:
 * - Keep rendering logic here; do not move game rules into rendering code. The renderer may read
 *   domain state but must not mutate business logic.
 */
#include "android/AndroidOut.h"
#include "../domain/services/CollisionDomainService.h"
#include <cmath>

/**
 * @brief コンストラクタ
 *
 * @param spTexture ユニット描画に使うデフォルトテクスチャ（nullptr でも可）
 */
UnitRenderer::UnitRenderer(std::shared_ptr<TextureAsset> spTexture)
    : spTexture_(spTexture)
    , unitModel_(createUnitModel()) {
    
    aout << "UnitRenderer initialized with " 
         << (spTexture ? "provided texture" : "default red texture") << std::endl;
}

UnitRenderer::~UnitRenderer() {
    aout << "UnitRenderer destroyed" << std::endl;
}

/**
 * @brief 攻撃範囲表示フラグを設定します。
 *
 * デバッグ目的のビジュアルで、ユニット周囲に攻撃範囲の円を描画するか制御します。
 */
void UnitRenderer::setShowAttackRanges(bool show) {
    showAttackRanges_ = show;
}

/**
 * @brief ユニットをレンダラーに登録します。
 *
 * 登録後は render()/updateUnits() の対象となり、内部でユニットに対応するテクスチャが
 * 設定されます。
 */
void UnitRenderer::registerUnit(const std::shared_ptr<UnitEntity>& unit) {
    if (unit) {
        units_[unit->getId()] = unit;
        
        // デフォルトテクスチャを設定（赤色）
        unitTextures_[unit->getId()] = getColorTexture(1.0f, 0.0f, 0.0f);
        
        aout << "Registered unit: " << unit->getName() << " (ID: " << unit->getId() << ")" << std::endl;
    }
}

void UnitRenderer::registerUnitWithColor(const std::shared_ptr<UnitEntity>& unit, float r, float g, float b) {
    if (unit) {
        units_[unit->getId()] = unit;
        
        // 指定された色のテクスチャを設定
        unitTextures_[unit->getId()] = getColorTexture(r, g, b);
        
        aout << "Registered unit: " << unit->getName() << " (ID: " << unit->getId() 
             << ") with color (" << r << ", " << g << ", " << b << ")" << std::endl;
    }
}

void UnitRenderer::unregisterUnit(int unitId) {
    auto it = units_.find(unitId);
    if (it != units_.end()) {
        aout << "Unregistered unit: " << it->second->getName() << " (ID: " << unitId << ")" << std::endl;
        units_.erase(it);
    }
}

void UnitRenderer::clearAllUnits() {
    aout << "Clearing all units. Total units: " << units_.size() << std::endl;
    units_.clear();
    unitTextures_.clear();
}

std::shared_ptr<TextureAsset> UnitRenderer::getColorTexture(float r, float g, float b) {
    // RGB値をキーにして、すでに同じ色のテクスチャがキャッシュにあるか確認
    std::string colorKey = std::to_string(r) + "_" + std::to_string(g) + "_" + std::to_string(b);
    
    auto it = colorTextureCache_.find(colorKey);
    if (it != colorTextureCache_.end()) {
        return it->second;
    }
    
    // キャッシュにない場合は新しいテクスチャを作成
    auto texture = TextureAsset::createSolidColorTexture(r, g, b);
    colorTextureCache_[colorKey] = texture;
    
    return texture;
}

/**
 * @brief 全ユニットを描画します。
 *
 * 各ユニットの状態に応じてテクスチャや色を変え、モデル描画とHPバー描画を行います。
 * 描画順序: ユニット本体 -> HPバー -> (最後に) ワイヤーフレーム / 攻撃範囲
 */
void UnitRenderer::render(const Shader* shader, float cameraOffsetX, float cameraOffsetY) {
    for (const auto& pair : units_) {
        const auto& unitId = pair.first;
        const auto& unit = pair.second;
        
        // このユニット用のテクスチャを取得
        std::shared_ptr<TextureAsset> unitTexture = nullptr;
        
        if (!unit->isAlive()) {
            // 死亡している場合は灰色に変更
            unitTexture = getColorTexture(0.5f, 0.5f, 0.5f);
        } else if (unit->getState() == UnitState::COMBAT) {
            // 攻撃中は明るいオレンジ色に変更
            unitTexture = getColorTexture(1.0f, 0.6f, 0.2f);
            aout << unit->getName() << " is attacking - showing orange highlight" << std::endl;
        // TODO: 衝突判定機能が実装されたら有効化
        // } else if (unit->isColliding()) {
        //     // 衝突中は明るい赤色に変更
        //     unitTexture = getColorTexture(1.0f, 0.2f, 0.2f);
        } else {
        // HP状態に応じて色を変化させる（HPが低いと赤っぽく、高いと元の色に近くなる）
            float hpRatio = static_cast<float>(unit->getStats().getCurrentHp()) / unit->getStats().getMaxHp();
            
            auto textureIt = unitTextures_.find(unitId);
            if (textureIt != unitTextures_.end()) {
                // 元の色情報を取得（簡易的な実装）
                // 陣営に基づく基本色を選択
                float r = 0.3f, g = 0.3f, b = 1.0f; // デフォルト青色
                int faction = unit->getFaction();
                if (faction == 1) {
                    r = 1.0f; g = 0.3f; b = 0.3f; // 赤陣営
                } else if (faction == 2) {
                    r = 0.3f; g = 0.3f; b = 1.0f; // 青陣営
                } else if (faction == 3) {
                    r = 0.3f; g = 1.0f; b = 0.3f; // 緑陣営
                }
                
                // HPに応じて色を変化（HPが低いほど赤くなる）
                r = std::min(1.0f, r + (1.0f - hpRatio) * 0.5f);
                g = g * hpRatio;
                b = b * hpRatio;
                
                unitTexture = getColorTexture(r, g, b);
            } else {
                unitTexture = spTexture_; // デフォルトテクスチャを使用
            }
        }
        
        // 一時的にモデルのテクスチャを変更
        Model unitModel = unitModel_; // モデルをコピー
        if (unitTexture) {
            // モデルクラスにはテクスチャを変更するAPIがないため、新しいモデルを作成
            // サイズを1/4程度に小さくする
            std::vector<Vertex> vertices = {
                Vertex(Vector3{ 0.2f,  0.2f, 0}, Vector2{1, 0}), // 右上
                Vertex(Vector3{-0.2f,  0.2f, 0}, Vector2{0, 0}), // 左上
                Vertex(Vector3{-0.2f, -0.2f, 0}, Vector2{0, 1}), // 左下
                Vertex(Vector3{ 0.2f, -0.2f, 0}, Vector2{1, 1})  // 右下
            };
            std::vector<Index> indices = {0, 1, 2, 0, 2, 3};
            unitModel = Model(vertices, indices, unitTexture);
        }
        
    // 描画前にユニットの位置に基づいてモデルマトリックスを設定
    float modelMatrix[16] = {0};
        
    // 単位行列を作成
    modelMatrix[0] = 1.0f;
    modelMatrix[5] = 1.0f;
    modelMatrix[10] = 1.0f;
    modelMatrix[15] = 1.0f;
        
    // ユニットの実際の位置を設定（カメラオフセットを考慮して画面上の正しい位置にする）
    auto position = unit->getPosition();
    modelMatrix[12] = position.getX() + cameraOffsetX; // X座標
    modelMatrix[13] = position.getY() + cameraOffsetY; // Y座標
        
    // モデルマトリックスをシェーダに送信
    shader->setModelMatrix(modelMatrix);
        
        // ユニットを描画
        shader->drawModel(unitModel);
        
        // HP表示を描画（生きているユニットのみ）
        if (unit->isAlive()) {
            renderHPBar(shader, unit, cameraOffsetX, cameraOffsetY);
        }
    }

    // 全ユニットの当たり判定ワイヤーフレームを最前面に表示
    if (showCollisionWireframes_) {
        renderCollisionWireframes(shader, cameraOffsetX, cameraOffsetY);
    }

    // 攻撃範囲の表示（最前面）
    if (showAttackRanges_) {
        renderAttackRanges(shader, cameraOffsetX, cameraOffsetY);
    }
}

/**
 * @brief 各ユニットの攻撃範囲を円で描画します（デバッグ表示）。
 *
 * カメラオフセットを考慮して、ユニットのワールド位置に円を配置します。
 */
void UnitRenderer::renderAttackRanges(const Shader* shader, float cameraOffsetX, float cameraOffsetY) {
    glDepthMask(GL_FALSE); // 深度書き込みオフ

    const int segments = 48; // より滑らかな円

    for (const auto& pair : units_) {
        const auto& unit = pair.second;
        if (!unit) continue;
        if (!unit->isAlive()) continue; // 死亡ユニットの攻撃範囲は表示しない

        float range = unit->getStats().getAttackRange();
        if (range <= 0.0f) continue;

        // 円頂点を生成
        std::vector<Vertex> circleVertices;
        circleVertices.reserve(segments);
        std::vector<Index> circleIndices;
        circleIndices.reserve(segments);

        for (int i = 0; i < segments; ++i) {
            float theta = (2.0f * 3.14159265358979323846f * i) / segments;
            float x = std::cos(theta) * range;
            float y = std::sin(theta) * range;
            // テクスチャ座標は使用しないが必要なので 0,0 を設定
            circleVertices.emplace_back(Vector3{ x, y, 0.0f }, Vector2{0,0});
            circleIndices.push_back(static_cast<Index>(i));
        }

        // カラーは陣営ベースで薄い半透明（テクスチャは単色）
        int faction = unit->getFaction();
        float lr = 1.0f, lg = 1.0f, lb = 1.0f;
        if (faction == 1) { lr = 1.0f; lg = 0.4f; lb = 0.4f; }
        else if (faction == 2) { lr = 0.4f; lg = 0.4f; lb = 1.0f; }
        else if (faction == 3) { lr = 0.4f; lg = 1.0f; lb = 0.4f; }

        // 透明度を下げて目立ち過ぎないようにする（テクスチャ作成時にアルファ扱いは簡便化）
        float alphaMultiplier = 0.35f;
        auto rangeTexture = getColorTexture(lr * alphaMultiplier, lg * alphaMultiplier, lb * alphaMultiplier);
        Model circleModel(circleVertices, circleIndices, rangeTexture);

        // モデルマトリクスをユニット位置に設定（カメラオフセットを考慮）
        float modelMatrix[16] = {0};
        modelMatrix[0] = 1.0f; modelMatrix[5] = 1.0f; modelMatrix[10] = 1.0f; modelMatrix[15] = 1.0f;
        auto position = unit->getPosition();
        modelMatrix[12] = position.getX() + cameraOffsetX;
        modelMatrix[13] = position.getY() + cameraOffsetY;

        shader->setModelMatrix(modelMatrix);
        // 描画は塗りつぶしにせずラインループで表示すると見やすい
        shader->drawModelWithMode(circleModel, GL_LINE_LOOP);
    }

    glDepthMask(GL_TRUE);
}

void UnitRenderer::setShowCollisionWireframes(bool show) {
    showCollisionWireframes_ = show;
}

/**
 * @brief 各ユニットの当たり判定（collision radius）をワイヤーフレームで描画します。
 *
 * デバッグ用途。描画は GL_LINE_LOOP で行われ、深度書き込みをオフにして最前面に表示します。
 */
void UnitRenderer::renderCollisionWireframes(const Shader* shader, float cameraOffsetX, float cameraOffsetY) {
    // 深度を最前面に表示するために深度書き込みを無効化
    glDepthMask(GL_FALSE); // 深度書き込みオフ

    const int segments = 32;

    for (const auto& pair : units_) {
        const auto& unit = pair.second;
        if (!unit) continue;

        float radius = unit->getStats().getCollisionRadius();

        // 円頂点を生成
        std::vector<Vertex> circleVertices;
        circleVertices.reserve(segments);
        std::vector<Index> circleIndices;
        circleIndices.reserve(segments);

        for (int i = 0; i < segments; ++i) {
            float theta = (2.0f * 3.14159265358979323846f * i) / segments;
            float x = std::cos(theta) * radius;
            float y = std::sin(theta) * radius;
            circleVertices.emplace_back(Vector3{ x, y, 0.0f }, Vector2{0,0});
            circleIndices.push_back(static_cast<Index>(i));
        }

        // ワイヤーフレームの色はユニットの陣営色をベースに暗めにする
        int faction = unit->getFaction();
        float lr = 0.2f, lg = 0.2f, lb = 0.2f; // デフォルトは濃い灰色
        if (faction == 1) {
            lr = 0.8f; lg = 0.15f; lb = 0.15f; // 濃い赤
        } else if (faction == 2) {
            lr = 0.15f; lg = 0.15f; lb = 0.8f; // 濃い青
        } else if (faction == 3) {
            lr = 0.15f; lg = 0.8f; lb = 0.15f; // 濃い緑
        }
        // ワイヤーフレームはさらに暗くして目立ちすぎないようにする
        lr *= 0.75f; lg *= 0.75f; lb *= 0.75f;
        auto lineTexture = getColorTexture(lr, lg, lb);
        Model circleModel(circleVertices, circleIndices, lineTexture);

    // モデルマトリクスをユニット位置に設定（カメラオフセットを考慮）
    float modelMatrix[16] = {0};
    modelMatrix[0] = 1.0f; modelMatrix[5] = 1.0f; modelMatrix[10] = 1.0f; modelMatrix[15] = 1.0f;
    auto position = unit->getPosition();
    modelMatrix[12] = position.getX() + cameraOffsetX;
    modelMatrix[13] = position.getY() + cameraOffsetY;

        shader->setModelMatrix(modelMatrix);
        shader->drawModelWithMode(circleModel, GL_LINE_LOOP);
    }

    glDepthMask(GL_TRUE); // 深度書き込みを元に戻す
}

/**
 * @brief ユニットの表示ロジックの更新を行います。
 *
 * 現状は移動更新（updateMovement）を呼び出し、将来的に衝突回避や攻撃の視覚的更新をここで
 * 行う設計です。ビジネスロジック（ダメージ計算等）は UseCase 層で扱うべきです。
 */
void UnitRenderer::updateUnits(float deltaTime) {
    // すべてのユニットのリストを作成
    std::vector<std::shared_ptr<UnitEntity>> unitList;
    unitList.reserve(units_.size());
    
    for (const auto& pair : units_) {
        unitList.push_back(pair.second);
    }
    
    // 衝突回避の強さ（0.0～1.0）
    // めり込み防止のために衝突回避の強さを上げる
    const float AVOIDANCE_STRENGTH = 0.5f;
    
    // 各ユニットの衝突回避処理（このステップで敵と接触したユニットが攻撃を行う）
    for (auto& pair : units_) {
        auto& unit = pair.second;
        // TODO: UnitEntityには avoidCollisions メソッドがないため一時的にコメントアウト
        // 将来的にはCollisionUseCaseで実装予定
        // unit->avoidCollisions(unitList, AVOIDANCE_STRENGTH);
    }
    
    // 攻撃可能なユニットは敵を攻撃（戦闘中のユニットを優先）
    for (auto& pair : units_) {
        auto& unit = pair.second;
        
        // ユニットが生きている場合のみ更新
        if (unit->isAlive()) {
            // TODO: 攻撃機能はUseCaseレイヤーに移動予定
            // 現在は基本的な状態管理のみ実装
            /*
            // 戦闘中か衝突中の場合は優先的に攻撃
            if (unit->getState() == UnitState::COMBAT || unit->isColliding()) {
                auto target = unit->findTargetToAttack(unitList);
                if (target) {
                    int damage = unit->attack(target);
                    if (damage > 0) {
                        aout << unit->getName() << " attacks " << target->getName() 
                             << " for " << damage << " damage!" << std::endl;
                    }
                }
            }
            // それ以外の場合も攻撃可能なら近くの敵を攻撃
            else {
                auto target = unit->findTargetToAttack(unitList);
                if (target) {
                    unit->attack(target);
                }
            }
            */
        }
    }
    
    // ユニットの更新（移動や状態更新）
    // 全ユニットのリストを作成
    std::vector<std::shared_ptr<UnitEntity>> allUnits;
    for (const auto& pair : units_) {
        allUnits.push_back(pair.second);
    }
    
    // 衝突予測機能付きでユニットを更新
    for (auto& pair : units_) {
        // 基本的な移動更新処理を実行
        pair.second->updateMovement(deltaTime);
        
        // TODO: より複雑な更新処理（攻撃、衝突回避等）は将来的にUseCaseレイヤーで実装予定
        // pair.second->update(deltaTime, allUnits);
    }
}

/**
 * @brief 指定ユニットのHPバーを描画します。
 *
 * バーはユニットの上に固定され、HP割合に応じて色と幅が変化します。
 */
void UnitRenderer::renderHPBar(const Shader* shader, const std::shared_ptr<UnitEntity>& unit, float cameraOffsetX, float cameraOffsetY) {
    // HP表示用のバーを描画する
    // バーの設定
    const float barWidth = 0.3f; // バーの幅
    const float barHeight = 0.05f; // バーの高さ
    const float barY = 0.25f; // ユニットの上端からの距離
    
    // HPの割合を計算
    float hpRatio = static_cast<float>(unit->getStats().getCurrentHp()) / unit->getStats().getMaxHp();
    hpRatio = std::max(0.0f, std::min(1.0f, hpRatio)); // 0.0～1.0に制限
    
    // HPバーの背景（灰色）
    {
        // バーの背景用のモデル
        std::vector<Vertex> vertices = {
            Vertex(Vector3{ barWidth/2,  barY + barHeight, 0.1f}, Vector2{1, 0}), // 右上
            Vertex(Vector3{-barWidth/2,  barY + barHeight, 0.1f}, Vector2{0, 0}), // 左上
            Vertex(Vector3{-barWidth/2,  barY, 0.1f}, Vector2{0, 1}), // 左下
            Vertex(Vector3{ barWidth/2,  barY, 0.1f}, Vector2{1, 1})  // 右下
        };
        std::vector<Index> indices = {0, 1, 2, 0, 2, 3};
        
        // 灰色のテクスチャ
        auto grayTexture = getColorTexture(0.3f, 0.3f, 0.3f);
        Model barBgModel(vertices, indices, grayTexture);
        
        // 描画前にユニットの位置に基づいてモデルマトリックスを設定
        float modelMatrix[16] = {0};
        
        // 単位行列を作成
        modelMatrix[0] = 1.0f;
        modelMatrix[5] = 1.0f;
        modelMatrix[10] = 1.0f;
        modelMatrix[15] = 1.0f;
        
        // ユニットの実際の位置を設定
        auto position = unit->getPosition();
    modelMatrix[12] = position.getX() + cameraOffsetX; // X座標
    modelMatrix[13] = position.getY() + cameraOffsetY; // Y座標
        
        // モデルマトリックスをシェーダに送信
        shader->setModelMatrix(modelMatrix);
        
        // バーの背景を描画
        shader->drawModel(barBgModel);
    }
    
    // HPバーの前景（緑～赤）
    if (hpRatio > 0) {
        // HPに応じた色（HPが低いほど赤くなる）
        float r = 1.0f - hpRatio;
        float g = hpRatio;
        float b = 0.0f;
        
        // HPの残量に応じたバーの幅を計算
        float currentWidth = barWidth * hpRatio;
        
        // バーの左端位置を計算（中央から左に向かってbarWidth/2だけずれた位置が左端）
        float leftX = -barWidth/2;
        
        // バーのモデル
        std::vector<Vertex> vertices = {
            Vertex(Vector3{ leftX + currentWidth,  barY + barHeight, 0.2f}, Vector2{1, 0}), // 右上
            Vertex(Vector3{ leftX,               barY + barHeight, 0.2f}, Vector2{0, 0}), // 左上
            Vertex(Vector3{ leftX,               barY, 0.2f}, Vector2{0, 1}), // 左下
            Vertex(Vector3{ leftX + currentWidth,  barY, 0.2f}, Vector2{1, 1})  // 右下
        };
        std::vector<Index> indices = {0, 1, 2, 0, 2, 3};
        
        auto hpTexture = getColorTexture(r, g, b);
        Model hpBarModel(vertices, indices, hpTexture);
        
        // 描画前にユニットの位置に基づいてモデルマトリックスを設定
        float modelMatrix[16] = {0};
        
        // 単位行列を作成
        modelMatrix[0] = 1.0f;
        modelMatrix[5] = 1.0f;
        modelMatrix[10] = 1.0f;
        modelMatrix[15] = 1.0f;
        
        // ユニットの実際の位置を設定
        auto position = unit->getPosition();
    modelMatrix[12] = position.getX() + cameraOffsetX; // X座標
    modelMatrix[13] = position.getY() + cameraOffsetY; // Y座標
        
        // モデルマトリックスをシェーダに送信
        shader->setModelMatrix(modelMatrix);
        
        // HPバーを描画
        shader->drawModel(hpBarModel);
    }
    
    // HPの数値を表示するには、テキストレンダリングが必要ですが、
    // この実装ではHP表示はバーのみにしています。
}

/**
 * @brief ユニット描画に使用する基本モデルを生成します。
 *
 * ここでは4頂点の単純な四角形を返し、ユニットごとにテクスチャを適用して描画します。
 */
Model UnitRenderer::createUnitModel() {
    // ユニットのサイズを1/4程度に小さく調整
    // 0 --- 1
    // | \   |
    // |  \  |
    // |   \ |
    // 3 --- 2
    std::vector<Vertex> vertices = {
        Vertex(Vector3{ 0.2f,  0.2f, 0}, Vector2{1, 0}), // 0: 右上
        Vertex(Vector3{-0.2f,  0.2f, 0}, Vector2{0, 0}), // 1: 左上
        Vertex(Vector3{-0.2f, -0.2f, 0}, Vector2{0, 1}), // 2: 左下
        Vertex(Vector3{ 0.2f, -0.2f, 0}, Vector2{1, 1})  // 3: 右下
    };
    
    std::vector<Index> indices = {
        0, 1, 2, 0, 2, 3 // 2つの三角形で四角形を構成
    };
    
    // テクスチャ処理のデバッグログを追加
    aout << "Creating solid color texture..." << std::endl;
    
    // デフォルトは明るい赤色（ユニット登録時に個別に色を変更する）
    auto redTexture = TextureAsset::createSolidColorTexture(1.0f, 0.0f, 0.0f);
    aout << "Created red texture with ID: " << redTexture->getTextureID() << std::endl;
    
    return Model(vertices, indices, redTexture);
}

/**
 * @brief 登録済みユニットをIDで取得します。
 */
std::shared_ptr<UnitEntity> UnitRenderer::getUnit(int unitId) const {
    auto it = units_.find(unitId);
    if (it != units_.end()) {
        return it->second;
    }
    return nullptr;
}

/**
 * @brief すべての登録ユニットを返します（参照返し）。
 */
const std::unordered_map<int, std::shared_ptr<UnitEntity>>& UnitRenderer::getAllUnits() const {
    return units_;
}
