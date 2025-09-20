#ifndef TESTGAME_UNITRENDERER_H
#define TESTGAME_UNITRENDERER_H

#include "entities/UnitEntity.h"
#include "Model.h"
#include "Shader.h"
#include <memory>
#include <vector>
#include <unordered_map>

/**
 * @brief ユニットの描画を管理するクラス
 * 
 * このクラスは、ユニットの位置と見た目を更新し、
 * OpenGLを通じて画面に描画する機能を提供します。
 */
class UnitRenderer {
public:
    /**
     * @brief コンストラクタ
     * 
     * @param spTexture ユニット描画に使用するデフォルトテクスチャ
     */
    explicit UnitRenderer(std::shared_ptr<TextureAsset> spTexture);
    
    /**
     * @brief デストラクタ
     */
    ~UnitRenderer();
    
    /**
     * @brief 新しいユニットをレンダラーに登録する
     * 
     * @param unit 登録するユニット
     */
    void registerUnit(const std::shared_ptr<UnitEntity>& unit);
    
    /**
     * @brief 新しいユニットを特定の色で登録する
     * 
     * @param unit 登録するユニット
     * @param r 赤成分 (0.0～1.0)
     * @param g 緑成分 (0.0～1.0)
     * @param b 青成分 (0.0～1.0)
     */
    void registerUnitWithColor(const std::shared_ptr<UnitEntity>& unit, float r, float g, float b);
    
    /**
     * @brief ユニットの登録を解除する
     * 
     * @param unitId 解除するユニットのID
     */
    void unregisterUnit(int unitId);
    
    /**
     * @brief 全ユニットをクリアする
     */
    void clearAllUnits();
    
    /**
     * @brief ユニットを描画する
     * 
     * @param shader 描画に使用するシェーダー
     */
    // Render units using the provided shader. Provide camera offsets so unit renderer
    // can correctly place units in the world relative to the camera.
    void render(const Shader* shader, float cameraOffsetX, float cameraOffsetY);
    
    /**
     * @brief 特定のユニットのHPバーを描画する
     * 
     * @param shader 描画に使用するシェーダー
     * @param unit HPバーを描画するユニット
     */
    void renderHPBar(const Shader* shader, const std::shared_ptr<UnitEntity>& unit, float cameraOffsetX, float cameraOffsetY);
    
    /**
     * @brief すべてのユニットの状態を更新する
     * 
     * @param deltaTime 前回の更新からの経過時間（秒）
     */
    void updateUnits(float deltaTime);
    
    /**
     * @brief 指定したIDのユニットを取得する
     * 
     * @param unitId ユニットのID
     * @return 指定されたIDのユニット（存在しない場合はnullptr）
     */
    std::shared_ptr<UnitEntity> getUnit(int unitId) const;
    
    /**
     * @brief すべてのユニットを取得する
     * 
     * @return すべてのユニットのマップ
     */
    const std::unordered_map<int, std::shared_ptr<UnitEntity>>& getAllUnits() const;

    /**
     * @brief 当たり判定のワイヤーフレーム表示を有効／無効にする
     */
    void setShowCollisionWireframes(bool show);

    /**
     * @brief 当たり判定ワイヤーフレームを描画する
     */
    void renderCollisionWireframes(const Shader* shader, float cameraOffsetX, float cameraOffsetY);

private:
    // ユニットのモデルデータを生成する
    Model createUnitModel();
    
    // 指定した色のテクスチャを取得する（キャッシュあり）
    std::shared_ptr<TextureAsset> getColorTexture(float r, float g, float b);
    
    // ユニットの表示に使用するデフォルトテクスチャ
    std::shared_ptr<TextureAsset> spTexture_;
    
    // 登録されたユニット
    std::unordered_map<int, std::shared_ptr<UnitEntity>> units_;
    
    // ユニットのテクスチャマップ (ユニットID -> テクスチャ)
    std::unordered_map<int, std::shared_ptr<TextureAsset>> unitTextures_;
    
    // カラーテクスチャのキャッシュ (RGBキー -> テクスチャ)
    std::unordered_map<std::string, std::shared_ptr<TextureAsset>> colorTextureCache_;
    
    // ユニットのモデル
    Model unitModel_;

    // 当たり判定ワイヤーフレームの表示フラグ
    bool showCollisionWireframes_ = false;
};

#endif // TESTGAME_UNITRENDERER_H
