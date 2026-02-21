package com.example.testapp2.ui.components

import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Person
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.testapp2.data.User

/**
 * ユーザー情報を表示する共通コンポーネント
 * @param user 表示対象ユーザー
 * @param onDelete 削除ボタン押下時のコールバック（nullの場合は削除ボタン非表示）
 */
@Composable
fun UserItem(user: User, onDelete: (() -> Unit)? = null) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Icon(
            Icons.Default.Person,
            contentDescription = "ユーザー",
            modifier = Modifier.padding(end = 8.dp)
        )
        Text(
            text = user.name,
            style = MaterialTheme.typography.bodyLarge,
            modifier = Modifier.weight(1f)
        )
        if (user.score > 0) {
            Text(
                text = "${user.score}点",
                style = MaterialTheme.typography.bodyMedium
            )
        }
        if (onDelete != null) {
            IconButton(onClick = onDelete) {
                Icon(Icons.Default.Delete, contentDescription = "削除")
            }
        }
    }
}
