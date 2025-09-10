#include "UnitRenderer.h"
#include "AndroidOut.h"

UnitRenderer::UnitRenderer(std::shared_ptr<TextureAsset> spTexture)
    : spTexture_(spTexture)
    , unitModel_(createUnitModel()) {
    
    aout << "UnitRenderer initialized with " 
         << (spTexture ? "provided texture" : "default red texture") << std::endl;
}

UnitRenderer::~UnitRenderer() {
    aout << "UnitRenderer destroyed" << std::endl;
}

void UnitRenderer::registerUnit(const std::shared_ptr<Unit>& unit) {
    if (unit) {
        units_[unit->getId()] = unit;
        
        // デフォルトテクスチャを設定（赤色）
        unitTextures_[unit->getId()] = getColorTexture(1.0f, 0.0f, 0.0f);
        
        aout << "Registered unit: " << unit->getName() << " (ID: " << unit->getId() << ")" << std::endl;
    }
}

void UnitRenderer::registerUnitWithColor(const std::shared_ptr<Unit>& unit, float r, float g, float b) {
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

void UnitRenderer::renderUnits(const Shader* shader) {
    for (const auto& pair : units_) {
        const auto& unitId = pair.first;
        const auto& unit = pair.second;
        
        // このユニット用のテクスチャを取得
        std::shared_ptr<TextureAsset> unitTexture = nullptr;
        
        if (!unit->isAlive()) {
            // 死亡している場合は灰色に変更
            unitTexture = getColorTexture(0.5f, 0.5f, 0.5f);
        } else if (unit->isAttacking()) {
            // 攻撃中は明るいオレンジ色に変更
            unitTexture = getColorTexture(1.0f, 0.6f, 0.2f);
            aout << unit->getName() << " is attacking - showing orange highlight" << std::endl;
        } else if (unit->isColliding()) {
            // 衝突中は明るい赤色に変更
            unitTexture = getColorTexture(1.0f, 0.2f, 0.2f);
        } else {
            // HP状態に応じて色を変化させる（HPが低いと赤っぽく、高いと元の色に近くなる）
            float hpRatio = static_cast<float>(unit->getCurrentHP()) / unit->getMaxHP();
            
            auto textureIt = unitTextures_.find(unitId);
            if (textureIt != unitTextures_.end()) {
                // 元の色情報を取得（簡易的な実装）
                float r = 0.3f, g = 0.3f, b = 1.0f; // デフォルト青色
                
                if (unitId == 1) {
                    r = 1.0f; g = 0.3f; b = 0.3f; // 赤ユニット
                } else if (unitId == 2) {
                    r = 0.3f; g = 0.3f; b = 1.0f; // 青ユニット
                } else if (unitId == 3) {
                    r = 0.3f; g = 1.0f; b = 0.3f; // 緑ユニット
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
        
        // ユニットの実際の位置を設定
        modelMatrix[12] = unit->getX(); // X座標
        modelMatrix[13] = unit->getY(); // Y座標
        
        // モデルマトリックスをシェーダに送信
        shader->setModelMatrix(modelMatrix);
        
        // ユニットを描画
        shader->drawModel(unitModel);
        
        // HP表示を描画（生きているユニットのみ）
        if (unit->isAlive()) {
            renderHPBar(shader, unit);
        }
    }
}

void UnitRenderer::updateUnits(float deltaTime) {
    // すべてのユニットのリストを作成
    std::vector<std::shared_ptr<Unit>> unitList;
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
        unit->avoidCollisions(unitList, AVOIDANCE_STRENGTH);
    }
    
    // 攻撃可能なユニットは敵を攻撃（戦闘中のユニットを優先）
    for (auto& pair : units_) {
        auto& unit = pair.second;
        
        // ユニットが生きていて攻撃可能な場合
        if (unit->isAlive() && unit->canAttack()) {
            // 戦闘中か衝突中の場合は優先的に攻撃
            if (unit->inCombat() || unit->isColliding()) {
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
        }
    }
    
    // ユニットの更新（移動や状態更新）
    for (auto& pair : units_) {
        pair.second->update(deltaTime);
    }
}

void UnitRenderer::renderHPBar(const Shader* shader, const std::shared_ptr<Unit>& unit) {
    // HP表示用のバーを描画する
    // バーの設定
    const float barWidth = 0.3f; // バーの幅
    const float barHeight = 0.05f; // バーの高さ
    const float barY = 0.25f; // ユニットの上端からの距離
    
    // HPの割合を計算
    float hpRatio = static_cast<float>(unit->getCurrentHP()) / unit->getMaxHP();
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
        modelMatrix[12] = unit->getX(); // X座標
        modelMatrix[13] = unit->getY(); // Y座標
        
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
        modelMatrix[12] = unit->getX(); // X座標
        modelMatrix[13] = unit->getY(); // Y座標
        
        // モデルマトリックスをシェーダに送信
        shader->setModelMatrix(modelMatrix);
        
        // HPバーを描画
        shader->drawModel(hpBarModel);
    }
    
    // HPの数値を表示するには、テキストレンダリングが必要ですが、
    // この実装ではHP表示はバーのみにしています。
}

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
