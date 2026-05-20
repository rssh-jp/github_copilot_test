package com.example.testapp2.ui.screens

import androidx.compose.foundation.clickable
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
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import com.example.testapp2.data.AppState
import com.example.testapp2.data.db.AppDatabase
import com.example.testapp2.ui.components.SessionItem
import kotlinx.coroutines.launch

/** 追加ダイアログで選択するモード */
private enum class AddMode { CATEGORY, SESSION }

/**
 * カテゴリブラウザ画面
 * 指定カテゴリ配下の子カテゴリとセッションを一覧表示する。
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

    Box(modifier = modifier.fillMaxSize()) {
        if (childCategories.isEmpty() && sessions.isEmpty()) {
            // 空の状態を表示
            Box(
                modifier = Modifier.fillMaxSize(),
                contentAlignment = Alignment.Center
            ) {
                Column(
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.Center
                ) {
                    Text(
                        text = "まだ項目がありません",
                        style = MaterialTheme.typography.bodyLarge,
                        textAlign = TextAlign.Center
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(
                        text = "右下の「＋」ボタンでカテゴリまたはセッションを追加できます",
                        style = MaterialTheme.typography.bodyMedium,
                        textAlign = TextAlign.Center,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                        modifier = Modifier.padding(horizontal = 32.dp)
                    )
                }
            }
        } else {
            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(horizontal = 16.dp, vertical = 8.dp)
            ) {
                // カテゴリ一覧
                if (childCategories.isNotEmpty()) {
                    item {
                        Text(
                            text = "カテゴリ",
                            style = MaterialTheme.typography.labelLarge,
                            color = MaterialTheme.colorScheme.primary,
                            modifier = Modifier.padding(vertical = 4.dp)
                        )
                    }
                    items(childCategories) { category ->
                        Card(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(vertical = 4.dp)
                                .clickable { onNavigateToCategory(category.id) }
                        ) {
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(horizontal = 16.dp, vertical = 12.dp),
                                verticalAlignment = Alignment.CenterVertically,
                                horizontalArrangement = Arrangement.SpaceBetween
                            ) {
                                Row(
                                    verticalAlignment = Alignment.CenterVertically,
                                    modifier = Modifier.weight(1f)
                                ) {
                                    // フォルダアイコン
                                    Icon(
                                        imageVector = Icons.Default.Folder,
                                        contentDescription = "カテゴリ",
                                        tint = MaterialTheme.colorScheme.primary,
                                        modifier = Modifier.size(24.dp)
                                    )
                                    Spacer(modifier = Modifier.width(12.dp))
                                    Column {
                                        Text(
                                            text = category.name,
                                            style = MaterialTheme.typography.titleMedium
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
                                                color = MaterialTheme.colorScheme.onSurfaceVariant
                                            )
                                        }
                                    }
                                }
                                // 削除ボタン
                                IconButton(onClick = { confirmDeleteCategoryId = category.id }) {
                                    Icon(Icons.Default.Delete, contentDescription = "削除")
                                }
                            }
                        }
                    }
                }

                // セッション一覧
                if (sessions.isNotEmpty()) {
                    item {
                        Text(
                            text = "セッション",
                            style = MaterialTheme.typography.labelLarge,
                            color = MaterialTheme.colorScheme.primary,
                            modifier = Modifier.padding(top = 12.dp, bottom = 4.dp)
                        )
                    }
                    items(sessions) { session ->
                        SessionItem(
                            session = session,
                            users = appState.getSessionUsers(session.id),
                            onClick = { onNavigateToSession(session.id) },
                            onDelete = { confirmDeleteSessionId = session.id }
                        )
                    }
                }

                // FAB の重なり分のスペース
                item { Spacer(modifier = Modifier.height(80.dp)) }
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
                .padding(16.dp)
        ) {
            Icon(Icons.Default.Add, contentDescription = "追加")
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
                            onClick = { addMode = AddMode.CATEGORY }
                        )
                        Text(
                            text = "カテゴリを追加",
                            modifier = Modifier.clickable { addMode = AddMode.CATEGORY }
                        )
                    }
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        RadioButton(
                            selected = addMode == AddMode.SESSION,
                            onClick = { addMode = AddMode.SESSION }
                        )
                        Text(
                            text = "セッションを追加",
                            modifier = Modifier.clickable { addMode = AddMode.SESSION }
                        )
                    }
                    Spacer(modifier = Modifier.height(8.dp))
                    // 名前入力フィールド
                    OutlinedTextField(
                        value = inputName,
                        onValueChange = { inputName = it },
                        label = { Text(if (addMode == AddMode.CATEGORY) "カテゴリ名" else "セッション名") },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth()
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
                                // セッションを追加（詳細画面へ遷移するため -1 の代わりに直接作成）
                                val session = appState.addSession(name, categoryId)
                                if (db != null) {
                                    scope.launch {
                                        appState.persistNewSession(db, session)
                                    }
                                }
                                onNavigateToSession(session.id)
                            }
                        }
                        showAddDialog = false
                    }
                ) { Text("作成") }
            },
            dismissButton = {
                TextButton(onClick = { showAddDialog = false }) { Text("キャンセル") }
            }
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
            }
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
            }
        )
    }
}
