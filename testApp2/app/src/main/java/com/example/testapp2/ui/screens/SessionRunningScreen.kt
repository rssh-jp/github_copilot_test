package com.example.testapp2.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Person
import androidx.compose.material.icons.filled.Add
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import androidx.compose.ui.unit.dp
import com.example.testapp2.data.AppState
import com.example.testapp2.data.ScoreRecord
import com.example.testapp2.data.User
import com.example.testapp2.ui.theme.TestApp2Theme
import kotlinx.coroutines.delay
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.compose.ui.platform.LocalLifecycleOwner
import java.text.SimpleDateFormat
import java.util.*

@Composable
fun SessionRunningScreen(
    modifier: Modifier = Modifier,
    appState: AppState,
    db: com.example.testapp2.data.db.AppDatabase? = null,
    sessionId: Int
) {
    val session = appState.sessions.find { it.id == sessionId }
    val users = appState.getSessionUsers(sessionId)
    var currentTime by remember { mutableStateOf(System.currentTimeMillis()) }
    // 画面入場時刻（同一sessionIdで固定）
    val enterTime = remember(sessionId) { System.currentTimeMillis() }
    
    // スコア履歴を表示するかのフラグ
    var showHistory by remember { mutableStateOf(false) }
    
    // ユーザーごとのスコア入力
    val userScores = remember(users) {
        mutableStateMapOf<Int, String>().apply {
            users.forEach { user -> 
                this[user.id] = "" // 空の文字列で初期化
            }
        }
    }
    
    // スコア登録成功時のメッセージ表示
    var showSuccessMessage by remember { mutableStateOf(false) }
    var lastScoreId by remember { mutableStateOf<Int?>(null) }
    // 情報メッセージ（全員0などの通知）
    var infoMessage by remember { mutableStateOf<String?>(null) }
    
    // UI更新用のタイマー（DB保存はライフサイクルに合わせて行う）
    LaunchedEffect(Unit) {
        while(true) {
            delay(1000)
            currentTime = System.currentTimeMillis()
        }
    }
    
    val baseElapsed = session?.elapsedTime ?: 0
    val elapsedTimeSec = baseElapsed + ((currentTime - enterTime) / 1000).toInt() // 経過時間（秒）
    val hours = elapsedTimeSec / 3600
    val minutes = (elapsedTimeSec % 3600) / 60
    val seconds = elapsedTimeSec % 60

    // 画面離脱 or バックグラウンド遷移時に保存
    val scopePersist = rememberCoroutineScope()
    var lastSavedElapsed by remember(sessionId) { mutableStateOf(session?.elapsedTime ?: 0) }
    // アプリのライフサイクルを監視（ON_STOP/ON_PAUSEで保存）
    val lifecycleOwner = LocalLifecycleOwner.current
    DisposableEffect(lifecycleOwner, sessionId) {
        val observer = LifecycleEventObserver { _, event ->
            if (event == Lifecycle.Event.ON_STOP || event == Lifecycle.Event.ON_PAUSE) {
                val finalElapsed = baseElapsed + ((System.currentTimeMillis() - enterTime) / 1000).toInt()
                if (finalElapsed > lastSavedElapsed) {
                    appState.updateSessionElapsed(sessionId, finalElapsed)
                    if (db != null) {
                        scopePersist.launch { appState.persistUpdateSessionElapsed(db, sessionId, finalElapsed) }
                    }
                    lastSavedElapsed = finalElapsed
                }
            }
        }
        val lifecycle = lifecycleOwner.lifecycle
        lifecycle.addObserver(observer)
        onDispose {
            lifecycle.removeObserver(observer)
            // ナビゲーションで画面が破棄される場合にも最終保存
            val finalElapsed = baseElapsed + ((System.currentTimeMillis() - enterTime) / 1000).toInt()
            if (finalElapsed > lastSavedElapsed) {
                appState.updateSessionElapsed(sessionId, finalElapsed)
                if (db != null) {
                    scopePersist.launch { appState.persistUpdateSessionElapsed(db, sessionId, finalElapsed) }
                }
            }
        }
    }
    
    Column(
        modifier = modifier.fillMaxSize(),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        // セッションの基本情報
        Card(
            modifier = Modifier.fillMaxWidth(),
        ) {
            Column(
                modifier = Modifier.padding(16.dp).fillMaxWidth(),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Text(
                    text = session?.name ?: "不明なセッション",
                    style = MaterialTheme.typography.headlineMedium,
                    modifier = Modifier.padding(bottom = 8.dp)
                )
                
                Text(
                    text = "実行中",
                    style = MaterialTheme.typography.titleMedium,
                    color = MaterialTheme.colorScheme.primary,
                    modifier = Modifier.padding(vertical = 8.dp)
                )
                
                // タイマー表示
                Text(
                    text = String.format("%02d:%02d:%02d", hours, minutes, seconds),
                    style = MaterialTheme.typography.headlineLarge,
                    modifier = Modifier.padding(vertical = 16.dp)
                )
                
                Text(
                    text = "参加者数: ${users.size}名",
                    style = MaterialTheme.typography.bodyLarge,
                    modifier = Modifier.padding(top = 8.dp)
                )
            }
        }
        
        // 参加者リストとスコア入力
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .weight(1f)
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    // 左手操作用にボタンを左側に配置
                    val scope = rememberCoroutineScope()
                    Button(
                        onClick = {
                            try {
                                // スコアを数値に変換して保存
                                val scoreMap = userScores.mapValues { (userId, value) -> 
                                    val score = value.toIntOrNull() ?: 0
                                    score
                                }
                                // 全員が0なら登録しない
                                if (scoreMap.values.all { it == 0 }) {
                                    infoMessage = "全員0のため登録しません"
                                    kotlinx.coroutines.MainScope().launch {
                                        delay(2000)
                                        infoMessage = null
                                    }
                                } else {
                    val scoreId = appState.addScoreRecord(sessionId, scoreMap)
                                    lastScoreId = scoreId
                    if (db != null) {
                                        val ts = System.currentTimeMillis()
                                        scope.launch {
                        appState.persistNewScoreRecord(db, sessionId, scoreId, scoreMap, ts)
                        // 合計値も同期
                        appState.persistSessionTotals(db, sessionId)
                                        }
                                    }
                                    showSuccessMessage = true

                                    // スコア登録後に最新のユーザー情報を反映
                                    val updatedUsers = appState.getSessionUsers(sessionId)
                                    // スコア入力フィールドを空に初期化
                                    updatedUsers.forEach { user ->
                                        userScores[user.id] = ""
                                    }

                                    // 3秒後にメッセージを非表示にする
                                    kotlinx.coroutines.MainScope().launch {
                                        delay(3000)
                                        showSuccessMessage = false
                                    }
                                }
                            } catch (e: Exception) {
                                println("スコア登録エラー: ${e.message}")
                            }
                        }
                    ) {
                        Icon(Icons.Filled.Add, contentDescription = "登録")
                        Spacer(Modifier.width(4.dp))
                        Text("スコア登録")
                    }
                    
                    Text(
                        text = "参加者スコア",
                        style = MaterialTheme.typography.titleMedium
                    )
                }
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    if (showSuccessMessage && lastScoreId != null) {
                        Text(
                            text = "スコア登録完了（ID: $lastScoreId）",
                            color = MaterialTheme.colorScheme.primary,
                            style = MaterialTheme.typography.bodyMedium,
                            modifier = Modifier.padding(top = 8.dp)
                        )
                    }
                    if (infoMessage != null) {
                        Text(
                            text = infoMessage!!,
                            color = MaterialTheme.colorScheme.secondary,
                            style = MaterialTheme.typography.bodyMedium,
                            modifier = Modifier.padding(top = 8.dp)
                        )
                    }
                }
                
                Spacer(Modifier.height(8.dp))
                
                LazyColumn(
                    modifier = Modifier.weight(1f),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    items(users) { user ->
                        // 参加者情報とスコア入力欄（左手操作用に左側配置）
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(vertical = 4.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            // スコア入力欄を左側に配置
                            OutlinedTextField(
                                value = userScores[user.id] ?: "",
                                onValueChange = { value ->
                                    // 数字のみ許可
                                    if (value.isEmpty() || value.all { it.isDigit() }) {
                                        userScores[user.id] = value
                                    }
                                },
                                label = { Text("新スコア") },
                                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                                modifier = Modifier.width(100.dp),
                                singleLine = true
                            )
                            
                            Spacer(Modifier.width(8.dp))
                            
                            Icon(
                                Icons.Default.Person,
                                contentDescription = "ユーザー",
                                modifier = Modifier.padding(end = 8.dp)
                            )
                            
                            Column(
                                modifier = Modifier.weight(1f)
                            ) {
                                Text(
                                    text = user.name,
                                    style = MaterialTheme.typography.bodyLarge
                                )
                                // スコア履歴から合計スコアを計算して表示
                                val totalScore = appState.getScoreHistory(sessionId).sumOf { record ->
                                    record.scores[user.id] ?: 0
                                }
                                Text(
                                    text = "現在スコア: $totalScore",
                                    style = MaterialTheme.typography.bodySmall,
                                    color = MaterialTheme.colorScheme.secondary
                                )
                            }
                        }
                        Divider()
                    }
                }
            }
        }
        
        // スコア履歴表示切り替えボタン
        OutlinedButton(
            onClick = { showHistory = !showHistory },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(if (showHistory) "履歴を閉じる" else "スコア履歴を表示")
        }
        
        // スコア履歴表示部分
        if (showHistory) {
            ScoreHistorySection(
                scoreHistory = appState.getScoreHistory(sessionId),
                users = users
            )
        }
    }
}

@Composable
fun ScoreHistorySection(
    scoreHistory: List<ScoreRecord>,
    users: List<User>
) {
    val dateFormat = SimpleDateFormat("MM/dd HH:mm", Locale.getDefault())
    
    Card(
        modifier = Modifier.fillMaxWidth()
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {
            Text(
                text = "スコア履歴",
                style = MaterialTheme.typography.titleMedium,
                modifier = Modifier.padding(bottom = 8.dp)
            )
            
            if (scoreHistory.isEmpty()) {
                Text("履歴がありません", modifier = Modifier.padding(vertical = 8.dp))
            } else {
                // ヘッダー行
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    Text(
                        text = "ID",
                        style = MaterialTheme.typography.bodyMedium,
                        modifier = Modifier.width(40.dp)
                    )
                    
                    Text(
                        text = "時刻",
                        style = MaterialTheme.typography.bodyMedium,
                        modifier = Modifier.width(100.dp)
                    )
                    
                    users.forEach { user ->
                        Text(
                            text = user.name,
                            style = MaterialTheme.typography.bodyMedium,
                            modifier = Modifier.weight(1f)
                        )
                    }
                }
                
                Divider(modifier = Modifier.padding(vertical = 4.dp))
                
                // 合計行
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(vertical = 8.dp),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    Text(
                        text = "合計",
                        style = MaterialTheme.typography.bodyMedium,
                        modifier = Modifier.width(40.dp)
                    )
                    
                    Text(
                        text = "",
                        style = MaterialTheme.typography.bodyMedium,
                        modifier = Modifier.width(100.dp)
                    )
                    
                    users.forEach { user ->
                        val totalScore = scoreHistory.sumOf { record ->
                            record.scores[user.id] ?: 0
                        }
                        Text(
                            text = totalScore.toString(),
                            style = MaterialTheme.typography.bodyMedium,
                            modifier = Modifier.weight(1f),
                            color = MaterialTheme.colorScheme.primary
                        )
                    }
                }
                
                Divider(modifier = Modifier.padding(vertical = 4.dp))
                
                // 履歴データ（最新順）
                LazyColumn(
                    modifier = Modifier.heightIn(max = 200.dp),
                    verticalArrangement = Arrangement.spacedBy(4.dp)
                ) {
                    items(scoreHistory.reversed()) { record ->
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.spacedBy(8.dp)
                        ) {
                            // スコアID
                            Text(
                                text = "#${record.id}",
                                style = MaterialTheme.typography.bodySmall,
                                modifier = Modifier.width(40.dp)
                            )
                            
                            Text(
                                text = dateFormat.format(record.timestamp),
                                style = MaterialTheme.typography.bodySmall,
                                modifier = Modifier.width(100.dp)
                            )
                            
                            users.forEach { user ->
                                Text(
                                    text = record.scores[user.id]?.toString() ?: "-",
                                    style = MaterialTheme.typography.bodySmall,
                                    modifier = Modifier.weight(1f)
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}

@Preview(showBackground = true)
@Composable
fun SessionRunningPreview() {
    val appState = AppState()
    val session = appState.addSession("テストセッション")
    appState.addUserToSession(session.id, "ユーザー1")
    appState.addUserToSession(session.id, "ユーザー2")
    
    // テスト用にスコア履歴を追加
    val scores1 = mapOf(1 to 10, 2 to 20)
    val scores2 = mapOf(1 to 15, 2 to 25)
    val scoreId1 = appState.addScoreRecord(session.id, scores1)
    val scoreId2 = appState.addScoreRecord(session.id, scores2)
    println("テスト用スコアID: $scoreId1, $scoreId2")
    
    TestApp2Theme {
        SessionRunningScreen(
            appState = appState,
            sessionId = session.id
        )
    }
}
