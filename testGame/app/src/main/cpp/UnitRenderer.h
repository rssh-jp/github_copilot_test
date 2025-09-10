#ifndef TESTGAME_UNITRENDERER_H
#define TESTGAME_UNITRENDERER_H

#include "Unit.h"
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
    void registerUnit(const std::shared_ptr<Unit>& unit);
    
    /**
     * @brief 新しいユニットを特定の色で登録する
     * 
     * @param unit 登録するユニット
     * @param r 赤成分 (0.0～1.0)
     * @param g 緑成分 (0.0～1.0)
     * @param b 青成分 (0.0～1.0)
     */
    void registerUnitWithColor(const std::shared_ptr<Unit>& unit, float r, float g, float b);
    
    /**
     * @brief ユニットの登録を解除する
     * 
     * @param unitId 解除するユニットのID
     */
    void unregisterUnit(int unitId);
    
    /**
     * @brief 登録されたすべてのユニットをレンダリングする
     * 
     * @param shader 使用するシェーダー
     */
    void renderUnits(const Shader* shader);
    
    /**
     * @brief ユニットのHPバーを描画する
     * 
     * @param shader 使用するシェーダー
     * @param unit 対象のユニット
     */
    void renderHPBar(const Shader* shader, const std::shared_ptr<Unit>& unit);
    
    /**
     * @brief 登録されたすべてのユニットの位置を更新する
     * 
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void updateUnits(float deltaTime);
    
private:
    // ユニットのモデルデータを生成する
    Model createUnitModel();
    
    // 指定した色のテクスチャを取得する（キャッシュあり）
    std::shared_ptr<TextureAsset> getColorTexture(float r, float g, float b);
    
    // ユニットの表示に使用するデフォルトテクスチャ
    std::shared_ptr<TextureAsset> spTexture_;
    
    // 登録されたユニット
    std::unordered_map<int, std::shared_ptr<Unit>> units_;
    
    // ユニットのテクスチャマップ (ユニットID -> テクスチャ)
    std::unordered_map<int, std::shared_ptr<TextureAsset>> unitTextures_;
    
    // カラーテクスチャのキャッシュ (RGBキー -> テクスチャ)
    std::unordered_map<std::string, std::shared_ptr<TextureAsset>> colorTextureCache_;
    
    // ユニットのモデル
    Model unitModel_;
};

#endif // TESTGAME_UNITRENDERER_H
