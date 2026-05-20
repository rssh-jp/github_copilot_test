package com.example.testapp2.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import com.example.testapp2.data.AppState
import com.example.testapp2.data.Category
import com.example.testapp2.data.CategoryType
import com.example.testapp2.ui.components.CategoryItem
import kotlinx.coroutines.launch

/**
 * カテゴリ/セクション一覧画面
 * @param appState アプリ状態
 * @param db Room データベース（null 許容）
 * @param sessionId 対象セッションID
 * @param parentCategoryId 現在表示している親カテゴリID（null=ルート）
 * @param onNavigateToCategory 子フォルダへの遷移コールバック
 * @param onNavigateToSection セクション（スコア投入）への遷移コールバック
 */
@Composable
fun CategoryBrowserScreen(
    modifier: Modifier = Modifier,
    appState: AppState,
    db: com.example.testapp2.data.db.AppDatabase? = null,
    sessionId: Int,
    parentCategoryId: Int?,
    onNavigateToCategory: (sessionId: Int, parentCategoryId: Int?) -> Unit,
    onNavigateToSection: (sessionId: Int, sectionId: Int) -> Unit,
) {
    val scope = rememberCoroutineScope()

    // 現在の階層の子カテゴリ一覧を取得
    val children by remember(appState.categories.size, sessionId, parentCategoryId) {
        derivedStateOf { appState.getChildCategories(sessionId, parentCategoryId) }
    }

    // 追加ダイアログ表示フラグ
    var showAddDialog by remember { mutableStateOf(false) }
    // 削除確認ダイアログの対象カテゴリ
    var deletingCategory by remember { mutableStateOf<Category?>(null) }

    // パンくずリスト文字列（ルート以外のとき表示）
    val breadcrumb = buildBreadcrumb(appState, parentCategoryId)

    Box(modifier = modifier.fillMaxSize()) {
        Column(modifier = Modifier.fillMaxSize()) {
            // パンくずリスト表示
            if (breadcrumb.isNotEmpty()) {
                Text(
                    text = breadcrumb,
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp),
                )
                HorizontalDivider()
            }

            if (children.isEmpty()) {
                // 空状態の表示
                Box(
                    modifier = Modifier.fillMaxSize(),
                    contentAlignment = Alignment.Center,
                ) {
                    Text(
                        text = "カテゴリ・セクションがありません\n「＋」ボタンで追加してください",
                        style = MaterialTheme.typography.bodyMedium,
                        textAlign = TextAlign.Center,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                }
            } else {
                LazyColumn(
                    modifier = Modifier.fillMaxSize(),
                    contentPadding = PaddingValues(bottom = 80.dp), // FAB の余白
                ) {
                    items(children, key = { it.id }) { category ->
                        CategoryItem(
                            category = category,
                            onClick = {
                                when (category.type) {
                                    // フォルダ: 同画面を再帰的に開く
                                    CategoryType.FOLDER -> onNavigateToCategory(sessionId, category.id)
                                    // セクション: スコア投入画面へ遷移
                                    CategoryType.SECTION -> onNavigateToSection(sessionId, category.id)
                                }
                            },
                            onDelete = { deletingCategory = category },
                        )
                        HorizontalDivider()
                    }
                }
            }
        }

        // 追加 FAB
        FloatingActionButton(
            onClick = { showAddDialog = true },
            modifier = Modifier
                .align(Alignment.BottomEnd)
                .padding(16.dp),
        ) {
            Icon(Icons.Default.Add, contentDescription = "カテゴリを追加")
        }
    }

    // カテゴリ追加ダイアログ
    if (showAddDialog) {
        AddCategoryDialog(
            onConfirm = { name, type ->
                val category = appState.addCategory(sessionId, parentCategoryId, name, type)
                if (db != null) {
                    scope.launch { appState.persistNewCategory(db, category) }
                }
                showAddDialog = false
            },
            onDismiss = { showAddDialog = false },
        )
    }

    // 削除確認ダイアログ
    deletingCategory?.let { target ->
        AlertDialog(
            onDismissRequest = { deletingCategory = null },
            title = { Text("削除の確認") },
            text = {
                Text("「${target.name}」を削除しますか？\n子カテゴリ・セクションもすべて削除されます。")
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        // DB削除用 ID をメモリから収集（deleteCategoryLocal の前に実行）
                        val idsToDelete = appState.collectCategoryAndDescendantIds(target.id)
                        appState.deleteCategoryLocal(target.id)
                        if (db != null) {
                            scope.launch { appState.persistDeleteCategoryTree(db, idsToDelete) }
                        }
                        deletingCategory = null
                    }
                ) { Text("削除", color = MaterialTheme.colorScheme.error) }
            },
            dismissButton = {
                TextButton(onClick = { deletingCategory = null }) { Text("キャンセル") }
            },
        )
    }
}

/**
 * 現在の階層からルートまでのパスをパンくずリスト文字列として生成
 * 例: 「大会 > 予選 > Aブロック」
 */
private fun buildBreadcrumb(appState: AppState, parentCategoryId: Int?): String {
    if (parentCategoryId == null) return ""
    val path = mutableListOf<String>()
    var currentId: Int? = parentCategoryId
    while (currentId != null) {
        val cat = appState.categories.find { it.id == currentId } ?: break
        path.add(0, cat.name)
        currentId = cat.parentId
    }
    return path.joinToString(" > ")
}

/**
 * カテゴリ/セクション追加ダイアログ
 */
@Composable
private fun AddCategoryDialog(
    onConfirm: (name: String, type: CategoryType) -> Unit,
    onDismiss: () -> Unit,
) {
    var name by remember { mutableStateOf("") }
    var selectedType by remember { mutableStateOf(CategoryType.FOLDER) }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text("カテゴリを追加") },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                OutlinedTextField(
                    value = name,
                    onValueChange = { name = it },
                    label = { Text("名前") },
                    modifier = Modifier.fillMaxWidth(),
                    singleLine = true,
                )
                Text("種別", style = MaterialTheme.typography.bodyMedium)
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    RadioButton(
                        selected = selectedType == CategoryType.FOLDER,
                        onClick = { selectedType = CategoryType.FOLDER },
                    )
                    Spacer(Modifier.width(4.dp))
                    Text("フォルダ（さらに分類可能）")
                }
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    RadioButton(
                        selected = selectedType == CategoryType.SECTION,
                        onClick = { selectedType = CategoryType.SECTION },
                    )
                    Spacer(Modifier.width(4.dp))
                    Text("セクション（スコアを入力）")
                }
            }
        },
        confirmButton = {
            TextButton(
                onClick = { if (name.isNotBlank()) onConfirm(name.trim(), selectedType) },
                enabled = name.isNotBlank(),
            ) { Text("追加") }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) { Text("キャンセル") }
        },
    )
}
