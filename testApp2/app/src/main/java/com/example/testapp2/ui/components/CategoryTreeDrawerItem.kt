package com.example.testapp2.ui.components

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.expandVertically
import androidx.compose.animation.shrinkVertically
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationDrawerItem
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.testapp2.data.AppState
import com.example.testapp2.data.Category

/**
 * カテゴリツリーの1ノードを再帰的に描画するコンポーザブル
 *
 * @param category 描画対象カテゴリ
 * @param allCategories AppState.categories の全件リスト（子カテゴリ取得に使用）
 * @param depth ネスト深さ（0 = ルート直下）
 * @param selectedCategoryId 現在表示中のカテゴリID（再帰の各段で選択状態を判定するために使用）
 * @param expandedCategoryIds 展開中カテゴリIDセット（再帰の各段で展開状態を判定するために使用）
 * @param onCategoryClick カテゴリ名タップ時のコールバック
 * @param onToggleExpand 展開アイコンタップ時のコールバック（カテゴリIDを渡す）
 */
@Composable
fun CategoryTreeItem(
    category: Category,
    allCategories: List<Category>,
    depth: Int,
    selectedCategoryId: Int?,
    expandedCategoryIds: Set<Int>,
    onCategoryClick: (Category) -> Unit,
    onToggleExpand: (Int) -> Unit,
) {
    // このカテゴリの直接の子カテゴリを取得
    val children = allCategories.filter { it.parentId == category.id }
    val hasChildren = children.isNotEmpty()
    val isSelected = category.id == selectedCategoryId
    val isExpanded = category.id in expandedCategoryIds

    // 深さに応じた左インデント（1段階につき16.dp）
    val indentStart = (depth * 16).dp

    NavigationDrawerItem(
        label = { Text(category.name) },
        selected = isSelected,
        // カテゴリ名タップ → 画面遷移、展開アイコン表示時はトグルも兼ねる
        onClick = {
            onCategoryClick(category)
        },
        modifier = Modifier
            .fillMaxWidth()
            .padding(start = indentStart),
        // 子カテゴリを持つ場合は展開状態アイコンを badge で表示（タップでトグル）
        badge = if (hasChildren) {
            {
                Text(
                    text = if (isExpanded) "▼" else "▶",
                    modifier = Modifier.clickable { onToggleExpand(category.id) },
                )
            }
        } else null,
    )

    // 子カテゴリを AnimatedVisibility でアニメーション展開
    if (hasChildren) {
        AnimatedVisibility(
            visible = isExpanded,
            enter = expandVertically(),
            exit = shrinkVertically()
        ) {
            Column {
                children.forEach { child ->
                    CategoryTreeItem(
                        category = child,
                        allCategories = allCategories,
                        depth = depth + 1,
                        selectedCategoryId = selectedCategoryId,
                        expandedCategoryIds = expandedCategoryIds,
                        onCategoryClick = onCategoryClick,
                        onToggleExpand = onToggleExpand,
                    )
                }
            }
        }
    }
}

/**
 * ドロワー全体のカテゴリツリーを描画するコンポーザブル
 *
 * @param appState アプリケーション状態（categories プロパティを参照）
 * @param selectedCategoryId 現在表示中のカテゴリID（null = ルート表示）
 * @param expandedCategoryIds 展開中のカテゴリIDセット
 * @param onCategoryClick カテゴリをタップした時のコールバック
 * @param onToggleExpand 展開アイコンをタップした時のコールバック（カテゴリIDを渡す）
 */
@Composable
fun CategoryTreeDrawerContent(
    appState: AppState,
    selectedCategoryId: Int?,
    expandedCategoryIds: Set<Int>,
    onCategoryClick: (Category) -> Unit,
    onToggleExpand: (Int) -> Unit,
) {
    val allCategories = appState.categories
    // ルート直下のカテゴリ（parentId = null）のみ列挙
    val rootCategories = allCategories.filter { it.parentId == null }

    if (rootCategories.isEmpty()) {
        // カテゴリが0件の場合はメッセージ表示
        Text(
            text = "カテゴリなし",
            style = MaterialTheme.typography.bodyMedium,
            modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp)
        )
    } else {
        LazyColumn {
            items(rootCategories) { category ->
                CategoryTreeItem(
                    category = category,
                    allCategories = allCategories,
                    depth = 0,
                    selectedCategoryId = selectedCategoryId,
                    expandedCategoryIds = expandedCategoryIds,
                    onCategoryClick = onCategoryClick,
                    onToggleExpand = onToggleExpand,
                )
            }
        }
    }
}
