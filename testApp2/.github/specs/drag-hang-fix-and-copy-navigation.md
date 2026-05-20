# 仕様ドキュメント: ドラッグハング修正 & コピー後ナビゲーション

作成日: 2026-05-21  
対象ファイル: `CategoryBrowserScreen.kt`, `SessionDetailScreen.kt`, `MainActivity.kt`

---

## 1. バグ1: ドラッグ&ドロップのハング

### 1.1 根本原因

**問題箇所**: `CategoryBrowserScreen.kt` — `LazyColumn` 内の各アイテムに直接 `pointerInput` を付与している箇所。

```kotlin
// カテゴリアイテム（L259付近）
.pointerInput(category.id) {
    detectDragGesturesAfterLongPress(...)
}

// セッションアイテム（L353付近）
.pointerInput(session.id) {
    detectDragGesturesAfterLongPress(...)
}
```

**ハングの発生メカニズム**:

1. `LazyColumn` は内部で `scrollable` モディファイアを保持しており、ポインタイベントを常時監視している。
2. ユーザーが長押しを開始すると、`detectDragGesturesAfterLongPress` が**ロングプレス認識待ち**のサスペンド状態に入る。
3. ロングプレスが認識されドラッグ開始の瞬間、**`LazyColumn` のスクロールジェスチャーハンドラーとアイテムの `pointerInput` ハンドラーの両方が同じポインタイベントを処理しようとして競合**する。
4. 一方が `consume()` してしまった場合、他方のコルーチンが**イベントを永遠に待ち続けるデッドロック状態**（= UIハング）に陥る。

**再現条件**: ロングプレスが認識されて「移動可能になった瞬間」そのままドラッグしようとしたとき。この瞬間がまさに2つのジェスチャーハンドラーの競合タイミングと一致する。

### 1.2 修正方針

**採用方針: 親コンテナ方式** — `pointerInput` を各アイテムから削除し、`LazyColumn` を包む親 `Box` に1つだけ配置する。

```
修正前:
Box (親)
  └── LazyColumn
        ├── Card(pointerInput)  ← 各アイテムにpointerInput
        └── Box(pointerInput)   ← 各アイテムにpointerInput

修正後:
Box (親, pointerInput をここに移動)
  └── LazyColumn (userScrollEnabled = !isDragging)
        ├── Card (onGloballyPositioned のみ)
        └── Box (onGloballyPositioned のみ)
```

**修正の詳細**:

#### (a) 各アイテムから `pointerInput` を削除

カテゴリアイテムの `Card` モディファイア、セッションアイテムの `Box` モディファイアから `.pointerInput(...)` ブロックをそれぞれ削除する。`onGloballyPositioned` による座標記録は**維持する**（ドラッグ対象特定に必要）。

#### (b) 親 `Box` に `pointerInput` を移動

`Scaffold` 内の `Box(modifier = Modifier.fillMaxSize().padding(innerPadding))` に対して、以下を追加する:

```kotlin
Box(
    modifier = Modifier
        .fillMaxSize()
        .padding(innerPadding)
        .pointerInput(Unit) {   // ← ここに移動
            detectDragGesturesAfterLongPress(
                onDragStart = { offset ->
                    // タッチ位置から対象アイテムを特定
                    val pos = offset  // ここは座標系に注意（後述）
                    // itemLayoutCoords を走査してどのアイテムが長押しされたか判定
                    val catEntry = itemLayoutCoords.entries
                        .filter { it.key.startsWith("cat_") }
                        .firstOrNull { (_, coords) -> coords.boundsInWindow().contains(pos) }
                    val sesEntry = itemLayoutCoords.entries
                        .filter { it.key.startsWith("ses_") }
                        .firstOrNull { (_, coords) -> coords.boundsInWindow().contains(pos) }

                    when {
                        catEntry != null -> {
                            val id = catEntry.key.removePrefix("cat_").toIntOrNull()
                            val cat = appState.getChildCategories(categoryId).find { it.id == id }
                            if (cat != null) {
                                draggingItem = DraggableItem.CategoryItem(cat)
                                dragOffset = catEntry.value.boundsInWindow().topLeft + offset
                            }
                        }
                        sesEntry != null -> {
                            val id = sesEntry.key.removePrefix("ses_").toIntOrNull()
                            val ses = appState.getSessionsByCategory(categoryId).find { it.id == id }
                            if (ses != null) {
                                draggingItem = DraggableItem.SessionItem(ses)
                                dragOffset = sesEntry.value.boundsInWindow().topLeft + offset
                            }
                        }
                    }
                },
                onDrag = { change, dragAmount ->
                    if (draggingItem != null) {
                        change.consume()
                        dragOffset += dragAmount
                        val dragCatId = (draggingItem as? DraggableItem.CategoryItem)?.category?.id
                        updateHoverState(dragOffset, dragCatId)
                    }
                },
                onDragEnd = {
                    if (draggingItem != null) {
                        handleDrop()
                        resetDragState()
                    }
                },
                onDragCancel = {
                    resetDragState()
                },
            )
        },
)
```

> **注意**: `pointerInput` ブロック内の `offset` は `PointerInputScope` のローカル座標。`boundsInWindow()` で取得した座標と同じ座標系であることを確認すること。必要に応じて `LocalView.current` 等でウィンドウ座標に変換する。

#### (c) `LazyColumn` に `userScrollEnabled = !isDragging` を追加

ドラッグ中はスクロールを無効化し、ポインタイベントが `LazyColumn` に吸収されないようにする:

```kotlin
LazyColumn(
    modifier = Modifier
        .fillMaxSize()
        .padding(horizontal = 16.dp, vertical = 8.dp),
    userScrollEnabled = !isDragging,  // ← 追加
)
```

#### (d) `pointerInput` のキーについて

親 `Box` の `pointerInput(Unit)` は一度しか起動しないが、ジェスチャーブロック内からは `itemLayoutCoords`（`HashMap` = 参照型）を直接参照するため、クロージャキャプチャの問題は発生しない。`draggingItem`、`dragOffset` 等の `MutableState` も同様に安全にアクセス可能。

### 1.3 修正の効果

- 各アイテムの `pointerInput` がなくなることで、`LazyColumn` のスクロールハンドラーとの競合が解消される
- ドラッグ認識は親コンテナが1箇所で担うため、ジェスチャーイベントの「奪い合い」が発生しない
- `userScrollEnabled = !isDragging` により、ドラッグ中の意図しないスクロールも防ぐ

---

## 2. 機能2: セッションコピー後のナビゲーション

### 2.1 現状の実装

**問題箇所**: `SessionDetailScreen.kt` — コピー後の処理がSnackbarのみ（L179〜L203）。

```kotlin
// 現状: コピー後はSnackbarを表示するだけ
val copiedSession = appState.copySessionWithDb(db, sessionId, newName)
if (copiedSession != null) {
    scopeSnackbar.launch {
        snackbarHostState.showSnackbar(
            message = "新しいセッション「${copiedSession.name}」を作成しました",
            duration = SnackbarDuration.Short
        )
    }
}
```

**問題点**:
- `SessionDetailScreen` にはナビゲーション用コールバックが `onStartSession` しか存在しない
- `copySessionWithDb()` は新しい `Session` オブジェクトを返しており、そのIDを使えばナビゲーション可能
- `MainActivity` で `SessionDetailScreen` を呼ぶ際にも `onNavigateToSession` コールバックが渡されていない

**関連する `AppState.copySessionWithDb` の戻り値**:

```kotlin
// models.kt L460〜L479
suspend fun copySessionWithDb(db: AppDatabase, sourceSessionId: Int, newName: String): Session?
// → 成功時: DB IDが反映された新しい Session オブジェクト
// → 失敗時: null
```

戻り値は利用可能な状態にある。

### 2.2 実装方針

#### (a) `SessionDetailScreen` にコールバックパラメータを追加

```kotlin
@Composable
fun SessionDetailScreen(
    modifier: Modifier = Modifier,
    appState: AppState,
    db: AppDatabase? = null,
    sessionId: Int,
    categoryId: Int? = null,
    onStartSession: (Int) -> Unit = {},
    onNavigateToSession: (Int) -> Unit = {},  // ← 追加
)
```

#### (b) コピー後に `onNavigateToSession` を呼び出す

```kotlin
// db あり
scopeCopy.launch {
    val copiedSession = appState.copySessionWithDb(db, sessionId, newName)
    if (copiedSession != null) {
        onNavigateToSession(copiedSession.id)  // ← Snackbarの代わりに遷移
    }
}

// db なし（フォールバック）
val copiedSession = appState.copySession(sessionId, newName)
if (copiedSession != null) {
    onNavigateToSession(copiedSession.id)  // ← 同様に遷移
}
```

Snackbarは不要になるため削除する。ナビゲーション自体がフィードバックになる。

#### (c) `MainActivity.kt` でコールバックを渡す

```kotlin
is Screen.SessionDetail -> {
    SessionDetailScreen(
        modifier = Modifier.padding(innerPadding),
        appState = appState,
        db = db,
        sessionId = screen.sessionId,
        onStartSession = { sid -> currentScreen = Screen.SessionRunning(sid) },
        onNavigateToSession = { sid -> currentScreen = Screen.SessionDetail(sid) },  // ← 追加
    )
}
```

---

## 3. 変更ファイル一覧

| ファイル | 変更種別 | 変更内容 |
|---------|---------|---------|
| `ui/screens/CategoryBrowserScreen.kt` | 修正 | 各アイテムの `pointerInput` を削除し親 `Box` に集約。`LazyColumn` に `userScrollEnabled = !isDragging` を追加。`onDragStart` でタッチ座標からアイテムを特定するロジックを実装 |
| `ui/screens/SessionDetailScreen.kt` | 修正 | `onNavigateToSession: (Int) -> Unit = {}` パラメータを追加。コピー後のSnackbarをナビゲーション呼び出しに置き換え |
| `MainActivity.kt` | 修正 | `SessionDetailScreen` の呼び出しに `onNavigateToSession` コールバックを追加 |

---

## 4. テスト観点

### バグ1 (ドラッグハング)

| テストケース | 期待動作 |
|------------|---------|
| カテゴリを長押し → 即座にドラッグ | ハングせずゴーストが表示されてドラッグ可能 |
| セッションを長押し → 即座にドラッグ | ハングせずゴーストが表示されてドラッグ可能 |
| ドラッグ中に指を動かす | `LazyColumn` がスクロールせず、ゴーストが追随 |
| ドラッグしてカテゴリ上でドロップ | アイテムが移動される |
| ドラッグしてルートゾーンでドロップ | アイテムがルートに移動される |
| 長押し → ドロップなしでキャンセル（指を離す） | 状態がリセットされ元の位置に戻る |
| ドラッグ中でないときに `LazyColumn` をスクロール | 通常通りスクロール可能 |

### 機能2 (コピー後ナビゲーション)

| テストケース | 期待動作 |
|------------|---------|
| 「同じメンバーで新しいセッションを作成」ボタンをタップ | コピーされたセッションの詳細画面に遷移する |
| コピー後の画面に表示されるセッション名 | 「元のセッション名 (コピー)」になっている |
| コピー後の画面に表示されるユーザー | コピー元と同じメンバー（スコアは0）が表示される |
| 戻るボタン/ジェスチャーを実行 | コピー元セッションの詳細画面に戻る（BackHandlerの動作確認） |
| db=null のフォールバックケース | メモリのみでコピーされた場合も同様に遷移する |

---

## 5. 補足事項・リスク

### バグ1 実装時の注意

- 親 `Box` の `pointerInput` に渡す offset の座標系を `boundsInWindow()` の座標系と統一することが重要。`detectDragGesturesAfterLongPress` の `onDragStart { offset }` は **PointerInputScope ローカル座標**であるため、`onGloballyPositioned` で取得した `LayoutCoordinates` の `boundsInWindow()` と直接比較するには `LocalView.current` 経由でウィンドウオフセットを加算する必要がある。または `positionInWindow()` を使う。
- `itemLayoutCoords` への書き込み（`onGloballyPositioned`）はコンポーズ/レイアウトフェーズで行われ、`pointerInput` のコールバックは Input フェーズで行われるため、スレッド的には安全。
- ドラッグ中に `LazyColumn` が再コンポーズされ、アイテムが再配置されても `itemLayoutCoords` が更新されるため問題ない。

### 機能2 実装時の注意

- `copySessionWithDb` は suspend 関数であり、コルーチン内で呼ぶ必要がある（現行通り）。
- `onNavigateToSession` は Compose の State 書き換えを行うため、メインスレッドから呼ぶ必要がある。`scopeCopy.launch { ... }` はデフォルトで Dispatchers.Main なので問題なし。
- `copySessionWithDb` 内で `persistNewSession` が DB 生成 ID をメモリに反映するまで完了を待ってから戻り値を返す設計になっているため、返ってきた `Session.id` は正式なDB IDである。
