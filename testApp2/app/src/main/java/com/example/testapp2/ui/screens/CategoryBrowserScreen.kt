package com.example.testapp2.ui.screens

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectDragGesturesAfterLongPress
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.LayoutCoordinates
import androidx.compose.ui.layout.boundsInWindow
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.dp
import androidx.compose.ui.zIndex
import com.example.testapp2.data.AppState
import com.example.testapp2.data.Category
import com.example.testapp2.data.Session
import com.example.testapp2.data.db.AppDatabase
import com.example.testapp2.ui.components.SessionItem
import kotlinx.coroutines.launch

/** 追加ダイアログで選択するモード */
private enum class AddMode { CATEGORY, SESSION }

/**
 * ドラッグ可能なアイテムを識別するシールドクラス
 */
sealed class DraggableItem {
    /** セッションのドラッグアイテム */
    data class SessionItem(val session: Session) : DraggableItem()
    /** カテゴリのドラッグアイテム */
    data class CategoryItem(val category: Category) : DraggableItem()
}

/**
 * カテゴリブラウザ画面
 * 指定カテゴリ配下の子カテゴリとセッションを一覧表示する。
 * 長押し→ドラッグ操作でアイテムをカテゴリ間で移動できる。
 *
 * @param modifier レイアウト修飾子
 * @param appState アプリケーション状態
 * @param db Room データベースインスタンス
 * @param categoryId 表示対象カテゴリのID（null = ルート）
 * @param onNavigateToCategory 子カテゴリへの遷移コールバック
 * @param onNavigateToSession セッション詳細への遷移コールバック
 */
@Composable
fun CategoryBrowserScreen(
    modifier: Modifier = Modifier,
    appState: AppState,
    db: AppDatabase? = null,
    categoryId: Int?,
    onNavigateToCategory: (Int) -> Unit,
    onNavigateToSession: (Int) -> Unit,
) {
    val scope = rememberCoroutineScope()
    val snackbarHostState = remember { SnackbarHostState() }

    // 現在のカテゴリ直下の子カテゴリとセッションを取得
    val childCategories = appState.getChildCategories(categoryId)
    val sessions = appState.getSessionsByCategory(categoryId)

    // 追加ダイアログ表示フラグと入力状態
    var showAddDialog by remember { mutableStateOf(false) }
    var addMode by remember { mutableStateOf(AddMode.CATEGORY) }
    var inputName by remember { mutableStateOf("") }

    // カテゴリ削除確認ダイアログ
    var confirmDeleteCategoryId by remember { mutableStateOf<Int?>(null) }
    // セッション削除確認ダイアログ
    var confirmDeleteSessionId by remember { mutableStateOf<Int?>(null) }

    // ドラッグ中のアイテム（null = ドラッグなし）
    var draggingItem by remember { mutableStateOf<DraggableItem?>(null) }
    // ドラッグ中のポインタ位置（ウィンドウ座標）
    var dragOffset by remember { mutableStateOf(Offset.Zero) }
    // ホバー中のカテゴリID（null = ルートゾーン or ホバーなし）
    var hoveredCategoryId by remember { mutableStateOf<Int?>(null) }
    // ルートゾーン上にホバーしているか
    var isHoveringRoot by remember { mutableStateOf(false) }

    // 各アイテムの画面座標を記録するマップ（キー: "cat_{id}" or "ses_{id}"）
    // 再コンポーズを発生させないよう通常の HashMap を使用
    val itemLayoutCoords = remember { HashMap<String, LayoutCoordinates>() }
    // ルートゾーンの座標（配列でラップして参照渡しを実現）
    val rootZoneCoordsHolder = remember { arrayOfNulls<LayoutCoordinates>(1) }

    // ドラッグ中かどうか
    val isDragging = draggingItem != null

    /**
     * ドラッグ座標からホバー状態を更新する
     * @param pos 現在のドラッグ位置（ウィンドウ座標）
     * @param excludeCategoryId ホバー対象から除外するカテゴリID（ドラッグ元）
     */
    fun updateHoverState(pos: Offset, excludeCategoryId: Int?) {
        // ルートゾーン上かどうかをチェック
        val rootBounds = rootZoneCoordsHolder[0]?.boundsInWindow()
        isHoveringRoot = rootBounds != null && rootBounds.contains(pos)
        if (isHoveringRoot) {
            hoveredCategoryId = null
            return
        }
        // カテゴリのホバーをチェック（ドラッグ中のカテゴリ自身は除外）
        hoveredCategoryId = itemLayoutCoords.entries
            .filter { it.key.startsWith("cat_") }
            .firstOrNull { (key, coords) ->
                val id = key.removePrefix("cat_").toIntOrNull()
                id != null && id != excludeCategoryId && coords.boundsInWindow().contains(pos)
            }
            ?.key?.removePrefix("cat_")?.toIntOrNull()
    }

    /**
     * ドロップ処理: ホバー中のカテゴリにアイテムを移動する
     */
    fun handleDrop() {
        val item = draggingItem ?: return
        val targetCategoryId = hoveredCategoryId
        val toRoot = isHoveringRoot

        when (item) {
            is DraggableItem.SessionItem -> {
                // ルートへ移動、またはカテゴリ上でのドロップのみ処理
                val newCatId = if (toRoot) null else targetCategoryId ?: return
                appState.moveSession(item.session.id, newCatId)
                if (db != null) {
                    scope.launch { appState.persistMoveSession(db, item.session.id, newCatId) }
                }
            }
            is DraggableItem.CategoryItem -> {
                val newParentId = if (toRoot) null else targetCategoryId ?: return
                if (!appState.moveCategory(item.category.id, newParentId)) {
                    // 循環参照などで移動不可の場合は Snackbar で通知
                    scope.launch {
                        snackbarHostState.showSnackbar("このカテゴリには移動できません")
                    }
                } else if (db != null) {
                    scope.launch { appState.persistMoveCategory(db, item.category.id, newParentId) }
                }
            }
        }
    }

    /**
     * ドラッグ状態をリセットする
     */
    fun resetDragState() {
        draggingItem = null
        dragOffset = Offset.Zero
        hoveredCategoryId = null
        isHoveringRoot = false
    }

    Scaffold(
        modifier = modifier,
        snackbarHost = { SnackbarHost(snackbarHostState) },
    ) { innerPadding ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding),
        ) {
            // ===== メインコンテンツ =====
            if (!isDragging && childCategories.isEmpty() && sessions.isEmpty()) {
                // 空の状態を表示（ドラッグ中は非表示）
                Box(
                    modifier = Modifier.fillMaxSize(),
                    contentAlignment = Alignment.Center,
                ) {
                    Column(
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.Center,
                    ) {
                        Text(
                            text = "まだ項目がありません",
                            style = MaterialTheme.typography.bodyLarge,
                            textAlign = TextAlign.Center,
                        )
                        Spacer(modifier = Modifier.height(8.dp))
                        Text(
                            text = "右下の「＋」ボタンでカテゴリまたはセッションを追加できます",
                            style = MaterialTheme.typography.bodyMedium,
                            textAlign = TextAlign.Center,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                            modifier = Modifier.padding(horizontal = 32.dp),
                        )
                    }
                }
            } else {
                LazyColumn(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(horizontal = 16.dp, vertical = 8.dp),
                ) {
                    // ===== ルートへ移動ゾーン（ドラッグ中のみ表示） =====
                    if (isDragging) {
                        item {
                            Box(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .height(48.dp)
                                    .background(
                                        if (isHoveringRoot) MaterialTheme.colorScheme.primaryContainer
                                        else MaterialTheme.colorScheme.surfaceVariant,
                                    )
                                    .border(
                                        width = if (isHoveringRoot) 2.dp else 1.dp,
                                        color = if (isHoveringRoot) MaterialTheme.colorScheme.primary
                                        else MaterialTheme.colorScheme.outline,
                                    )
                                    .onGloballyPositioned { coords ->
                                        // ルートゾーンの座標を記録
                                        rootZoneCoordsHolder[0] = coords
                                    },
                                contentAlignment = Alignment.Center,
                            ) {
                                Text(
                                    text = "↑ ここにドロップしてルートへ移動",
                                    style = MaterialTheme.typography.labelLarge,
                                    color = if (isHoveringRoot) MaterialTheme.colorScheme.onPrimaryContainer
                                    else MaterialTheme.colorScheme.onSurfaceVariant,
                                )
                            }
                        }
                    }

                    // ===== カテゴリ一覧 =====
                    if (childCategories.isNotEmpty()) {
                        item {
                            Text(
                                text = "カテゴリ",
                                style = MaterialTheme.typography.labelLarge,
                                color = MaterialTheme.colorScheme.primary,
                                modifier = Modifier.padding(vertical = 4.dp),
                            )
                        }
                        items(childCategories, key = { it.id }) { category ->
                            // このカテゴリがドラッグ中かどうか
                            val isBeingDragged = draggingItem is DraggableItem.CategoryItem &&
                                    (draggingItem as DraggableItem.CategoryItem).category.id == category.id
                            // このカテゴリがホバー対象かどうか
                            val isHovered = isDragging && hoveredCategoryId == category.id

                            Card(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(vertical = 4.dp)
                                    .alpha(if (isBeingDragged) 0.3f else 1f)
                                    .onGloballyPositioned { coords ->
                                        // カテゴリの画面座標を記録
                                        itemLayoutCoords["cat_${category.id}"] = coords
                                    }
                                    .pointerInput(category.id) {
                                        detectDragGesturesAfterLongPress(
                                            onDragStart = { offset ->
                                                // アイテムのウィンドウ座標を取得して絶対位置を計算
                                                val bounds = itemLayoutCoords["cat_${category.id}"]
                                                    ?.boundsInWindow()
                                                    ?: return@detectDragGesturesAfterLongPress
                                                draggingItem = DraggableItem.CategoryItem(category)
                                                dragOffset = bounds.topLeft + offset
                                            },
                                            onDrag = { change, dragAmount ->
                                                change.consume()
                                                dragOffset += dragAmount
                                                // ドラッグ中カテゴリIDを自身の除外対象として渡す
                                                val dragCatId = (draggingItem as? DraggableItem.CategoryItem)
                                                    ?.category?.id
                                                updateHoverState(dragOffset, dragCatId)
                                            },
                                            onDragEnd = {
                                                handleDrop()
                                                resetDragState()
                                            },
                                            onDragCancel = {
                                                resetDragState()
                                            },
                                        )
                                    }
                                    .then(
                                        // ドラッグ中はタップによる画面遷移を抑制
                                        if (!isDragging) Modifier.clickable { onNavigateToCategory(category.id) }
                                        else Modifier
                                    ),
                                colors = if (isHovered) CardDefaults.cardColors(
                                    containerColor = MaterialTheme.colorScheme.primaryContainer,
                                ) else CardDefaults.cardColors(),
                                border = if (isHovered) BorderStroke(
                                    2.dp, MaterialTheme.colorScheme.primary,
                                ) else null,
                            ) {
                                Row(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(horizontal = 16.dp, vertical = 12.dp),
                                    verticalAlignment = Alignment.CenterVertically,
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                ) {
                                    Row(
                                        verticalAlignment = Alignment.CenterVertically,
                                        modifier = Modifier.weight(1f),
                                    ) {
                                        // フォルダアイコン
                                        Icon(
                                            imageVector = Icons.Default.Folder,
                                            contentDescription = "カテゴリ",
                                            tint = MaterialTheme.colorScheme.primary,
                                            modifier = Modifier.size(24.dp),
                                        )
                                        Spacer(modifier = Modifier.width(12.dp))
                                        Column {
                                            Text(
                                                text = category.name,
                                                style = MaterialTheme.typography.titleMedium,
                                            )
                                            // 子カテゴリ数・セッション数をバッジ表示
                                            val subCount = appState.getChildCategories(category.id).size
                                            val sesCount = appState.getSessionsByCategory(category.id).size
                                            if (subCount > 0 || sesCount > 0) {
                                                Text(
                                                    text = buildString {
                                                        if (subCount > 0) append("カテゴリ ${subCount}")
                                                        if (subCount > 0 && sesCount > 0) append(" / ")
                                                        if (sesCount > 0) append("セッション ${sesCount}")
                                                    },
                                                    style = MaterialTheme.typography.bodySmall,
                                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                                )
                                            }
                                        }
                                    }
                                    // ドラッグ中は削除ボタンを非表示
                                    if (!isDragging) {
                                        IconButton(onClick = { confirmDeleteCategoryId = category.id }) {
                                            Icon(Icons.Default.Delete, contentDescription = "削除")
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // ===== セッション一覧 =====
                    if (sessions.isNotEmpty()) {
                        item {
                            Text(
                                text = "セッション",
                                style = MaterialTheme.typography.labelLarge,
                                color = MaterialTheme.colorScheme.primary,
                                modifier = Modifier.padding(top = 12.dp, bottom = 4.dp),
                            )
                        }
                        items(sessions, key = { it.id }) { session ->
                            // このセッションがドラッグ中かどうか
                            val isBeingDragged = draggingItem is DraggableItem.SessionItem &&
                                    (draggingItem as DraggableItem.SessionItem).session.id == session.id

                            Box(
                                modifier = Modifier
                                    .alpha(if (isBeingDragged) 0.3f else 1f)
                                    .onGloballyPositioned { coords ->
                                        // セッションの画面座標を記録
                                        itemLayoutCoords["ses_${session.id}"] = coords
                                    }
                                    .pointerInput(session.id) {
                                        detectDragGesturesAfterLongPress(
                                            onDragStart = { offset ->
                                                val bounds = itemLayoutCoords["ses_${session.id}"]
                                                    ?.boundsInWindow()
                                                    ?: return@detectDragGesturesAfterLongPress
                                                draggingItem = DraggableItem.SessionItem(session)
                                                dragOffset = bounds.topLeft + offset
                                            },
                                            onDrag = { change, dragAmount ->
                                                change.consume()
                                                dragOffset += dragAmount
                                                // セッションはカテゴリへのみドロップ可能（自身除外なし）
                                                updateHoverState(dragOffset, null)
                                            },
                                            onDragEnd = {
                                                handleDrop()
                                                resetDragState()
                                            },
                                            onDragCancel = {
                                                resetDragState()
                                            },
                                        )
                                    },
                            ) {
                                SessionItem(
                                    session = session,
                                    users = appState.getSessionUsers(session.id),
                                    // ドラッグ中はタップによる画面遷移を抑制
                                    onClick = { if (!isDragging) onNavigateToSession(session.id) },
                                    onDelete = { confirmDeleteSessionId = session.id },
                                )
                            }
                        }
                    }

                    // FAB の重なり分のスペース
                    item { Spacer(modifier = Modifier.height(80.dp)) }
                }
            }

            // ===== フローティングドラッグプレビュー（ドラッグ中のみ表示） =====
            val currentDraggingItem = draggingItem
            if (currentDraggingItem != null) {
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .zIndex(10f),
                ) {
                    // ドラッグ中アイテムのゴースト（半透明・絶対座標に配置）
                    Card(
                        modifier = Modifier
                            .offset {
                                IntOffset(
                                    dragOffset.x.toInt(),
                                    dragOffset.y.toInt(),
                                )
                            }
                            .alpha(0.5f)
                            .widthIn(min = 180.dp),
                        elevation = CardDefaults.cardElevation(defaultElevation = 8.dp),
                    ) {
                        Row(
                            modifier = Modifier.padding(horizontal = 16.dp, vertical = 12.dp),
                            verticalAlignment = Alignment.CenterVertically,
                        ) {
                            when (currentDraggingItem) {
                                is DraggableItem.CategoryItem -> {
                                    Icon(
                                        imageVector = Icons.Default.Folder,
                                        contentDescription = null,
                                        tint = MaterialTheme.colorScheme.primary,
                                        modifier = Modifier.size(24.dp),
                                    )
                                    Spacer(Modifier.width(12.dp))
                                    Text(
                                        text = currentDraggingItem.category.name,
                                        style = MaterialTheme.typography.titleMedium,
                                    )
                                }
                                is DraggableItem.SessionItem -> {
                                    Text(
                                        text = currentDraggingItem.session.name,
                                        style = MaterialTheme.typography.titleMedium,
                                    )
                                }
                            }
                        }
                    }
                }
            }

            // FAB: カテゴリまたはセッションを追加
            FloatingActionButton(
                onClick = {
                    inputName = ""
                    addMode = AddMode.CATEGORY
                    showAddDialog = true
                },
                modifier = Modifier
                    .align(Alignment.BottomEnd)
                    .padding(16.dp),
            ) {
                Icon(Icons.Default.Add, contentDescription = "追加")
            }
        }
    }

    // =====================
    // 追加ダイアログ
    // =====================
    if (showAddDialog) {
        AlertDialog(
            onDismissRequest = { showAddDialog = false },
            title = { Text("追加") },
            text = {
                Column {
                    // カテゴリ / セッション の選択ラジオボタン
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        RadioButton(
                            selected = addMode == AddMode.CATEGORY,
                            onClick = { addMode = AddMode.CATEGORY },
                        )
                        Text(
                            text = "カテゴリを追加",
                            modifier = Modifier.clickable { addMode = AddMode.CATEGORY },
                        )
                    }
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        RadioButton(
                            selected = addMode == AddMode.SESSION,
                            onClick = { addMode = AddMode.SESSION },
                        )
                        Text(
                            text = "セッションを追加",
                            modifier = Modifier.clickable { addMode = AddMode.SESSION },
                        )
                    }
                    Spacer(modifier = Modifier.height(8.dp))
                    // 名前入力フィールド
                    OutlinedTextField(
                        value = inputName,
                        onValueChange = { inputName = it },
                        label = { Text(if (addMode == AddMode.CATEGORY) "カテゴリ名" else "セッション名") },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth(),
                    )
                }
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        val name = inputName.trim()
                        if (name.isNotBlank()) {
                            if (addMode == AddMode.CATEGORY) {
                                // カテゴリを追加
                                val category = appState.addCategory(categoryId, name)
                                if (db != null) {
                                    scope.launch { appState.persistNewCategory(db, category) }
                                }
                            } else {
                                // セッションを追加（詳細画面へ遷移するため直接作成）
                                val session = appState.addSession(name, categoryId)
                                if (db != null) {
                                    scope.launch { appState.persistNewSession(db, session) }
                                }
                                onNavigateToSession(session.id)
                            }
                        }
                        showAddDialog = false
                    },
                ) { Text("作成") }
            },
            dismissButton = {
                TextButton(onClick = { showAddDialog = false }) { Text("キャンセル") }
            },
        )
    }

    // =====================
    // カテゴリ削除確認ダイアログ
    // =====================
    if (confirmDeleteCategoryId != null) {
        val targetId = confirmDeleteCategoryId!!
        AlertDialog(
            onDismissRequest = { confirmDeleteCategoryId = null },
            title = { Text("カテゴリを削除") },
            text = { Text("このカテゴリと配下のカテゴリ・セッションをすべて削除します。よろしいですか？") },
            confirmButton = {
                TextButton(onClick = {
                    appState.deleteCategoryLocal(targetId)
                    if (db != null) {
                        scope.launch { appState.persistDeleteCategory(db, targetId) }
                    }
                    confirmDeleteCategoryId = null
                }) { Text("削除") }
            },
            dismissButton = {
                TextButton(onClick = { confirmDeleteCategoryId = null }) { Text("キャンセル") }
            },
        )
    }

    // =====================
    // セッション削除確認ダイアログ
    // =====================
    if (confirmDeleteSessionId != null) {
        val targetId = confirmDeleteSessionId!!
        AlertDialog(
            onDismissRequest = { confirmDeleteSessionId = null },
            title = { Text("セッションを削除") },
            text = { Text("このセッションと関連データを削除します。よろしいですか？") },
            confirmButton = {
                TextButton(onClick = {
                    if (db != null) {
                        scope.launch { appState.deleteSession(db, targetId) }
                    } else {
                        appState.deleteSessionLocal(targetId)
                    }
                    confirmDeleteSessionId = null
                }) { Text("削除") }
            },
            dismissButton = {
                TextButton(onClick = { confirmDeleteSessionId = null }) { Text("キャンセル") }
            },
        )
    }
}
