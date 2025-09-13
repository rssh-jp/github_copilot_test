# Frameworks Layer

このディレクトリは、クリーンアーキテクチャのFrameworks & Drivers層を表します。

## 構造

### android/
- AndroidOut.cpp/h: Android ログ出力機能
- UnitStatusJNI.cpp: JNI Bridge for game state access and touch handling

### graphics/
- Renderer.cpp/h: OpenGL ES レンダリングエンジン
- Shader.cpp/h: シェーダー管理
- TextureAsset.cpp/h: テクスチャリソース管理
- UnitRenderer.cpp/h: ユニット描画専用レンダラー

### input/
- （将来的に入力処理系ファイルを配置）

### utils/
- Utility.cpp/h: 汎用ユーティリティ関数

## 依存関係

Frameworks層は他の層に依存せず、最も外側の層として機能します。
