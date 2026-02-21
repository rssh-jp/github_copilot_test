# エージェントガイドライン

このドキュメントは、本プロジェクトの開発に関わるすべてのエージェントが参照すべきガイドラインです。

## プロジェクト概要

本プロジェクトは **Kotlin / Jetpack Compose** で構築された Android アプリ（得点集計 TestApp2）です。
詳細は `.github/copilot-instructions.md` を参照してください。

## エージェント構成

| エージェント | ファイル | 役割 |
|------------|---------|------|
| Orchestrator | `.github/agents/orchestrator.agent.md` | ワークフロー全体の進行管理、サブエージェントへの指示 |
| Git | `.github/agents/git.agent.md` | ブランチ作成・コミット・プッシュ・PR作成 |
| Spec | `.github/agents/spec.agent.md` | ユーザー要望の解析、仕様ドキュメントの作成 |
| Impl | `.github/agents/impl.agent.md` | 仕様に基づくコード実装 |
| Test | `.github/agents/test.agent.md` | 実装コードの自動テスト実装 |
| TechLead | `.github/agents/techlead.agent.md` | テックリードとしてのコードレビュー |
| Quality | `.github/agents/quality.agent.md` | 総合的な品質チェックと最終承認 |
| Release | `.github/agents/release.agent.md` | リリースビルド（AAB/APK）の作成と成果物確認 |

## ワークフロー

### 機能開発・バグ修正フロー（Orchestrator）

```
ユーザー要望
    ↓
[Orchestrator]
    ↓
[Git] → main からブランチ作成
    ↓
[Spec] → 仕様ドキュメント
    ↓
[Impl] → 実装コード
    ↓
[Test] → テストコード
    ↓
[TechLead] → レビュー結果
    ↓（要修正の場合は Impl へ戻る）
[Quality] → 品質レポート
    ↓
[Git] → コミット
    ↓
[Release] → リリースビルド（AAB）
    ↓（失敗の場合は Impl へ戻る）
[Git] → プッシュ → PR 作成
    ↓
完了
```

## 開発ポリシー

### 技術スタック

- **言語**: Kotlin
- **UI フレームワーク**: Jetpack Compose (Material3)
- **データベース**: Room Database
- **ビルドシステム**: Gradle (Kotlin DSL)
- **最小 SDK**: 35 / **ターゲット SDK**: 36

### ディレクトリ構造

```
app/src/main/java/com/example/testapp2/
├── MainActivity.kt
├── data/
│   ├── AppState.kt
│   ├── models.kt
│   ├── enums.kt
│   └── db/
│       ├── AppDatabase.kt
│       ├── Dao.kt
│       └── Entities.kt
└── ui/
    ├── components/
    ├── screens/
    └── theme/
```

### コーディング規約

- Kotlin の型安全性を保つこと（`Any` 型の乱用禁止）
- コンポーザブルは `ui/components/` または `ui/screens/` に配置する
- 状態は `AppState` を Single Source of Truth として管理する
- DB 操作はバックグラウンドスレッドで非同期（`CoroutineScope`）に実行する
- コメントは日本語で記述すること
- コミットメッセージは日本語で記述すること

### テスト規約

- テストフレームワーク: JUnit4 + Compose UI Test
- テストの説明は日本語で記述すること
- ユニットテストは `app/src/test/`、インストゥルメンテーションテストは `app/src/androidTest/` に配置する

### ビルド・デプロイ

- ビルドコマンド（PowerShell）:
  ```powershell
  $env:JAVA_HOME = "C:\Program Files\Android\Android Studio\jbr"
  .\gradlew.bat assembleDebug
  ```
- ビルドが成功することを必ず確認すること
- バージョン情報は `app/build.gradle.kts` の `versionCode` / `versionName` で管理する
