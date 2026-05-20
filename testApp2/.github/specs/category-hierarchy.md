# 仕様ドキュメント: カテゴリ階層機能

**作成日**: 2026-05-20  
**ステータス**: ドラフト  
**対象バージョン**: versionCode 次回リリース

---

## 1. 機能概要

現在のフロー「セッション → セクション → スコア投入」を拡張し、セッション配下にカテゴリ（入れ子可能）を追加する。

**変更前のフロー**:
```
セッション → SessionRunning（スコア投入）
```

**変更後のフロー**:
```
セッション → CategoryBrowser（ルート）
           └─ カテゴリ（FOLDER）
              └─ カテゴリ（FOLDER）  ← 何階層でも入れ子可能
                 └─ セクション（SECTION）→ SessionRunning（スコア投入）
```

### 用語定義

| 用語 | 定義 |
|------|------|
| カテゴリ（FOLDER） | 子カテゴリまたはセクションを持てる「フォルダ」相当のノード |
| セクション（SECTION） | スコア投入の単位。子を持てないリーフノード（現在の SessionRunning 相当） |
| カテゴリツリー | セッション配下の全カテゴリ・セクションの階層構造 |

---

## 2. データモデル変更

### 2-1. 新規追加エンティティ: `CategoryEntity`

**ファイル**: `app/src/main/java/com/example/testapp2/data/db/Entities.kt`

```kotlin
@Entity(
    tableName = "categories",
    foreignKeys = [
        ForeignKey(
            entity = SessionEntity::class,
            parentColumns = ["id"],
            childColumns = ["sessionId"],
            onDelete = ForeignKey.CASCADE,
        ),
        ForeignKey(
            entity = CategoryEntity::class,
            parentColumns = ["id"],
            childColumns = ["parentId"],
            onDelete = ForeignKey.CASCADE,
        ),
    ],
    indices = [
        Index(value = ["sessionId"]),
        Index(value = ["parentId"]),
    ],
)
data class CategoryEntity(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    val sessionId: Int,
    val parentId: Int?,       // nullはルートカテゴリ（セッション直下）
    val name: String,
    val type: String,         // "FOLDER" または "SECTION"
    val sortOrder: Int = 0,
)
```

**設計上の注意**:
- `parentId` が `null` = セッション直下（ルート）のカテゴリ/セクション
- 自己参照外部キーは SQLite の `ON DELETE CASCADE` に対応しているが、Room では循環参照の自己参照 FK に制限がある。実装時は `onDelete = ForeignKey.CASCADE` が意図通り動作するか検証が必要
- `type` は文字列型（Room の `@TypeConverter` 不要）

### 2-2. 既存エンティティの変更: `ScoreRecordEntity`

**ファイル**: `app/src/main/java/com/example/testapp2/data/db/Entities.kt`

`sectionId` カラムを追加（nullable）:
```kotlin
data class ScoreRecordEntity(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    val sessionId: Int,
    val timestamp: Long,
    val sectionId: Int? = null,   // 追加: 紐づくセクション（SECTION型カテゴリ）のID
)
```

**後方互換性**: 既存レコードは `sectionId = null`（セクション未分類）として扱い、そのまま表示・編集を継続できる。

### 2-3. 新規ドメインモデル: `Category`

**ファイル**: `app/src/main/java/com/example/testapp2/data/models.kt`

```kotlin
/** カテゴリ種別 */
enum class CategoryType { FOLDER, SECTION }

/**
 * カテゴリ（フォルダまたはセクション）を表すデータクラス
 * @param id カテゴリの一意識別子
 * @param sessionId 所属するセッションのID
 * @param parentId 親カテゴリのID（nullはルート）
 * @param name カテゴリ名
 * @param type FOLDER（入れ子可能） または SECTION（スコア投入）
 * @param sortOrder 同階層内での表示順
 */
data class Category(
    val id: Int,
    val sessionId: Int,
    val parentId: Int?,
    val name: String,
    val type: CategoryType,
    val sortOrder: Int = 0,
)
```

### 2-4. 既存ドメインモデルの変更: `ScoreRecord`

**ファイル**: `app/src/main/java/com/example/testapp2/data/models.kt`

```kotlin
data class ScoreRecord(
    val id: Int,
    val sessionId: Int,
    val timestamp: Date,
    val scores: Map<Int, Int>,
    val sectionId: Int? = null,   // 追加: 紐づくセクションID
)
```

---

## 3. 画面フロー

```
SessionListScreen
    │  (セッション選択)
    ▼
SessionDetailScreen          ← 既存画面（ユーザー管理 + カテゴリ管理UIを追加）
    │  (「カテゴリを管理」または「開始」)
    ▼
CategoryBrowserScreen        ← 新規画面（parentCategoryId=nullでルート表示）
    │  (カテゴリをタップ)
    ▼
CategoryBrowserScreen        ← 再帰的に遷移（parentCategoryId = タップしたカテゴリID）
    │  (セクションをタップ)
    ▼
SessionRunningScreen         ← 既存画面（sectionId パラメータを追加）
```

**バック操作時の挙動**:
- `SessionRunning → CategoryBrowser(sectionId に対応する親 categoryId)`
- `CategoryBrowser(parentId≠null) → CategoryBrowser(そのparentIdの親)`
- `CategoryBrowser(parentId=null) → SessionDetail`
- `SessionDetail → SessionList`（既存と同じ）

---

## 4. UI 設計

### 4-1. CategoryBrowserScreen（新規）

**役割**: カテゴリ（FOLDER）またはセクション（SECTION）の一覧表示・追加・削除

**コンポーネント構成**:
- `TopAppBar`: 現在のカテゴリ名 or「カテゴリ」（ルート時）、戻るボタン
- パンくずリスト（Breadcrumb）: 現在の階層パスをテキストで表示（例: `大会 > 予選 > Aブロック`）
- `LazyColumn`: 子カテゴリ一覧
  - カテゴリ（FOLDER）: フォルダアイコン + 名前 → タップで下階層へ遷移
  - セクション（SECTION）: プレイアイコン + 名前 → タップで `SessionRunningScreen` へ遷移
  - 各アイテムにスワイプ削除 or 削除ボタン（確認ダイアログあり）
- `FloatingActionButton`:「＋」ボタン → カテゴリ/セクション追加ダイアログを開く

**追加ダイアログ**:
- 入力: カテゴリ名テキストフィールド
- 種別選択: ラジオボタン「フォルダ（さらに分類可能）」「セクション（スコアを入力）」
- 追加 / キャンセルボタン

**CategoryBrowserItem コンポーネント** (再利用可能):
- `ui/components/CategoryItem.kt` として実装
- `category: Category`, `onClick: () -> Unit`, `onDelete: () -> Unit` を受け取る

### 4-2. SessionDetailScreen（既存画面の変更）

**変更内容**:
- 「開始」ボタンを削除（カテゴリ/セクション経由でのみスコア投入可能）  
  ※ 移行期間として「カテゴリなしで直接開始」ボタンを残す場合は仕様を再検討すること
- 「カテゴリを管理」ボタン（または「スコアを入力」ボタン）を追加 → `CategoryBrowserScreen` へ遷移
- カテゴリツリーの簡易サマリー（ルート直下のカテゴリ数 + セクション数）を表示

**変更しない内容**:
- ユーザー管理（追加・削除）機能は変更なし
- セッション名編集機能は変更なし

### 4-3. SessionRunningScreen（既存画面の変更）

**変更内容**:
- 引数に `sectionId: Int?` を追加
- TopAppBar に現在のセクション名を表示（`sectionId` から `Category.name` を参照）
- スコア記録時に `sectionId` を `addScoreRecord` に渡す

**変更しない内容**:
- タイマー機能、スコア入力 UI、スコア履歴表示は変更なし

---

## 5. AppState の変更点

**ファイル**: `app/src/main/java/com/example/testapp2/data/models.kt`

### 5-1. 追加するフィールド

```kotlin
/** カテゴリ一覧（全セッションのカテゴリ・セクションを保持） */
val categories = mutableStateListOf<Category>()
```

### 5-2. 追加するメソッド

```kotlin
/** 指定親配下の直接子カテゴリ一覧を取得（sortOrder順） */
fun getChildCategories(sessionId: Int, parentId: Int?): List<Category>

/** カテゴリを追加してメモリに反映 */
fun addCategory(sessionId: Int, parentId: Int?, name: String, type: CategoryType): Category

/** カテゴリをメモリから削除（子孫も含む再帰的削除） */
fun deleteCategoryLocal(categoryId: Int)

/** カテゴリ名を更新 */
fun updateCategory(categoryId: Int, newName: String)

/** DBからカテゴリを永続化 */
suspend fun persistNewCategory(db: AppDatabase, category: Category): Int

/** DBのカテゴリを削除 */
suspend fun persistDeleteCategory(db: AppDatabase, categoryId: Int)

/** DBのカテゴリ名を更新 */
suspend fun persistUpdateCategoryName(db: AppDatabase, categoryId: Int, name: String)
```

### 5-3. 変更するメソッド

```kotlin
// addScoreRecord: sectionId パラメータを追加
fun addScoreRecord(sessionId: Int, userScores: Map<Int, Int>, sectionId: Int? = null): Int

// persistNewScoreRecord: sectionId パラメータを追加
suspend fun persistNewScoreRecord(db: AppDatabase, sessionId: Int, recordId: Int, 
                                   userScores: Map<Int, Int>, timestamp: Long, sectionId: Int? = null)

// loadFromDb: カテゴリ読み込みを追加
suspend fun loadFromDb(db: AppDatabase)
// → categories の読み込みロジックを追加
// → ScoreRecord の sectionId マッピングを追加

// deleteSessionLocal: カテゴリの削除も追加
fun deleteSessionLocal(sessionId: Int)
// → categories.removeAll { it.sessionId == sessionId } を追加
```

### 5-4. recalcSessionTotals の変更

現在はセッション全体でスコアを集計しているが、今後はセクション別の集計も可能にする。
`sectionId` フィルタ付きのオーバーロードを追加するかどうかは実装フェーズで決定する（今回は全セッション集計を維持し、セクション別は将来対応とする）。

---

## 6. DB マイグレーション方針

### 6-1. バージョンアップ

- **変更前**: `version = 1`
- **変更後**: `version = 2`

### 6-2. Migration(1, 2) の実装内容

**ファイル**: `app/src/main/java/com/example/testapp2/data/db/AppDatabase.kt` に Migration オブジェクトを定義

```kotlin
val MIGRATION_1_2 = object : Migration(1, 2) {
    override fun migrate(database: SupportSQLiteDatabase) {
        // 1. categories テーブルを新規作成
        database.execSQL("""
            CREATE TABLE IF NOT EXISTS `categories` (
                `id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
                `sessionId` INTEGER NOT NULL,
                `parentId` INTEGER,
                `name` TEXT NOT NULL,
                `type` TEXT NOT NULL DEFAULT 'FOLDER',
                `sortOrder` INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY(`sessionId`) REFERENCES `sessions`(`id`) ON DELETE CASCADE,
                FOREIGN KEY(`parentId`) REFERENCES `categories`(`id`) ON DELETE CASCADE
            )
        """.trimIndent())
        database.execSQL("CREATE INDEX IF NOT EXISTS `index_categories_sessionId` ON `categories` (`sessionId`)")
        database.execSQL("CREATE INDEX IF NOT EXISTS `index_categories_parentId` ON `categories` (`parentId`)")

        // 2. score_records テーブルに sectionId カラムを追加（nullable、既存データは NULL）
        database.execSQL("ALTER TABLE `score_records` ADD COLUMN `sectionId` INTEGER")
    }
}
```

**AppDatabase に Migration を登録**:

```kotlin
Room.databaseBuilder(...)
    .addMigrations(MIGRATION_1_2)
    .build()
```

### 6-3. fallbackToDestructiveMigration は使用しない

既存ユーザーのデータを保護するため、`fallbackToDestructiveMigration()` は使用しない。
マイグレーションの実装後、`exportSchema = true` に変更してスキーマ JSON を記録することを推奨する。

### 6-4. 後方互換性

- 既存の `score_records` レコードは `sectionId = null` として扱われ、セクション未分類として表示される
- 既存データの動作は変更されない

---

## 7. ナビゲーション変更点

### 7-1. Screen sealed class の変更

**ファイル**: `app/src/main/java/com/example/testapp2/data/enums.kt`

```kotlin
sealed class Screen {
    object SessionList : Screen()
    data class SessionDetail(val sessionId: Int) : Screen()
    // 新規追加: カテゴリ一覧画面
    data class CategoryBrowser(
        val sessionId: Int,
        val parentCategoryId: Int?,  // null = セッション直下（ルート）
    ) : Screen()
    data class SessionRunning(
        val sessionId: Int,
        val sectionId: Int? = null,  // 変更: sectionId を追加
    ) : Screen()
}
```

### 7-2. MainActivity の変更

**ファイル**: `app/src/main/java/com/example/testapp2/MainActivity.kt`

**追加・変更が必要な箇所**:

1. **TopAppBar のタイトル表示** (`when (currentScreen)` ブロック):
   - `Screen.CategoryBrowser` ケースを追加（カテゴリ名または「カテゴリ」を表示）

2. **BackHandler の戻り遷移** (`when (val screen = currentScreen)` ブロック):
   - `Screen.CategoryBrowser(parentId≠null)` → 親カテゴリの `CategoryBrowser` へ
   - `Screen.CategoryBrowser(parentId=null)` → `SessionDetail(sessionId)` へ
   - `Screen.SessionRunning(sectionId≠null)` → `CategoryBrowser(sessionId, sectionId の parentId)` へ
   - `Screen.SessionRunning(sectionId=null)` → `SessionDetail(sessionId)`（後方互換）

3. **画面表示 (`when (currentScreen)` の `Scaffold` 内)**:
   - `Screen.CategoryBrowser` → `CategoryBrowserScreen(...)` を追加
   - `Screen.SessionRunning` に `sectionId` を渡す

4. **ナビゲーションコールバック**:
   - `navigateToCategoryBrowser: (sessionId: Int, parentCategoryId: Int?) -> Unit` を追加
   - `navigateToSessionRunning: (sessionId: Int, sectionId: Int?) -> Unit` を追加

### 7-3. TopAppBar 戻るボタンの対応

- `Screen.CategoryBrowser` でも戻るボタンを表示する
- 戻り先は BackHandler と同じロジックを共用（関数化を推奨）

---

## 8. 新規追加ファイル一覧

| ファイルパス | 種別 | 説明 |
|------------|------|------|
| `ui/screens/CategoryBrowserScreen.kt` | 新規 | カテゴリ/セクション一覧画面 |
| `ui/components/CategoryItem.kt` | 新規 | カテゴリリストアイテムコンポーネント |

---

## 9. 変更ファイル一覧

| ファイルパス | 変更内容 |
|------------|---------|
| `data/enums.kt` | `Screen` に `CategoryBrowser` 追加、`SessionRunning` に `sectionId` 追加 |
| `data/models.kt` | `Category` / `CategoryType` 追加、`ScoreRecord` に `sectionId` 追加、`AppState` のメソッド追加・変更 |
| `data/db/Entities.kt` | `CategoryEntity` 追加、`ScoreRecordEntity` に `sectionId` 追加 |
| `data/db/Dao.kt` | `CategoryDao` 追加（CRUD クエリ） |
| `data/db/AppDatabase.kt` | `CategoryDao` 登録、`version = 2`、`MIGRATION_1_2` 追加 |
| `MainActivity.kt` | `CategoryBrowserScreen` のルーティング追加、BackHandler 変更 |
| `ui/screens/SessionDetailScreen.kt` | 「カテゴリを管理」ボタン追加、「開始」ボタンのフロー変更 |
| `ui/screens/SessionRunningScreen.kt` | `sectionId` パラメータ追加、スコア記録時に `sectionId` を渡す |

---

## 10. テスト観点

### ユニットテスト（`app/src/test/`）

| テスト対象 | テスト内容 |
|-----------|-----------|
| `AppState.addCategory` | ルートカテゴリ追加、子カテゴリ追加、メモリへの即時反映 |
| `AppState.deleteCategoryLocal` | カテゴリ削除時に子孫カテゴリも再帰的に削除されること |
| `AppState.getChildCategories` | 指定した parentId 配下の直接子のみ返すこと |
| `AppState.addScoreRecord` | `sectionId` が正しく `ScoreRecord` に反映されること |
| `AppState.loadFromDb` | カテゴリ・sectionId 付きスコアが正しくメモリに復元されること |

### インストゥルメンテーションテスト（`app/src/androidTest/`）

| テスト対象 | テスト内容 |
|-----------|-----------|
| DB Migration 1→2 | マイグレーション後に categories テーブルが存在すること |
| DB Migration 1→2 | 既存の score_records の sectionId が NULL であること |
| `CategoryBrowserScreen` | カテゴリ追加ダイアログ表示・追加・削除の動作確認 |
| `CategoryBrowserScreen` | セクションをタップすると SessionRunningScreen へ遷移すること |
| BackHandler | CategoryBrowser → SessionDetail への戻り遷移 |
| BackHandler | 深い階層での連続「戻る」操作 |

---

## 11. 未決事項・将来対応

| 課題 | 説明 |
|------|------|
| セクション別スコア集計 | 現在は全セッション合計のみ。カテゴリ/セクション別集計は将来対応 |
| カテゴリの並び替え | `sortOrder` フィールドは用意するが、ドラッグ並び替え UI は将来対応 |
| カテゴリのコピー | `copySession` 同様にカテゴリツリーごとコピーする機能は将来対応 |
| 移行データ対応 | 既存セッションにはカテゴリがない状態になる。「カテゴリなしで直接スコア入力」フロー（既存フロー）を維持するか廃止するかは要検討 |
