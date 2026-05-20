package com.example.testapp2.ui.components

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.testapp2.data.Category
import com.example.testapp2.data.CategoryType

/**
 * カテゴリ・セクションのリストアイテムコンポーネント
 * @param category 表示するカテゴリ情報
 * @param onClick アイテムタップ時のコールバック
 * @param onDelete 削除ボタンタップ時のコールバック
 */
@Composable
fun CategoryItem(
    category: Category,
    onClick: () -> Unit,
    onDelete: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Row(
        modifier = modifier
            .fillMaxWidth()
            .clickable(onClick = onClick)
            .padding(horizontal = 16.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        // 種別アイコン
        Icon(
            imageVector = when (category.type) {
                CategoryType.FOLDER -> Icons.Default.Folder
                CategoryType.SECTION -> Icons.Default.PlayArrow
            },
            contentDescription = when (category.type) {
                CategoryType.FOLDER -> "フォルダ"
                CategoryType.SECTION -> "セクション"
            },
            tint = when (category.type) {
                CategoryType.FOLDER -> MaterialTheme.colorScheme.primary
                CategoryType.SECTION -> MaterialTheme.colorScheme.secondary
            },
        )

        // カテゴリ名と種別ラベル
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = category.name,
                style = MaterialTheme.typography.bodyLarge,
            )
            Text(
                text = when (category.type) {
                    CategoryType.FOLDER -> "フォルダ（さらに分類可能）"
                    CategoryType.SECTION -> "セクション（スコアを入力）"
                },
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
        }

        // 削除ボタン
        IconButton(onClick = onDelete) {
            Icon(Icons.Default.Delete, contentDescription = "削除")
        }
    }
}
