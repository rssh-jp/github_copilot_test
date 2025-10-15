#ifndef TESTGAME_TEXTRENDERER_H
#define TESTGAME_TEXTRENDERER_H

#include "Model.h"
#include "Shader.h"
#include "TextureAsset.h"
#include <memory>
#include <string>
#include <unordered_map>

/**
 * @brief テキスト（主に数値）をレンダリングするクラス
 *
 * このクラスは、ビットマップフォントを使用して数値やシンプルな文字列を
 * 画面に描画する機能を提供します。カメラのズームレベルに関わらず、
 * 常に一定サイズで表示されるよう補正機能も備えています。
 */
class TextRenderer {
public:
  /**
   * @brief コンストラクタ
   *
   * フォントビットマップを自動生成し、初期化します。
   */
  TextRenderer();

  /**
   * @brief デストラクタ
   */
  ~TextRenderer();

  /**
   * @brief テキストを描画する
   *
   * @param shader 描画に使用するシェーダー
   * @param text 描画する文字列（数値文字と'/'のみサポート）
   * @param x ワールド座標でのX位置
   * @param y ワールド座標でのY位置
   * @param scale 文字のスケール（基準サイズに対する倍率）
   * @param cameraZoom カメラのズームレベル（ズーム補正用）
   * @param r 赤成分（0.0～1.0）
   * @param g 緑成分（0.0～1.0）
   * @param b 青成分（0.0～1.0）
   */
  void renderText(const Shader *shader, const std::string &text, float x,
                  float y, float scale, float cameraZoom, float r = 1.0f,
                  float g = 1.0f, float b = 1.0f);

  /**
   * @brief 整数値を指定位置に描画する（便利メソッド）
   *
   * @param shader 描画に使用するシェーダー
   * @param value 描画する整数値
   * @param x ワールド座標でのX位置
   * @param y ワールド座標でのY位置
   * @param scale 文字のスケール
   * @param cameraZoom カメラのズームレベル
   * @param r 赤成分
   * @param g 緑成分
   * @param b 青成分
   */
  void renderNumber(const Shader *shader, int value, float x, float y,
                    float scale, float cameraZoom, float r = 1.0f,
                    float g = 1.0f, float b = 1.0f);

  /**
   * @brief HP表示（"現在HP/最大HP"形式）を描画する
   *
   * @param shader 描画に使用するシェーダー
   * @param currentHp 現在のHP
   * @param maxHp 最大HP
   * @param x ワールド座標でのX位置（中央揃え）
   * @param y ワールド座標でのY位置
   * @param scale 文字のスケール
   * @param cameraZoom カメラのズームレベル
   * @param r 赤成分
   * @param g 緑成分
   * @param b 青成分
   */
  void renderHP(const Shader *shader, int currentHp, int maxHp, float x,
                float y, float scale, float cameraZoom, float r = 1.0f,
                float g = 1.0f, float b = 1.0f);

private:
  /**
   * @brief 数値フォント用ビットマップテクスチャを生成する
   *
   * プログラム内で単純な数値ビットマップ（0-9, '/'）を生成し、
   * テクスチャとして登録します。
   *
   * @return 生成されたテクスチャ
   */
  std::shared_ptr<TextureAsset> createNumberFontTexture();

  /**
   * @brief 単一文字を描画する
   *
   * @param shader 描画に使用するシェーダー
   * @param c 描画する文字
   * @param x ワールド座標でのX位置
   * @param y ワールド座標でのY位置
   * @param scale スケール
   * @param cameraZoom カメラのズームレベル
   * @param r 赤成分
   * @param g 緑成分
   * @param b 青成分
   * @return 描画した文字の幅（次の文字の描画開始位置用）
   */
  float renderChar(const Shader *shader, char c, float x, float y, float scale,
                   float cameraZoom, float r, float g, float b);

  /**
   * @brief テキストの幅を計算する
   *
   * @param text 計算対象の文字列
   * @param scale スケール
   * @param cameraZoom カメラのズームレベル
   * @return テキストの総幅
   */
  float calculateTextWidth(const std::string &text, float scale,
                           float cameraZoom) const;

  // フォントテクスチャ
  std::shared_ptr<TextureAsset> fontTexture_;

  // 文字サイズ情報（ピクセル単位でのフォント画像内のサイズ）
  static constexpr int kFontBitmapWidth = 11;  // 各文字の幅（ピクセル）
  static constexpr int kFontBitmapHeight = 11; // 各文字の高さ（ピクセル）
  static constexpr int kFontCharsPerRow = 11;  // 1行あたりの文字数
  static constexpr int kFontTextureSize = 128; // テクスチャ全体のサイズ

  // ワールド座標での基準サイズ（表示サイズ）
  static constexpr float kCharWidth = 0.08f;  // ワールド単位での文字幅
  static constexpr float kCharHeight = 0.12f; // ワールド単位での文字高さ
};

#endif // TESTGAME_TEXTRENDERER_H
