package com.example.testapp.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.testapp.ui.components.CenteredScreenLayout

/**
 * ユーザー数入力画面
 */
@Composable
fun UserCountInputScreen(
    onUserCountSet: (Int) -> Unit,
    modifier: Modifier = Modifier
) {
    var userCountText by remember { mutableStateOf("") }
    var isError by remember { mutableStateOf(false) }
    
    CenteredScreenLayout(modifier = modifier) {
        // タイトル
        Text(
            text = "得点集計アプリ",
            fontSize = 32.sp,
            fontWeight = FontWeight.Bold,
            color = MaterialTheme.colorScheme.primary
        )
        
        Spacer(modifier = Modifier.height(48.dp))
        
        // 説明
        Text(
            text = "参加するユーザー数を入力してください",
            fontSize = 18.sp,
            color = MaterialTheme.colorScheme.onSurface
        )
        
        Spacer(modifier = Modifier.height(32.dp))
        
        // ユーザー数入力フィールド
        OutlinedTextField(
            value = userCountText,
            onValueChange = { newValue ->
                userCountText = newValue
                isError = false
            },
            label = { Text("ユーザー数") },
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
            isError = isError,
            supportingText = if (isError) {
                { Text("1以上の数値を入力してください") }
            } else null,
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 32.dp)
        )
        
        Spacer(modifier = Modifier.height(32.dp))
        
        // 開始ボタン
        Button(
            onClick = {
                val count = userCountText.toIntOrNull()
                if (count != null && count > 0) {
                    onUserCountSet(count)
                } else {
                    isError = true
                }
            },
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 32.dp)
        ) {
            Text(
                text = "開始",
                fontSize = 16.sp,
                modifier = Modifier.padding(8.dp)
            )
        }
        
        Spacer(modifier = Modifier.height(32.dp))
        
        // 注意事項
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 16.dp),
            colors = CardDefaults.cardColors(
                containerColor = MaterialTheme.colorScheme.surfaceVariant
            )
        ) {
            Column(
                modifier = Modifier.padding(16.dp),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Text(
                    text = "ご利用方法",
                    fontSize = 16.sp,
                    fontWeight = FontWeight.Medium,
                    color = MaterialTheme.colorScheme.primary
                )
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    text = "1. 参加ユーザー数を入力\n" +
                            "2. 各ユーザーの名前を入力\n" +
                            "3. ゲームを進めながらスコアを記録\n" +
                            "4. 履歴から過去のスコアを確認・編集",
                    fontSize = 14.sp,
                    lineHeight = 20.sp,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
    }
}
