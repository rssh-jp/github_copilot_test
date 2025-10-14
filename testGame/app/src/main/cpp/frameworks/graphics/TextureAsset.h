#ifndef ANDROIDGLINVESTIGATIONS_TEXTUREASSET_H
#define ANDROIDGLINVESTIGATIONS_TEXTUREASSET_H

#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include <memory>
#include <string>
#include <vector>

class TextureAsset {
public:
  /*!
   * Loads a texture asset from the assets/ directory
   * @param assetManager Asset manager to use
   * @param assetPath The path to the asset
   * @return a shared pointer to a texture asset, resources will be reclaimed
   * when it's cleaned up
   */
  static std::shared_ptr<TextureAsset> loadAsset(AAssetManager *assetManager,
                                                 const std::string &assetPath);

  /*!
   * 単色のテクスチャを作成する
   * @param r 赤成分 (0.0～1.0)
   * @param g 緑成分 (0.0～1.0)
   * @param b 青成分 (0.0～1.0)
   * @param a アルファ成分 (0.0～1.0)
   * @return 単色テクスチャのシェアードポインタ
   */
  static std::shared_ptr<TextureAsset>
  createSolidColorTexture(float r, float g, float b, float a = 1.0f);

  static std::shared_ptr<TextureAsset>
  createFromPixels(int width, int height, const std::vector<uint8_t> &rgbaData);

  ~TextureAsset();

  /*!
   * @return the texture id for use with OpenGL
   */
  constexpr GLuint getTextureID() const { return textureID_; }

private:
  inline TextureAsset(GLuint textureId) : textureID_(textureId) {}

  GLuint textureID_;
};

#endif // ANDROIDGLINVESTIGATIONS_TEXTUREASSET_H