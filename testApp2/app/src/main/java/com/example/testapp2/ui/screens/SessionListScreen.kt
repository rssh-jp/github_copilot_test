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
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.example.testapp2.data.AppState
import com.example.testapp2.ui.components.SessionItem
import com.example.testapp2.ui.theme.TestApp2Theme

@Composable
fun SessionListScreen(
    modifier: Modifier = Modifier, 
    appState: AppState,
    onSessionSelected: (Int) -> Unit
) {
    Column(
        modifier = modifier.padding(16.dp).fillMaxSize()
    ) {
        // ヘッダー部分（タイトルと新規セッション作成ボタン）
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 16.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = "セッション一覧",
                style = MaterialTheme.typography.headlineMedium
            )
            
            // 新しいセッション作成ボタン
            Button(
                onClick = { 
                    // 新しいセッション画面に遷移する (MainScreenのcurrentScreenを変更する代わりに)
                    // まず新しいセッションを作成してすぐに詳細画面に遷移させる
                    val session = appState.addSession("")  // 空の名前で作成し、詳細画面で編集可能に
                    onSessionSelected(session.id)
                }
            ) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Icon(Icons.Default.Add, contentDescription = "追加")
                    Spacer(modifier = Modifier.width(4.dp))
                    Text("新規セッション")
                }
            }
        }
        
        if (appState.sessions.isEmpty()) {
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f),
                contentAlignment = Alignment.Center
            ) {
                Column(
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.Center
                ) {
                    Text(
                        text = "セッションがまだ作成されていません",
                        style = MaterialTheme.typography.bodyLarge,
                        textAlign = TextAlign.Center
                    )
                    
                    Spacer(modifier = Modifier.height(16.dp))
                    
                    Text(
                        text = "「新規セッション」ボタンをタップして\n新しいセッションを作成しましょう",
                        style = MaterialTheme.typography.bodyMedium,
                        textAlign = TextAlign.Center,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
        } else {
            LazyColumn(
                modifier = Modifier.weight(1f)
            ) {
                items(appState.sessions) { session ->
                    SessionItem(
                        session = session,
                        users = appState.sessionUsers[session.id] ?: emptyList(),
                        onClick = { onSessionSelected(session.id) }
                    )
                }
            }
        }
    }
}

@Preview(showBackground = true)
@Composable
fun SessionListPreview() {
    val appState = AppState()
    val session = appState.addSession("テストセッション")
    appState.addUserToSession(session.id, "ユーザー1")
    appState.addUserToSession(session.id, "ユーザー2")
    
    TestApp2Theme {
        SessionListScreen(
            appState = appState,
            onSessionSelected = {}
        )
    }
}
