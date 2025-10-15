#include "TextRenderer.h"
#include "android/AndroidOut.h"
#include <cmath>
#include <vector>

/*
 * TextRenderer.cpp
 *
 * 責務:
 * - ビットマップフォントを使用して数値やテキストを画面に描画する
 * - カメラのズームレベルに関わらず一定サイズで表示できるよう補正する
 *
 * 設計方針:
 * - 外部フォントファイルに依存せず、プログラム内でシンプルな数値フォントを生成
 * - HP表示など、ゲーム内の重要な情報をわかりやすく表示する
 */

/**
 * @brief コンストラクタ
 *
 * フォントテクスチャを生成して初期化します。
 */
TextRenderer::TextRenderer() {
  fontTexture_ = createNumberFontTexture();
  aout << "TextRenderer initialized with bitmap font" << std::endl;
}

/**
 * @brief デストラクタ
 */
TextRenderer::~TextRenderer() {
  aout << "TextRenderer destroyed" << std::endl;
}

/**
 * @brief 数値フォント用ビットマップテクスチャを生成する
 *
 * 0-9および'/'の11文字をプログラム内で生成します。
 * 各文字は11x11ピクセルで、シンプルなドット描画で表現します。
 */
std::shared_ptr<TextureAsset>
TextRenderer::createNumberFontTexture() {
  // テクスチャサイズは128x128（十分なサイズ）
  const int texSize = kFontTextureSize;
  std::vector<uint8_t> pixels(texSize * texSize * 4, 0);

  // 背景は透明（RGBA = 0,0,0,0）
  // 文字は白色（RGBA = 255,255,255,255）

  // 各文字のビットマップパターン（11x11）
  // 1 = 白ピクセル、0 = 透明
  // 文字: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, /

  // シンプルな5x7のビットマップを使用（中央配置）
  const int charPatterns[11][7][5] = {
      // 0
      {{0, 1, 1, 1, 0},
       {1, 0, 0, 0, 1},
       {1, 0, 0, 0, 1},
       {1, 0, 0, 0, 1},
       {1, 0, 0, 0, 1},
       {1, 0, 0, 0, 1},
       {0, 1, 1, 1, 0}},
      // 1
      {{0, 0, 1, 0, 0},
       {0, 1, 1, 0, 0},
       {0, 0, 1, 0, 0},
       {0, 0, 1, 0, 0},
       {0, 0, 1, 0, 0},
       {0, 0, 1, 0, 0},
       {0, 1, 1, 1, 0}},
      // 2
      {{0, 1, 1, 1, 0},
       {1, 0, 0, 0, 1},
       {0, 0, 0, 0, 1},
       {0, 0, 0, 1, 0},
       {0, 0, 1, 0, 0},
       {0, 1, 0, 0, 0},
       {1, 1, 1, 1, 1}},
      // 3
      {{0, 1, 1, 1, 0},
       {1, 0, 0, 0, 1},
       {0, 0, 0, 0, 1},
       {0, 0, 1, 1, 0},
       {0, 0, 0, 0, 1},
       {1, 0, 0, 0, 1},
       {0, 1, 1, 1, 0}},
      // 4
      {{0, 0, 0, 1, 0},
       {0, 0, 1, 1, 0},
       {0, 1, 0, 1, 0},
       {1, 0, 0, 1, 0},
       {1, 1, 1, 1, 1},
       {0, 0, 0, 1, 0},
       {0, 0, 0, 1, 0}},
      // 5
      {{1, 1, 1, 1, 1},
       {1, 0, 0, 0, 0},
       {1, 1, 1, 1, 0},
       {0, 0, 0, 0, 1},
       {0, 0, 0, 0, 1},
       {1, 0, 0, 0, 1},
       {0, 1, 1, 1, 0}},
      // 6
      {{0, 0, 1, 1, 0},
       {0, 1, 0, 0, 0},
       {1, 0, 0, 0, 0},
       {1, 1, 1, 1, 0},
       {1, 0, 0, 0, 1},
       {1, 0, 0, 0, 1},
       {0, 1, 1, 1, 0}},
      // 7
      {{1, 1, 1, 1, 1},
       {0, 0, 0, 0, 1},
       {0, 0, 0, 1, 0},
       {0, 0, 1, 0, 0},
       {0, 1, 0, 0, 0},
       {0, 1, 0, 0, 0},
       {0, 1, 0, 0, 0}},
      // 8
      {{0, 1, 1, 1, 0},
       {1, 0, 0, 0, 1},
       {1, 0, 0, 0, 1},
       {0, 1, 1, 1, 0},
       {1, 0, 0, 0, 1},
       {1, 0, 0, 0, 1},
       {0, 1, 1, 1, 0}},
      // 9
      {{0, 1, 1, 1, 0},
       {1, 0, 0, 0, 1},
       {1, 0, 0, 0, 1},
       {0, 1, 1, 1, 1},
       {0, 0, 0, 0, 1},
       {0, 0, 0, 1, 0},
       {0, 1, 1, 0, 0}},
      // / (スラッシュ)
      {{0, 0, 0, 0, 1},
       {0, 0, 0, 1, 0},
       {0, 0, 0, 1, 0},
       {0, 0, 1, 0, 0},
       {0, 1, 0, 0, 0},
       {0, 1, 0, 0, 0},
       {1, 0, 0, 0, 0}},
  };

  // 各文字をテクスチャに描画
  for (int charIdx = 0; charIdx < 11; ++charIdx) {
    int charX = (charIdx % kFontCharsPerRow) * kFontBitmapWidth;
    int charY = (charIdx / kFontCharsPerRow) * kFontBitmapHeight;

    // パターンを中央配置（11x11の中に5x7を配置）
    int offsetX = (kFontBitmapWidth - 5) / 2;
    int offsetY = (kFontBitmapHeight - 7) / 2;

    for (int py = 0; py < 7; ++py) {
      for (int px = 0; px < 5; ++px) {
        if (charPatterns[charIdx][py][px]) {
          int texX = charX + offsetX + px;
          int texY = charY + offsetY + py;
          int index = (texY * texSize + texX) * 4;

          // 白色不透明
          pixels[index + 0] = 255; // R
          pixels[index + 1] = 255; // G
          pixels[index + 2] = 255; // B
          pixels[index + 3] = 255; // A
        }
      }
    }
  }

  // テクスチャを作成
  return TextureAsset::createFromPixels(texSize, texSize, pixels);
}

/**
 * @brief テキストの幅を計算する
 */
float TextRenderer::calculateTextWidth(const std::string &text, float scale,
                                       float cameraZoom) const {
  // ズーム補正を適用したスケール
  float adjustedScale = scale / cameraZoom;
  return text.length() * kCharWidth * adjustedScale;
}

/**
 * @brief 単一文字を描画する
 */
float TextRenderer::renderChar(const Shader *shader, char c, float x, float y,
                               float scale, float cameraZoom, float r, float g,
                               float b) {
  // 文字のインデックスを取得
  int charIndex = -1;
  if (c >= '0' && c <= '9') {
    charIndex = c - '0';
  } else if (c == '/') {
    charIndex = 10;
  } else {
    // サポートされていない文字はスキップ
    return kCharWidth * scale / cameraZoom;
  }

  // テクスチャ座標を計算
  float texCharWidth = static_cast<float>(kFontBitmapWidth) / kFontTextureSize;
  float texCharHeight =
      static_cast<float>(kFontBitmapHeight) / kFontTextureSize;

  int charCol = charIndex % kFontCharsPerRow;
  int charRow = charIndex / kFontCharsPerRow;

  float u0 = charCol * texCharWidth;
  float v0 = charRow * texCharHeight;
  float u1 = u0 + texCharWidth;
  float v1 = v0 + texCharHeight;

  // ズーム補正を適用したスケール（カメラがズームインしても文字サイズは一定）
  float adjustedScale = scale / cameraZoom;
  float charWidth = kCharWidth * adjustedScale;
  float charHeight = kCharHeight * adjustedScale;

  // 文字のクアッド（四角形）を作成（原点からの相対座標）
  std::vector<Vertex> vertices = {
      Vertex(Vector3{charWidth, charHeight, 0.3f}, Vector2{u1, v0}), // 右上
      Vertex(Vector3{0, charHeight, 0.3f}, Vector2{u0, v0}),         // 左上
      Vertex(Vector3{0, 0, 0.3f}, Vector2{u0, v1}),                  // 左下
      Vertex(Vector3{charWidth, 0, 0.3f}, Vector2{u1, v1})           // 右下
  };

  std::vector<Index> indices = {0, 1, 2, 0, 2, 3};

  // 色付きテクスチャを使用（カラー変調）
  // 注: 現在のシェーダーはテクスチャの色をそのまま使うため、
  // 色の変更には別途カラーテクスチャを使うか、シェーダーの拡張が必要
  // ここでは白色テクスチャをそのまま使用
  Model charModel(vertices, indices, fontTexture_);

  // モデル行列を設定（xとyの位置に配置）
  float modelMatrix[16] = {0};
  modelMatrix[0] = 1.0f;
  modelMatrix[5] = 1.0f;
  modelMatrix[10] = 1.0f;
  modelMatrix[15] = 1.0f;
  modelMatrix[12] = x; // X座標
  modelMatrix[13] = y; // Y座標

  shader->setModelMatrix(modelMatrix);
  shader->drawModel(charModel);

  return charWidth;
}

/**
 * @brief テキストを描画する
 */
void TextRenderer::renderText(const Shader *shader, const std::string &text,
                              float x, float y, float scale, float cameraZoom,
                              float r, float g, float b) {
  float currentX = x;

  for (char c : text) {
    float charWidth = renderChar(shader, c, currentX, y, scale, cameraZoom, r, g, b);
    currentX += charWidth;
  }
}

/**
 * @brief 整数値を描画する
 */
void TextRenderer::renderNumber(const Shader *shader, int value, float x,
                                float y, float scale, float cameraZoom, float r,
                                float g, float b) {
  std::string text = std::to_string(value);
  renderText(shader, text, x, y, scale, cameraZoom, r, g, b);
}

/**
 * @brief HP表示（"現在HP/最大HP"形式）を描画する
 *
 * テキストを中央揃えで表示します。
 */
void TextRenderer::renderHP(const Shader *shader, int currentHp, int maxHp,
                            float x, float y, float scale, float cameraZoom,
                            float r, float g, float b) {
  // HP文字列を作成
  std::string hpText =
      std::to_string(currentHp) + "/" + std::to_string(maxHp);

  // テキストの幅を計算して中央揃え
  float textWidth = calculateTextWidth(hpText, scale, cameraZoom);
  float startX = x - textWidth / 2.0f;

  // 描画
  renderText(shader, hpText, startX, y, scale, cameraZoom, r, g, b);
}
