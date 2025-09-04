package com.example.testapp2.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Person
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.example.testapp2.data.AppState
import com.example.testapp2.ui.theme.TestApp2Theme
import kotlinx.coroutines.delay

@Composable
fun SessionRunningScreen(
    modifier: Modifier = Modifier,
    appState: AppState,
    sessionId: Int
) {
    val session = appState.sessions.find { it.id == sessionId }
    val users = appState.sessionUsers[sessionId] ?: emptyList()
    var currentTime by remember { mutableStateOf(System.currentTimeMillis()) }
    val startTime = remember { System.currentTimeMillis() }
    
    // 時間を更新する効果
    LaunchedEffect(Unit) {
        while(true) {
            delay(1000) // 1秒ごとに更新
            currentTime = System.currentTimeMillis()
        }
    }
    
    val elapsedTime = (currentTime - startTime) / 1000 // 経過時間（秒）
    val hours = elapsedTime / 3600
    val minutes = (elapsedTime % 3600) / 60
    val seconds = elapsedTime % 60
    
    Column(
        modifier = modifier.padding(16.dp).fillMaxSize()
    ) {
        // セッションの基本情報
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 16.dp)
        ) {
            Column(
                modifier = Modifier.padding(16.dp),
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
        
        // 参加者リスト
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .weight(1f)
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "参加者",
                    style = MaterialTheme.typography.titleMedium,
                    modifier = Modifier.padding(bottom = 16.dp)
                )
                
                LazyColumn {
                    items(users) { user ->
                        // 参加者情報と得点の入力欄
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(vertical = 8.dp),
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
                            
                            // 得点表示
                            Text(
                                text = "${user.score}点",
                                style = MaterialTheme.typography.bodyLarge
                            )
                        }
                        Divider()
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
    
    TestApp2Theme {
        SessionRunningScreen(
            appState = appState,
            sessionId = session.id
        )
    }
}
