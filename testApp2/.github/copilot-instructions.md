# プロジェクト仕様書 (Instructions)

## AIアシスタントへの指示
**重要**: このプロジェクトに関するすべての応答、コードのコメント、コミットメッセージ、およびドキュメント作成は、**日本語**で行ってください。

## プロジェクト概要
**得点集計 (TestApp2)** は、複数のユーザーのゲームセッションとスコアを管理するために設計された Android アプリケーションです。ユーザーはセッションの作成、プレイヤーの追加、経過時間の追跡、およびラウンドごとのスコア記録を行うことができます。アプリはローカルデータベースを使用してデータを永続化します。

## 技術スタック
- **言語**: Kotlin
- **UIフレームワーク**: Jetpack Compose (Material3)
- **データベース**: Room Database
- **ビルドシステム**: Gradle (Kotlin DSL)
- **最小 SDK**: 35
- **ターゲット SDK**: 36

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

## アーキテクチャ
このアプリケーションは、小規模から中規模のアプリに適したシンプルなアーキテクチャを採用しています：

1.  **UI レイヤー (Compose)**:
    -   画面は `AppState` の状態を監視します。
    -   ユーザーのアクションは `AppState` のメソッド、または `MainActivity` から渡されたコールバックに委譲されます。
    -   ナビゲーションは `MainActivity` 内で `currentScreen` 状態を使用して手動で処理されます。

2.  **状態管理 (`AppState`)**:
    -   UI の信頼できる唯一の情報源 (Single Source of Truth) として機能します。
    -   セッション、ユーザー、スコア記録の `mutableStateListOf` を保持し、UI の更新をトリガーします。
    -   ビジネスロジック（例：合計スコアの計算）を処理します。
    -   Room データベースと対話してデータの永続化を管理します。

3.  **データレイヤー (Room)**:
    -   `Entities.kt` はデータベーススキーマを定義します。
    -   `Dao.kt` はデータアクセスのための SQL クエリを提供します。
    -   `AppDatabase` はデータベースを初期化します。

## データモデル
-   **Session**: ゲームセッションを表します（ID、名前、経過時間）。
-   **User**: セッション内のプレイヤーを表します（ID、セッションID、名前、現在のスコア）。
-   **ScoreRecord**: 記録されたスコアのラウンドを表します（ID、セッションID、タイムスタンプ）。
-   **ScoreItem**: 特定のレコード内のユーザーごとの詳細スコア（DBに保存され、メモリ上でマップされます）。

## 主な機能
-   **セッション管理**: ゲームセッションの作成、一覧表示、削除。
-   **ユーザー管理**: 開始前にセッションにユーザーを追加。
-   **ゲーム実行**:
    -   リアルタイムの経過時間追跡。
    -   ユーザーごとのラウンドベースのスコア入力。
    -   履歴に基づいた合計スコアの自動計算。
    -   スコア履歴ログの表示。

## 開発ガイドライン
-   **状態更新**: UI に変更を即座に反映させるため、常に `AppState` の監視可能なリスト (observable lists) を更新してください。DB 操作はバックグラウンドで非同期に実行します。
-   **ナビゲーション**: 新しい画面を追加する場合は、`Screen` シールドクラス（`data/enums.kt` または `models.kt` 内）に追加し、`MainActivity` で処理してください。
-   **データベース**: スキーマを変更する場合は、`Entities.kt` と `Dao.kt` が更新されていることを確認してください。
-   **UI コンポーネント**: リストアイテムや一般的なウィジェットについては、`ui/components/` に再利用可能なコンポーネントを作成することを推奨します。
-   **ビルド実行**: フルビルドやリリースビルドなどの長時間かかるタスクを実行する場合は、タイムアウトを防ぐために**必ずバックグラウンド実行 (`isBackground: true`)** を使用してください。
    -   コマンド例: `$env:JAVA_HOME = "C:\Program Files\Android\Android Studio\jbr"; .\gradlew.bat assembleRelease`
    -   **絶対に**完了するまで待機してください。プロセスがハングしているように見えても、バックグラウンドで処理が続いている可能性があります。途中で中断したり、諦めたりすることは**禁止**です。出力が止まっているように見えても、完了メッセージが出るまで待ち続けてください。

## バージョン管理
アプリのバージョン情報は `app/build.gradle.kts` 内の `android { defaultConfig { ... } }` ブロックで定義されています。

-   **versionCode**: 整数値。Google Play ストアに新しいバージョンをアップロードするたびに、**必ず**以前の値より大きくする必要があります（例: 1 -> 2 -> 3）。
-   **versionName**: 文字列。ユーザーに表示されるバージョン番号です（例: "1.0", "1.1"）。

**更新タイミング**:
-   Google Play Console に新しい AAB/APK をアップロードする前。
-   `versionCode` が重複しているとアップロードエラーになります。

## ビルドと実行
### Windows (PowerShell)
Android Studioがインストールされている場合、同梱のJDKを使用してビルドできます。

```powershell
# JDKのパスを設定 (Android Studioのインストール先に合わせて調整してください)
$env:JAVA_HOME = "C:\Program Files\Android\Android Studio\jbr"

# デバッグビルドの作成
.\gradlew.bat assembleDebug

# デバイスへのインストール (デバイス接続時)
.\gradlew.bat installDebug
```

### リリースビルド
署名付きAPKまたはAABを作成するには、プロジェクトルートに `keystore.properties` を作成し、キーストア情報を設定してください。

1.  `keystore.properties.sample` をコピーして `keystore.properties` を作成。
2.  自身のキーストア情報に合わせて内容を編集。
3.  以下のコマンドを実行（**必ずバックグラウンド実行**）。

```powershell
# Google Play リリース用 (AAB形式 - 推奨)
.\gradlew.bat bundleRelease

# 配布用 (APK形式)
.\gradlew.bat assembleRelease
```

生成されたファイルは以下に出力されます：
-   AAB: `app/build/outputs/bundle/release/`
-   APK: `app/build/outputs/apk/release/`

```powershell
# リリースビルドの作成
.\gradlew.bat assembleRelease
```

生成されたAPKは `app/build/outputs/apk/release/` に出力されます。
`keystore.properties` がない場合は、署名なしAPK (`app-release-unsigned.apk`) が生成されます。

## 今後のタスク (推測)
-   データベース操作のエラーハンドリングの改善。
-   より詳細な統計やチャートの追加。
-   スコア入力の UI/UX の改善。
