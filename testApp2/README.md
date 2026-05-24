# 得点集計 (TestApp2)

## プロジェクト概要

複数ユーザーのゲームセッションとスコアを管理する Android アプリです。

**アプリケーション ID**: `jp.rssh.testapp2`（ソースパッケージは `com.example.testapp2`）

## 主な機能

- セッション管理（作成・一覧・削除）
- ユーザー管理（セッションへのプレイヤー追加）
- リアルタイム経過時間トラッキング
- ラウンドベースのスコア入力・履歴ログ表示

---

## 技術スタック

| 項目 | 内容 |
|------|------|
| 言語 | Kotlin |
| UI フレームワーク | Jetpack Compose (Material3) |
| データベース | Room |
| ビルドシステム | Gradle (Kotlin DSL) |
| 最小 SDK | 35 (Android 15) |
| ターゲット SDK | 36 |

---

## 動作環境 / 前提条件

- Android Studio（最新安定版推奨）
- JDK: Android Studio 同梱の JBR（`C:\Program Files\Android\Android Studio\jbr`）
- Android SDK: API 35 以上
- 実機またはエミュレータ（API 35 以上の AVD）

---

## プロジェクト構造

```
app/src/main/java/com/example/testapp2/
├── MainActivity.kt          # エントリーポイント、ナビゲーション制御
├── data/
│   ├── AppState.kt          # 状態管理の中枢 & リポジトリロジック
│   ├── models.kt            # ドメインモデル (Session, User, ScoreRecord)
│   ├── enums.kt             # 列挙型 (MenuType, Screen)
│   └── db/                  # Room データベースコンポーネント
│       ├── AppDatabase.kt
│       ├── Dao.kt
│       └── Entities.kt
└── ui/
    ├── components/          # 再利用可能な UI コンポーネント
    ├── screens/             # 画面コンポーザブル
    │   ├── SessionListScreen.kt
    │   ├── SessionDetailScreen.kt
    │   └── SessionRunningScreen.kt
    └── theme/               # アプリテーマ定義
```

---

## ビルド手順

### デバッグビルド (Windows PowerShell)

```powershell
$env:JAVA_HOME = "C:\Program Files\Android\Android Studio\jbr"
.\gradlew.bat assembleDebug
```

出力先: `app/build/outputs/apk/debug/app-debug.apk`

### リリースビルド

1. `keystore.properties.sample` をコピーして `keystore.properties` を作成します。
2. 各フィールド（`storeFile`、`storePassword`、`keyAlias`、`keyPassword`）を実際のキーストア情報に書き換えます。

   > **注意**: `keystore.properties` は `.gitignore` 対象です。実際の鍵情報をリポジトリにコミットしないでください。

3. 以下のコマンドを実行します。

```powershell
$env:JAVA_HOME = "C:\Program Files\Android\Android Studio\jbr"

# Google Play 推奨 (AAB形式)
.\gradlew.bat bundleRelease

# または APK 形式
.\gradlew.bat assembleRelease
```

- AAB 出力先: `app/build/outputs/bundle/release/`
- APK 出力先: `app/build/outputs/apk/release/`

---

## バーチャルデバイス (Android Emulator) へのインストール

1. Android Studio → Device Manager で API 35 以上の AVD を作成・起動します。
2. `adb devices` でデバイスが認識されていることを確認します。
3. 以下のコマンドでインストールします。

   ```powershell
   $env:JAVA_HOME = "C:\Program Files\Android\Android Studio\jbr"
   .\gradlew.bat installDebug
   ```

4. または Android Studio の「Run」ボタン（▶）を使用して直接デプロイすることもできます。

---

## 実機へのインストール

1. 端末の「設定」→「開発者向けオプション」→「USB デバッグ」を有効化します。
2. USB ケーブルで PC に接続します。
3. `adb devices` でデバイスが認識されていることを確認します。
4. 以下のコマンドを実行します（JDK パスが設定済みの場合）。

   ```powershell
   .\gradlew.bat installDebug
   ```

---

## Google Play へのリリース手順

リリースは GitHub Actions による自動デプロイで行います。`v*.*.*` 形式のタグを push すると、以下のワークフローが自動実行されます。

1. Play Console から次の `versionCode` を自動取得
2. 署名済み AAB をビルド
3. Google Play の **内部テスト**トラックに自動アップロード

### 前提条件

以下の Secrets が GitHub リポジトリに設定されていること：

| Secret 名 | 内容 |
|-----------|------|
| `GOOGLE_PLAY_SERVICE_ACCOUNT_JSON` | Google Play サービスアカウントの JSON キー |
| `KEYSTORE_BASE64` | キーストアファイルを Base64 エンコードしたもの |
| `KEYSTORE_PASSWORD` | キーストアのパスワード |
| `KEY_ALIAS` | キーのエイリアス |
| `KEY_PASSWORD` | キーのパスワード |

### リリース手順

1. `app/build.gradle.kts` の `versionName` を更新してコミット・マージします（`versionCode` は CI が自動設定）。
2. リリース用タグを作成して push します。

   ```bash
   git tag v1.7.0
   git push origin v1.7.0
   ```

3. GitHub Actions の [Actions タブ](../../actions) でワークフロー「Deploy to Google Play Store」の実行を確認します。
4. 正常完了後、[Google Play Console](https://play.google.com/console) の内部テストトラックに AAB がアップロードされていることを確認します。
5. 内部テストで問題がなければ、Play Console 上で製品版などのトラックに昇格させます。

> **注意**: タグは `v1.0.0` のような `v*.*.*` 形式で作成してください。形式が異なるとワークフローが起動しません。
