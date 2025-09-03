package com.example.testapp.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.testapp.ui.components.*

/**
 * ユーザー数入力画面
 */
@Composable
fun UserCountInputScreen(
    onUserCountSet: (Int) -> Unit,
    onBack: (() -> Unit)? = null,
    modifier: Modifier = Modifier
) {
    var userCountText by remember { mutableStateOf("") }
    var isError by remember { mutableStateOf(false) }
    
    CommonScreenLayout(modifier = modifier) {
        // ヘッダー
        CommonHeader(
            title = "得点集計アプリ",
            subtitle = "参加するユーザー数を入力してください"
        )
        
        VerticalSpacer(32)
        
        // ユーザー数入力フィールド
        NumberInputField(
            value = userCountText,
            onValueChange = { newValue ->
                userCountText = newValue
                isError = false
            },
            label = "ユーザー数",
            isError = isError,
            errorMessage = "1以上の数値を入力してください",
            modifier = Modifier.standardHorizontalPadding()
        )
        
        VerticalSpacer(32)
        
        // 開始ボタン
        PrimaryActionButton(
            text = "開始",
            onClick = {
                val count = userCountText.toIntOrNull()
                if (count != null && count > 0) {
                    onUserCountSet(count)
                } else {
                    isError = true
                }
            },
            modifier = Modifier.standardHorizontalPadding()
        )
        
        VerticalSpacer(16)
        
        // 戻るボタン
        onBack?.let { backAction ->
            SecondaryActionButton(
                text = "戻る",
                onClick = backAction,
                modifier = Modifier.standardHorizontalPadding()
            )
            
            VerticalSpacer(16)
        }
        
        // 注意事項
        InfoCard(
            title = "ご利用方法",
            content = "1. 参加ユーザー数を入力\n" +
                    "2. 各ユーザーの名前を入力\n" +
                    "3. ゲームを進めながらスコアを記録\n" +
                    "4. 履歴から過去のスコアを確認・編集",
            modifier = Modifier.standardCardPadding()
        )
    }
}
