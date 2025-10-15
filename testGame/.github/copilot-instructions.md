<!--
このファイルは `testGame` リポジトリで作業する AI コーディングエージェント向けの手引きです。
短く（20〜50行程度）まとめ、具体的なファイルやパターンを参照してください。
-->

# testGame 用 Copilot 指示書

目的
- 本リポジトリのクリーンアーキテクチャ（domain / usecases / frameworks / adapters）に沿って、Android ネイティブ（C++）ゲームの実装・リファクタ・拡張を支援すること。

簡単な概観
- このプロジェクトは Android Gradle を使い、ネイティブ C++ 層を持ちます（`app` モジュール）。NDK/CMake 等のビルド設定は `app/build.gradle.kts` を参照してください。
- ネイティブソースは `app/src/main/cpp/` 配下にあり、概ね次のレイヤーで整理されています:
  - `domain/`（エンティティ、ドメインサービス、値オブジェクト）
  - `usecases/`（`MovementUseCase`, `CombatUseCase` — アプリケーションロジック、コールバック）
  - `frameworks/`（`graphics/`, `android/`, `utils/` — プラットフォーム統合、レンダリング、JNI ブリッジ）
  - `adapters/`（外部／システムアダプタ）

前提（想定事項）
- ビルド・実行は Android Studio または Gradle ラッパー（`gradlew` / `gradlew.bat`）を想定（Windows）。ネイティブツールチェインは `app/build.gradle.kts` と `CMakeLists.txt` で設定されています。
- リポジトリの設計方針は `copilot.prompt.md` にまとまっています。変更時はそちらの制約（クリーンアーキテクチャや読みやすさ）を優先してください。

主要なパターンと注目箇所
- 依存性逆転が使われています：ユースケースはドメイン（エンティティ／サービス）に依存し、frameworks 層が具体的なレンダリングや入力を提供します。コールバックベースの API は `usecases/MovementUseCase.h` と `usecases/CombatUseCase.h` を参照してください。
- レンダリングや入力は Android の GameActivity / native_app_glue に密接に結びついています。プラットフォームの扱い方や座標変換の例は `app/src/main/cpp/frameworks/graphics/Renderer.cpp` の `screenToWorldCoordinates` や `moveUnitToPosition` を参照してください。
- ユニットや戦闘のロジックは `domain/entities/UnitEntity.*` と `domain/services/CombatDomainService.*` にあります。ドメインの振る舞い変更はレンダラではなく domain/usecases 側で行ってください。

守るべき慣習
- C++ コードは既存のレイヤー構成を保ち、所有権セマンティクス（`std::shared_ptr` 等）を崩さないでください。
- ログ出力は既存の `frameworks/android/AndroidOut.cpp/h`（`aout <<`）を使ってください。`printf` は避ける。
- シェーダは現状 `Renderer.cpp` に埋め込まれています。資産を追加する場合は既存の `TextureAsset` / `Shader` の抽象に倣ってください。

ビルド・テスト・デバッグのヒント
- ビルド（Windows PowerShell）: リポジトリルートから

```powershell
./gradlew.bat assembleDebug
```

スクリプト、linuxコマンドの実行
- wslコマンドで実行すること：wsl

- ネイティブビルドは Gradle + CMake（`app/src/main/cpp/CMakeLists.txt`）で制御されます。ネイティブコードを変更したらフルビルドで再ビルドされます。
- 実機／エミュレータ実行は Android Studio を推奨。immersive モードやサーフェスサイズ変更は `Renderer::updateRenderArea()` 内で扱われています。

具体的な実装例（小さなタスク）
- 新しい移動ルールを追加する場合は `usecases/MovementUseCase.cpp` を編集し、衝突判定等は `domain/services/CollisionDomainService` を呼び出すようにしてください。ドメインロジックを `Renderer.cpp` に入れないでください。
- ユニット選択の UI コールバックを追加する場合は `frameworks/android/UnitStatusJNI.cpp` で JNI ブリッジを作り、`Renderer::moveUnitToPosition` や MovementUseCase の API を呼び出すパターンに従ってください。

不明点があるとき
- ゲームロジックの変更はまず `domain/`・`usecases/` を触るのが基本です。`frameworks/` はプラットフォーム固有の処理（レンダリング・入力）に限定してください。
- リポジトリから設計方針が読み取れない重大な選択（例：ネットワーク連携、永続化フォーマット、対象 Android API レベルなど）がある場合は確認を求めてください。

最初に読むべきファイル
- `copilot.prompt.md`（プロジェクト全体のゴールと制約）
- `app/build.gradle.kts`（ビルド／ツールチェイン設定）
- `app/src/main/cpp/frameworks/graphics/Renderer.cpp`（レンダリング・入力の実装例）
- `app/src/main/cpp/usecases/MovementUseCase.*` / `usecases/CombatUseCase.*`（アプリケーションロジック）

## 設計ルール（プロジェクトで使われる設計ルール）

このリポジトリでは次の設計ルールを基準に作業してください（`copilot.prompt.md` の要旨）:

- クリーンアーキテクチャの原則に従う（`domain` / `usecases` / `frameworks` / `adapters` を明確に分離する）。
- 読みやすさ（リーダブルコード）を重視する。命名・関数分割・コメントを丁寧に行う。
- まずはドメイン層（エンティティ）とユースケース（ビジネスロジック）を設計・実装する。
- 依存関係逆転の原則を守り、明確なインターフェース／抽象を使って層を接続する。
- ファイルが長くなりすぎないように分割案を提示する（1ファイル = 単一責任を目安）。
- 実装変更時には設計意図・メリット・デメリットを短くコメント／ドキュメント化する。
- 可能な範囲でテストコードを追加する（ユースケース層・ドメインロジックを優先）。

具体的な優先事項（採用済みルールの例）:
- ユニットの攻撃範囲・衝突判定・2D 見下ろし表現はドメイン + ユースケースで扱う（`domain/entities`, `usecases/`）。
- UI / 入力、レンダリングは `frameworks/` に閉じる。ドメインロジックをレンダラに混ぜないでください。

上記ルールに沿って、実装／修正／レビューを行ってください。

# このゲームの仕様
## あなたはプロのソフトウェアアーキテクト兼ゲームプログラマーです。

### 前提
- クリーンアーキテクチャの原則に従った実装を行ってください。
- 「リーダブルコード」の考え方を重視し、読みやすく保守性の高いコードを出力してください。
- ゲームのスケルトン（雛形）から、各レイヤー（エンティティ、ユースケース、インターフェース、外部フレームワーク等）を意識して設計してください。
- 応答はすべて日本語で行うこと

### やってほしいこと
- [シミュレーションゲーム]
  - ユニットの攻撃範囲に入った時の戦闘
  - ユニット同士の衝突判定
  - 見下ろし方の2Dでの表現
  - ステータス画面などのUI作成
- まずはドメイン層（エンティティ）とユースケース（ビジネスロジック）の設計から始めてください。
- 依存関係逆転の原則を守り、インターフェース/抽象クラスを活用してください。
- 設計意図やレイヤー分割の理由も、コメントやドキュメントで解説してください。

### コーディングルール
- 名前付け・関数分割・コメントは「リーダブルコード」に準拠してください。
- 1ファイルが長くなりすぎないよう、適宜ファイル分割案も提案してください。
- テストコードも可能な範囲で出力してください。
- 日本語でコメントを付けて、処理に迷わないようにしてください。

### 注意
- あなたは設計レビューも行う立場です。提案した設計のメリット・デメリットも簡単に記載してください。
- 分からない仕様は質問してください。
- コードのビルドはAndroid Studioでおこなうので、その手順は不要です。
- 以下の内容を基に実装を行ってください。

## 依頼内容
copilot-agent.mdに書いてある依頼内容を基に実装を行ってください。