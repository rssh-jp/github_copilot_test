# 仕様ドキュメント: カテゴリ階層によるセッション管理

**ブランチ**: `feature/category-session-hierarchy`  
**作成日**: 2026-05-20  
**ステータス**: ドラフト

---

## 1. 機能概要

「カテゴリ」という概念を導入し、セッション（ゲーム）をカテゴリで階層的に整理できるようにする。カテゴリはカテゴリを内包できる（ツリー構造）。ユーザーはまずカテゴリをブラウズし、目的のカテゴリ内にあるセッションを選択してゲームを開始する。

---

## 2. 新しいアプリフロー（画面遷移）

```
アプリ起動
    └─ CategoryBrowserScreen (root: categoryId = null)
           ├─ カテゴリ項目タップ
           │      └─ CategoryBrowserScreen (categoryId = 選択カテゴリのID)
           │               └─ （さらに入れ子可能）
           │                      └─ セッション項目タップ
           │                             └─ SessionDetailScreen
           │                                    └─ SessionRunningScreen
           └─ セッション項目タップ
                  └─ SessionDetailScreen
                         └─ SessionRunningScreen
```

### 戻るナビゲーション

| 現在の画面 | 戻り先 |
|---|---|
| `CategoryBrowserScreen(categoryId != null)` | 親カテゴリの `CategoryBrowserScreen`（`Category.parentId` を参照） |
| `CategoryBrowserScreen(categoryId = null)` | アプリ終了（BackHandler 無効） |
| `SessionDetailScreen` | 直前の `CategoryBrowserScreen` |
| `SessionRunningScreen` | `SessionDetailScreen` |

---

## 3. データモデル変更

### 3-1. 新規: `Category` ドメインモデル（`data/models.kt`）

```kotlin
data class Category(
    val id: Int,
    val parentId: Int?,   // null = ルート直下
    val name: String,
    val sortOrder: Int,
)
```

### 3-2. 変更: `Session` ドメインモデル（`data/models.kt`）

`categoryId: Int?` フィールドを追加する。

```kotlin
// 変更前
data class Session(
    val id: Int,
    val name: String,
    val elapsedTime: Int,
)

// 変更後
data class Session(
    val id: Int,
    val name: String,
    val elapsedTime: Int,
    val categoryId: Int?,   // 所属カテゴリID。null = ルート直下
)
```

### 3-3. 新規: `CategoryEntity`（`data/db/Entities.kt`）

```kotlin
@Entity(
    tableName = "categories",
    foreignKeys = [
        ForeignKey(
            entity = CategoryEntity::class,
            parentColumns = ["id"],
            childColumns = ["parentId"],
            onDelete = ForeignKey.CASCADE,
        )
    ],
    indices = [Index(value = ["parentId"])],
)
data class CategoryEntity(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    val parentId: Int?,
    val name: String,
    val sortOrder: Int,
)
```

### 3-4. 変更: `SessionEntity`（`data/db/Entities.kt`）

`categoryId: Int?` カラムを追加する。

```kotlin
// 変更後
@Entity(tableName = "sessions")
data class SessionEntity(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    val name: String,
    val elapsedTime: Int,
    val categoryId: Int?,   // 追加
)
```

### 3-5. DBバージョン

| 項目 | 値 |
|---|---|
| 変更前バージョン | `1` |
| 変更後バージョン | `2` |

---

## 4. DBマイグレーション方針

### マイグレーション: version 1 → 2

`AppDatabase.kt` に `Migration(1, 2)` を定義し、`.addMigrations(MIGRATION_1_2)` で登録する。

```
実行するSQL:
  1. CREATE TABLE categories (
         id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
         parentId INTEGER,
         name TEXT NOT NULL,
         sortOrder INTEGER NOT NULL,
         FOREIGN KEY (parentId) REFERENCES categories(id) ON DELETE CASCADE
     );

  2. ALTER TABLE sessions ADD COLUMN categoryId INTEGER;
     -- categoryId は NULL を許容（既存レコードは自動的に NULL になるため追加対応不要）
```

**既存データの扱い**:  
既存セッションはすべて `categoryId = NULL`（ルート直下）として保持される。データ損失なし。

---

## 5. 新規 DAO（`data/db/Dao.kt`）

### `CategoryDao`

| メソッド | SQL / 概要 |
|---|---|
| `insert(category: CategoryEntity): Long` | INSERT |
| `getAll(): List<CategoryEntity>` | SELECT * FROM categories |
| `getByParent(parentId: Int?): List<CategoryEntity>` | WHERE parentId = :parentId (nullable対応) |
| `getRoots(): List<CategoryEntity>` | WHERE parentId IS NULL |
| `updateName(id: Int, name: String)` | UPDATE |
| `updateSortOrder(id: Int, sortOrder: Int)` | UPDATE |
| `deleteById(id: Int)` | DELETE（子カテゴリは CASCADE で連鎖削除） |

### `SessionDao` の変更

`insert` メソッドが受け取る `SessionEntity` に `categoryId` が追加されるため、DAO 自体の変更は最小限（`getAll()` はそのまま使用可能）。  
カテゴリ別取得用クエリを追加する。

```kotlin
@Query("SELECT * FROM sessions WHERE categoryId = :categoryId")
suspend fun getByCategory(categoryId: Int): List<SessionEntity>

@Query("SELECT * FROM sessions WHERE categoryId IS NULL")
suspend fun getRootSessions(): List<SessionEntity>
```

---

## 6. `AppState` の変更点（`data/models.kt`）

### 追加するフィールド

```kotlin
val categories = mutableStateListOf<Category>()
```

### 追加するメソッド

| メソッド | 概要 |
|---|---|
| `getChildCategories(parentId: Int?): List<Category>` | 指定親IDの直下カテゴリ一覧を返す |
| `getSessionsByCategory(categoryId: Int?): List<Session>` | 指定カテゴリに属するセッション一覧を返す |
| `addCategory(parentId: Int?, name: String, sortOrder: Int): Category` | カテゴリをメモリに追加 |
| `deleteCategory(categoryId: Int)` | カテゴリをメモリから削除（子カテゴリ・セッションも再帰削除） |
| `persistNewCategory(db, category: Category): Int` | カテゴリをDBに保存 |
| `persistDeleteCategory(db, categoryId: Int)` | カテゴリをDBから削除 |
| `persistUpdateCategoryName(db, categoryId: Int, name: String)` | カテゴリ名をDBに更新 |

### 変更するメソッド

| メソッド | 変更内容 |
|---|---|
| `addSession(name: String, categoryId: Int?): Session` | `categoryId` 引数を追加 |
| `loadFromDb(db)` | `CategoryEntity` をロードして `categories` に追加する処理を追加。`SessionEntity` の `categoryId` を `Session` にマッピング |
| `persistNewSession(db, session: Session)` | `SessionEntity` の `categoryId` を含めて保存 |
| `deleteCategory(categoryId: Int)` | 子カテゴリの再帰削除、および所属セッションの削除を連動（DBは CASCADE で対応） |

---

## 7. ナビゲーション変更（`data/enums.kt`）

### `Screen` sealed class の変更

```kotlin
// 変更前
sealed class Screen {
    object SessionList : Screen()
    data class SessionDetail(val sessionId: Int) : Screen()
    data class SessionRunning(val sessionId: Int) : Screen()
}

// 変更後
sealed class Screen {
    // CategoryBrowserScreen のルート（categoryId = null）
    data class CategoryBrowser(val categoryId: Int?) : Screen()
    data class SessionDetail(val sessionId: Int) : Screen()
    data class SessionRunning(val sessionId: Int) : Screen()
}
```

`Screen.SessionList` は削除し、`Screen.CategoryBrowser(null)` を初期画面とする。

### `MenuType` の変更（`data/enums.kt`）

```kotlin
// 変更前
enum class MenuType(val title: String) {
    SESSION_LIST("一覧")
}

// 変更後
enum class MenuType(val title: String) {
    CATEGORY_BROWSER("ブラウザ")
}
```

---

## 8. 画面設計

### 8-1. 新規: `CategoryBrowserScreen`（`ui/screens/CategoryBrowserScreen.kt`）

**引数**:

| 引数 | 型 | 説明 |
|---|---|---|
| `modifier` | `Modifier` | レイアウト修飾子 |
| `appState` | `AppState` | アプリ状態 |
| `db` | `AppDatabase?` | DBインスタンス |
| `categoryId` | `Int?` | 表示するカテゴリのID（null = ルート） |
| `onNavigateToCategory` | `(Int) -> Unit` | 子カテゴリへの遷移コールバック |
| `onNavigateToSession` | `(Int) -> Unit` | セッション詳細への遷移コールバック |

**表示内容**:

- パンくずリスト（現在の階層パス表示、オプション）
- 子カテゴリ一覧（`LazyColumn`）
  - 各アイテム: フォルダアイコン + カテゴリ名 + 子の数バッジ
  - タップ → `onNavigateToCategory(category.id)`
  - 長押し or スワイプ: 削除確認ダイアログ
- セッション一覧（子カテゴリの下に表示）
  - 各アイテム: 既存 `SessionItem` コンポーネントを流用
  - タップ → `onNavigateToSession(session.id)`
- FAB（右下）: タップで「カテゴリを追加」「セッションを追加」の2択モーダルを表示

**追加モーダルの UI 案**:

```
[追加] ダイアログ（AlertDialog or BottomSheet）
  ◉ カテゴリを追加
  ○ セッションを追加
  [名前入力フィールド]
  [作成] [キャンセル]
```

### 8-2. 変更: `SessionListScreen`（`ui/screens/SessionListScreen.kt`）

`CategoryBrowserScreen` へ置き換えるため、**このファイルを削除する**。  
既存の `SessionItem` コンポーネント（`ui/components/SessionItem.kt`）は `CategoryBrowserScreen` から引き続き利用する。

### 8-3. 変更: `SessionDetailScreen`

- `sessionId: Int` は変更なし。
- 「カテゴリに属する」情報をサブタイトルとして表示する（オプション表示）。
- セッション作成時に `categoryId` を渡すよう変更する。

---

## 9. `MainActivity.kt` の変更点

### 初期画面

```kotlin
// 変更前
var currentScreen by remember { mutableStateOf<Screen>(Screen.SessionList) }

// 変更後
var currentScreen by remember { mutableStateOf<Screen>(Screen.CategoryBrowser(null)) }
```

### `BackHandler`

カテゴリ階層を遡るため、`Screen.CategoryBrowser` の場合は `categoryId` から親を参照して遷移する。

```kotlin
BackHandler(enabled = currentScreen !is Screen.CategoryBrowser || (currentScreen as Screen.CategoryBrowser).categoryId != null) {
    currentScreen = when (val screen = currentScreen) {
        is Screen.SessionDetail  -> {
            // セッションが属するカテゴリへ戻る
            val session = appState.sessions.find { it.id == screen.sessionId }
            Screen.CategoryBrowser(session?.categoryId)
        }
        is Screen.SessionRunning -> Screen.SessionDetail(screen.sessionId)
        is Screen.CategoryBrowser -> {
            // 親カテゴリへ戻る
            val parentId = appState.categories.find { it.id == screen.categoryId }?.parentId
            Screen.CategoryBrowser(parentId)
        }
    }
}
```

### `TopAppBar` タイトル

```kotlin
is Screen.CategoryBrowser -> {
    val name = appState.categories.find { it.id == (currentScreen as Screen.CategoryBrowser).categoryId }?.name
    name ?: "ブラウザ"  // ルート時は "ブラウザ"
}
```

### 画面ルーティング追加

```kotlin
is Screen.CategoryBrowser -> {
    val categoryId = (currentScreen as Screen.CategoryBrowser).categoryId
    CategoryBrowserScreen(
        modifier = Modifier.padding(innerPadding),
        appState = appState,
        db = db,
        categoryId = categoryId,
        onNavigateToCategory = { id -> currentScreen = Screen.CategoryBrowser(id) },
        onNavigateToSession = { sid -> currentScreen = Screen.SessionDetail(sid) }
    )
}
```

### メニュードロワー

`MenuType.SESSION_LIST` → `MenuType.CATEGORY_BROWSER` に変更し、対応する画面遷移を `Screen.CategoryBrowser(null)` に変更する。

---

## 10. 変更・追加ファイル一覧

| ファイル | 操作 | 変更概要 |
|---|---|---|
| `data/models.kt` | 変更 | `Category` モデル追加、`Session` に `categoryId` 追加、`AppState` にカテゴリ関連メソッド追加 |
| `data/enums.kt` | 変更 | `Screen.SessionList` 削除、`Screen.CategoryBrowser(categoryId: Int?)` 追加、`MenuType` 変更 |
| `data/db/Entities.kt` | 変更 | `CategoryEntity` 追加、`SessionEntity` に `categoryId` 追加 |
| `data/db/Dao.kt` | 変更 | `CategoryDao` 追加、`SessionDao` にカテゴリ別クエリ追加 |
| `data/db/AppDatabase.kt` | 変更 | `version = 2`、`CategoryEntity` 追加、`Migration(1, 2)` 定義・登録、`categoryDao()` 追加 |
| `MainActivity.kt` | 変更 | 初期画面変更、`BackHandler` 更新、画面ルーティング更新、メニュー変更 |
| `ui/screens/CategoryBrowserScreen.kt` | **新規** | カテゴリ＋セッション一覧表示、追加モーダル、削除確認ダイアログ |
| `ui/screens/SessionListScreen.kt` | **削除** | `CategoryBrowserScreen` に置き換え |
| `ui/screens/SessionDetailScreen.kt` | 変更 | セッション作成時に `categoryId` を渡すよう修正 |

---

## 11. テスト観点

### ユニットテスト（`app/src/test/`）

| テスト対象 | テスト内容 |
|---|---|
| `AppState.getChildCategories(parentId)` | parentId=null でルートカテゴリのみ返す／parentId指定で正しい子を返す |
| `AppState.getSessionsByCategory(categoryId)` | categoryId=null でルートセッションのみ返す／指定IDで絞り込まれる |
| `AppState.addCategory(...)` | ID が自動採番されリストに追加される |
| `AppState.deleteCategory(categoryId)` | 子カテゴリ・所属セッションがメモリから連動削除される |
| `AppState.addSession(name, categoryId)` | categoryId が Session に正しく設定される |

### インストゥルメンテーションテスト（`app/src/androidTest/`）

| テスト対象 | テスト内容 |
|---|---|
| DBマイグレーション 1→2 | 既存セッションの `categoryId` が NULL になる |
| DBマイグレーション 1→2 | `categories` テーブルが存在し挿入・取得できる |
| `CategoryBrowserScreen` | ルート画面にカテゴリが表示される |
| `CategoryBrowserScreen` | カテゴリタップで子カテゴリ画面へ遷移する |
| `CategoryBrowserScreen` | FAB タップで追加ダイアログが表示される |
| `CategoryBrowserScreen` | カテゴリ追加後にリストに反映される |
| `CategoryBrowserScreen` | セッション追加後にリストに反映される |
| ナビゲーション | カテゴリ階層を深く遷移し、戻るで親に戻れる |
| ナビゲーション | セッション詳細から戻ると所属カテゴリ画面に戻る |

---

## 12. 実装上の注意事項

1. **自己参照 FK の Room 対応**: `CategoryEntity` の `parentId` は `ForeignKey` で自テーブルを参照するが、Room は自己参照をサポートする。ただし `onDelete = CASCADE` を設定することで子カテゴリの連鎖削除を DB 側で保証する。

2. **AppState の再帰削除**: DB の CASCADE に頼るが、メモリ上（`AppState`）でも `deleteCategory()` 時に子カテゴリ・所属セッションを再帰的に削除する必要がある。再帰処理は `getChildCategories()` を繰り返し呼んで実装する。

3. **sortOrder の初期値**: 新規追加時は `(同じ parentId の最大 sortOrder) + 1` として設定する。

4. **`CategoryBrowserScreen` の BackHandler との分離**: 画面スタックは `MainActivity` が管理するため、`CategoryBrowserScreen` 内では戻るボタンを持たない。`BackHandler` は `MainActivity` に一元化する。

5. **DBビルダーの `fallbackToDestructiveMigration` 禁止**: マイグレーション未定義時のデータ消失を避けるため、`Migration(1, 2)` を必ず定義・登録してから `addMigrations()` を使用する。
