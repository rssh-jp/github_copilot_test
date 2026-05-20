# 仕様ドキュメント: セッションコピーのカテゴリ引き継ぎ & ドラッグ＆ドロップ移動機能

**ブランチ**: `feature/session-copy-fix-and-drag-move`
**作成日**: 2026-05-21

---

## 修正1: セッションコピーのカテゴリ引き継ぎ

### 概要

セッションをコピーした際、コピー先セッションがコピー元と同じカテゴリに属するようにする。
現在は `AppState.copySession()` 内で `addSession(newName)` を categoryId なしで呼んでいるため、コピーされたセッションが常にルート（categoryId = null）に配置されてしまっている。

### 詳細要件

- コピー元セッションの `categoryId` をコピー先セッションにも引き継ぐ。
- コピー元が `categoryId = null`（ルート直下）の場合、コピー先もルート直下に作成する。
- セッション名は従来どおり `"${元の名前} (コピー)"` とする。
- DB（`sessions` テーブルの `categoryId` カラム）にも正しく保存される。

### 技術的方針

#### 変更箇所: `AppState.copySession()`

**ファイル**: `app/src/main/java/com/example/testapp2/data/models.kt`

```kotlin
// 修正前
fun copySession(sourceSessionId: Int, newName: String): Session? {
    val sourceSession = sessions.find { it.id == sourceSessionId } ?: return null
    val sourceUsers = getSessionUsers(sourceSessionId)
    val newSession = addSession(newName)  // ← categoryId が渡されていない
    ...
}

// 修正後
fun copySession(sourceSessionId: Int, newName: String): Session? {
    val sourceSession = sessions.find { it.id == sourceSessionId } ?: return null
    val sourceUsers = getSessionUsers(sourceSessionId)
    val newSession = addSession(newName, sourceSession.categoryId)  // ← categoryId を引き継ぐ
    ...
}
```

#### テスト観点（修正1）

| No. | テスト内容 | 期待結果 |
|-----|-----------|---------|
| 1 | カテゴリ配下のセッションをコピーする | コピー先が同じカテゴリに存在する |
| 2 | ルート直下のセッションをコピーする | コピー先がルート直下に存在する |
| 3 | コピー後に DB を再ロードする | `categoryId` が正しく永続化されている |

---

## 修正2: ドラッグ＆ドロップ移動機能

### 概要

`CategoryBrowserScreen` において、セッションまたはカテゴリを長押し（0.5秒）することで「移動モード」に入り、
ドラッグ操作で別のカテゴリへ移動できる機能を追加する。
カテゴリを移動する場合はサブツリーごと一括移動し、循環参照を防止する。

### UI/UX 設計

#### 操作フロー

```
[通常状態]
    ↓ アイテムを 500ms 長押し
[移動モード開始]
  - ドラッグ中のアイテムが半透明（alpha = 0.5）で指に追従して浮かび上がる
  - ドロップ可能なカテゴリがハイライト（primaryContainer 背景 + ボーダー）
  - ドロップ先として「ルート（カテゴリなし）」を示すゾーンをリスト上部に表示
    ↓ カテゴリ（またはルートゾーン）上でドロップ
[移動完了]
  - メモリ状態を即時更新
  - DB を非同期で更新
  - 移動モード終了
```

#### 移動モード中の視覚フィードバック

| 要素 | 通常時 | 移動モード時 |
|------|--------|-------------|
| ドラッグ中アイテム | 通常表示 | alpha = 0.5、Z 方向に浮き上がり（`zIndex = 1f`） |
| ドロップ可能カテゴリ | 通常背景 | `primaryContainer` 背景色 + 太めの `Border` |
| ドロップ不可アイテム（セッション） | 通常表示 | 変更なし（ドロップ対象外であることを示すため薄く表示も可） |
| ルートゾーン | 非表示 | リスト最上部に「ルートへ移動」バーを表示 |

#### 制約

- ドロップ先はカテゴリのみ（セッション上へのドロップは無効）。
- カテゴリを自分自身または子孫カテゴリへ移動することを禁止（循環参照防止）。
- ルート（`categoryId = null`）への移動も可能。
- 移動モード中は通常のタップ操作（画面遷移）を抑制する。

---

### Compose 実装方針

#### 状態管理

`CategoryBrowserScreen` 内のローカル状態として以下を追加する：

```kotlin
// ドラッグ中のアイテムを識別するシールドクラス
sealed class DragItem {
    data class SessionDrag(val sessionId: Int) : DragItem()
    data class CategoryDrag(val categoryId: Int) : DragItem()
}

var draggingItem by remember { mutableStateOf<DragItem?>(null) }
var dragOffset by remember { mutableStateOf(Offset.Zero) }
var hoveredCategoryId by remember { mutableStateOf<Int?>(null) }  // null = ルートゾーン上
var isHoveringRoot by remember { mutableStateOf(false) }
```

#### 長押し & ドラッグ検出

各アイテムに `Modifier.pointerInput` を付与し、`detectDragGesturesAfterLongPress` を利用する：

```kotlin
Modifier.pointerInput(item.id) {
    detectDragGesturesAfterLongPress(
        onDragStart = {
            draggingItem = DragItem.SessionDrag(session.id)  // または CategoryDrag
            dragOffset = Offset.Zero
        },
        onDrag = { change, dragAmount ->
            change.consume()
            dragOffset += dragAmount
            // ドラッグ座標からホバー対象カテゴリを特定（後述）
        },
        onDragEnd = {
            // ドロップ処理
            val targetCategoryId = if (isHoveringRoot) null else hoveredCategoryId
            if (targetCategoryId != INVALID_DROP) {
                scope.launch { /* moveSession / moveCategory を呼ぶ */ }
            }
            draggingItem = null
            dragOffset = Offset.Zero
        },
        onDragCancel = {
            draggingItem = null
            dragOffset = Offset.Zero
        }
    )
}
```

#### ホバー対象の特定

`LazyColumn` の各アイテムの座標を `onGloballyPositioned` で収集し、
ドラッグ中の絶対座標と比較して `hoveredCategoryId` を更新する。

```kotlin
// カテゴリカードの座標を保持するマップ
val categoryPositions = remember { mutableStateMapOf<Int, LayoutCoordinates>() }
val rootZonePosition = remember { mutableStateOf<LayoutCoordinates?>(null) }

// 各カテゴリカードに付与
Modifier.onGloballyPositioned { coords ->
    categoryPositions[category.id] = coords
}
```

ドラッグ中は `dragOffset` の絶対座標と各カテゴリの `boundsInWindow()` を比較して
ホバー対象を決定する。

#### フローティングドラッグプレビュー

`Box` を `fillMaxSize` で画面上にオーバーレイし、
`draggingItem != null` のとき絶対座標（`dragOffset` ベース）にアイテムのゴーストを描画する：

```kotlin
// CategoryBrowserScreen の最外 Box 内に重ねる
if (draggingItem != null) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .zIndex(10f)  // 最前面
    ) {
        // ドラッグアイテムのゴースト（半透明）を絶対座標に配置
        DragItemGhost(
            item = draggingItem!!,
            offset = dragOffset,
            alpha = 0.5f,
        )
    }
}
```

---

### AppState へのメソッド追加

#### `moveSession(sessionId: Int, newCategoryId: Int?)`

**ファイル**: `app/src/main/java/com/example/testapp2/data/models.kt`

```kotlin
/**
 * セッションを別のカテゴリへ移動（メモリのみ）
 * @param sessionId 移動対象セッションID
 * @param newCategoryId 移動先カテゴリID（null = ルート直下）
 */
fun moveSession(sessionId: Int, newCategoryId: Int?) {
    val index = sessions.indexOfFirst { it.id == sessionId }
    if (index >= 0) {
        sessions[index] = sessions[index].copy(categoryId = newCategoryId)
    }
}
```

#### `moveCategory(categoryId: Int, newParentId: Int?)`

**ファイル**: `app/src/main/java/com/example/testapp2/data/models.kt`

```kotlin
/**
 * カテゴリを別の親カテゴリへ移動（サブツリーごと。循環参照チェック済み）
 * @param categoryId 移動対象カテゴリID
 * @param newParentId 新しい親カテゴリID（null = ルート直下）
 * @return 移動成功時 true、循環参照の場合 false
 */
fun moveCategory(categoryId: Int, newParentId: Int?): Boolean {
    // 循環参照チェック：newParentId が categoryId の子孫でないことを確認
    if (newParentId != null && isDescendant(ancestorId = categoryId, targetId = newParentId)) {
        return false
    }
    // 自分自身への移動は禁止
    if (newParentId == categoryId) return false

    val index = categories.indexOfFirst { it.id == categoryId }
    if (index >= 0) {
        categories[index] = categories[index].copy(parentId = newParentId)
    }
    return true
}

/**
 * targetId が ancestorId の子孫であるかチェック（循環参照防止用）
 */
private fun isDescendant(ancestorId: Int, targetId: Int): Boolean {
    var current: Int? = targetId
    val visited = mutableSetOf<Int>()
    while (current != null) {
        if (current == ancestorId) return true
        if (!visited.add(current)) break  // 既訪問（安全策）
        current = categories.find { it.id == current }?.parentId
    }
    return false
}
```

---

### DB 更新方法

#### 追加するDAO クエリ

**ファイル**: `app/src/main/java/com/example/testapp2/data/db/Dao.kt`

```kotlin
// SessionDao に追加
@Query("UPDATE sessions SET categoryId = :categoryId WHERE id = :id")
suspend fun updateCategoryId(id: Int, categoryId: Int?)

// CategoryDao に追加
@Query("UPDATE categories SET parentId = :parentId WHERE id = :id")
suspend fun updateParentId(id: Int, parentId: Int?)
```

#### AppState への DB 永続化メソッド追加

**ファイル**: `app/src/main/java/com/example/testapp2/data/models.kt`

```kotlin
/**
 * セッションのカテゴリIDをDBに保存
 */
suspend fun persistMoveSession(db: AppDatabase, sessionId: Int, newCategoryId: Int?) {
    try {
        withContext(Dispatchers.IO) {
            db.sessionDao().updateCategoryId(sessionId, newCategoryId)
        }
    } catch (e: Exception) {
        Log.e("AppState", "セッション移動エラー: ${e.message}", e)
    }
}

/**
 * カテゴリの親IDをDBに保存
 */
suspend fun persistMoveCategory(db: AppDatabase, categoryId: Int, newParentId: Int?) {
    try {
        withContext(Dispatchers.IO) {
            db.categoryDao().updateParentId(categoryId, newParentId)
        }
    } catch (e: Exception) {
        Log.e("AppState", "カテゴリ移動エラー: ${e.message}", e)
    }
}
```

#### ドロップ時の呼び出しパターン（UI 側）

```kotlin
// セッションをドロップした場合
scope.launch {
    appState.moveSession(sessionId, targetCategoryId)
    if (db != null) appState.persistMoveSession(db, sessionId, targetCategoryId)
}

// カテゴリをドロップした場合
val success = appState.moveCategory(categoryId, targetCategoryId)
if (success && db != null) {
    scope.launch {
        appState.persistMoveCategory(db, categoryId, targetCategoryId)
    }
}
```

---

### 循環参照の防止

カテゴリを移動する際、以下のケースを禁止する：

| ケース | 説明 | 対処 |
|--------|------|------|
| 自分自身への移動 | `categoryId == newParentId` | `moveCategory` で false を返す |
| 子孫への移動 | 移動先が移動元の子孫カテゴリ | `isDescendant()` チェックで false を返す |

`isDescendant()` は `categories` リストを親方向に辿り、`ancestorId` に到達した場合 `true` を返す。
無限ループ防止のため `visited` セットを使用する。

UI 側ではドロップ時に `moveCategory` が `false` を返した場合、
Snackbar で「循環参照のため移動できません」とユーザーに通知する。

---

## マイグレーション戦略

本実装ではDBスキーマ変更は**不要**。

- `sessions.categoryId` カラムは既存（変更なし）
- `categories.parentId` カラムは既存（変更なし）
- 追加するのは DAO のクエリメソッドのみであり、スキーマの変更を伴わない

---

## 変更ファイル一覧

| ファイル | 変更種別 | 内容 |
|---------|---------|------|
| `app/src/main/java/com/example/testapp2/data/models.kt` | 修正 | `copySession()` に `sourceSession.categoryId` を渡す |
| `app/src/main/java/com/example/testapp2/data/models.kt` | 追加 | `moveSession()`, `moveCategory()`, `isDescendant()`, `persistMoveSession()`, `persistMoveCategory()` |
| `app/src/main/java/com/example/testapp2/data/db/Dao.kt` | 修正 | `SessionDao.updateCategoryId()`, `CategoryDao.updateParentId()` を追加 |
| `app/src/main/java/com/example/testapp2/ui/screens/CategoryBrowserScreen.kt` | 修正 | ドラッグ＆ドロップ UI 実装（長押し検出・ドラッグ状態管理・ドロップゾーン・フローティングプレビュー） |

---

## テスト観点（修正2）

### ユニットテスト（`app/src/test/`）

| No. | テスト対象 | 内容 | 期待結果 |
|-----|-----------|------|---------|
| 1 | `AppState.moveSession()` | 別カテゴリへ移動 | `sessions` の `categoryId` が更新される |
| 2 | `AppState.moveSession()` | ルートへ移動 | `categoryId = null` になる |
| 3 | `AppState.moveCategory()` | 別カテゴリへ移動 | `categories` の `parentId` が更新される |
| 4 | `AppState.moveCategory()` | 自分自身へ移動 | `false` を返す、状態変化なし |
| 5 | `AppState.moveCategory()` | 子孫カテゴリへ移動（循環参照） | `false` を返す、状態変化なし |
| 6 | `AppState.isDescendant()` | 直接の子 | `true` を返す |
| 7 | `AppState.isDescendant()` | 孫（多段）| `true` を返す |
| 8 | `AppState.isDescendant()` | 親（逆方向） | `false` を返す |

### インストゥルメンテーションテスト（`app/src/androidTest/`）

| No. | テスト内容 | 期待結果 |
|-----|-----------|---------|
| 1 | セッションを長押しして別カテゴリへドラッグ＆ドロップ | そのカテゴリ配下に移動する |
| 2 | カテゴリを長押しして別カテゴリへドラッグ＆ドロップ | サブツリーごと移動する |
| 3 | カテゴリをルートゾーンへドラッグ＆ドロップ | ルート直下に移動する |
| 4 | カテゴリを自分の子孫へドラッグ＆ドロップ | 移動されず Snackbar が表示される |

---

## 受け入れ条件チェックリスト

### 修正1
- [ ] セッションをコピーすると、同じカテゴリ配下に新セッションが表示される
- [ ] コピー後にアプリを再起動しても、カテゴリ配置が正しく維持される
- [ ] ルート直下のセッションをコピーした場合もルート直下に表示される

### 修正2
- [ ] アイテムを 0.5秒長押しすると移動モードに入る
- [ ] ドラッグ中、アイテムが半透明で指に追従する
- [ ] ドロップ可能なカテゴリがハイライト表示される
- [ ] ルートゾーンへのドロップでルート直下への移動ができる
- [ ] セッションをカテゴリへドロップすると移動が完了する
- [ ] カテゴリをドロップするとサブツリーごと移動する
- [ ] 循環参照となる移動は拒否され、Snackbar で通知される
- [ ] 移動後の状態がアプリ再起動後も正しく維持される
