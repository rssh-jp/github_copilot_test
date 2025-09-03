package com.example.testapp.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.testapp.data.entity.Session
import com.example.testapp.ui.components.*

/**
 * スコア入力画面（リファクタリング版）
 */
@Composable
fun ScoreInputScreen(
    session: Session,
    gameScoreInputs: Map<String, Int>,
    onScoreInputChange: (String, Int) -> Unit,
    onSaveScores: (String) -> Unit,
    onCompleteSession: () -> Unit,
    onViewHistory: () -> Unit,
    modifier: Modifier = Modifier
) {
    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp)
            .verticalScroll(rememberScrollState())
    ) {
        // セッション情報ヘッダー
        SessionInfoHeader(session = session)
        
        VerticalSpacer(16)
        
        // スコア入力フォーム
        ScoreInputForm(
            gameScoreInputs = gameScoreInputs,
            onScoreInputChange = onScoreInputChange,
            onSaveScores = onSaveScores
        )
        
        VerticalSpacer(16)
        
        // ゲーム履歴
        GameHistorySection(session = session)
        
        VerticalSpacer(24)
        
        // アクションボタン
        ActionButtonRow(
            onCompleteSession = onCompleteSession,
            onViewHistory = onViewHistory
        )
    }
}
