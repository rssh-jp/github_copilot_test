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
import com.example.testapp2.ui.components.UserItem
import com.example.testapp2.ui.theme.TestApp2Theme

@Composable
fun SessionDetailScreen(
    modifier: Modifier = Modifier,
    appState: AppState,
    sessionId: Int,
    onStartSession: (Int) -> Unit = {}
) {
    val session = appState.sessions.find { it.id == sessionId }
    val users = appState.getSessionUsers(sessionId)
    var userName by remember { mutableStateOf("") }
    
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
                modifier = Modifier.padding(16.dp)
            ) {
                // セッション名を編集可能に
                var sessionNameEditing by remember { mutableStateOf(session?.name ?: "") }
                
                OutlinedTextField(
                    value = sessionNameEditing,
                    onValueChange = { 
                        sessionNameEditing = it
                        // 新しいupdateSessionメソッドを使用
                        appState.updateSession(sessionId, sessionNameEditing)
                    },
                    label = { Text("セッション名") },
                    placeholder = { Text("セッション名を入力") },
                    textStyle = MaterialTheme.typography.headlineMedium,
                    modifier = Modifier.fillMaxWidth().padding(bottom = 8.dp)
                )
                
                Text(
                    text = "セッションID: $sessionId",
                    style = MaterialTheme.typography.bodyMedium
                )
                
                Text(
                    text = "参加者数: ${users.size}名",
                    style = MaterialTheme.typography.bodyMedium,
                    modifier = Modifier.padding(top = 8.dp)
                )
                
                // 開始ボタン
                if (users.isNotEmpty()) {
                    Button(
                        onClick = { onStartSession(sessionId) },
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(top = 16.dp),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = MaterialTheme.colorScheme.primary
                        )
                    ) {
                        Text(
                            text = "セッションを開始",
                            style = MaterialTheme.typography.titleMedium
                        )
                    }
                } else {
                    Button(
                        onClick = {},
                        enabled = false,
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(top = 16.dp)
                    ) {
                        Text(
                            text = "参加者を追加してください",
                            style = MaterialTheme.typography.titleMedium
                        )
                    }
                }
            }
        }
        
        // ユーザー追加フォーム
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 16.dp)
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "ユーザーの追加",
                    style = MaterialTheme.typography.titleMedium,
                    modifier = Modifier.padding(bottom = 8.dp)
                )
                
                OutlinedTextField(
                    value = userName,
                    onValueChange = { userName = it },
                    label = { Text("ユーザー名") },
                    modifier = Modifier.fillMaxWidth()
                )
                
                Button(
                    onClick = {
                        if (userName.isNotBlank()) {
                            appState.addUserToSession(sessionId, userName)
                            userName = ""
                        }
                    },
                    modifier = Modifier.padding(top = 8.dp)
                ) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Icon(Icons.Default.Add, contentDescription = "追加")
                        Spacer(modifier = Modifier.width(4.dp))
                        Text("ユーザーを追加")
                    }
                }
            }
        }
        
        // 参加者一覧
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .weight(1f)
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "参加者一覧",
                    style = MaterialTheme.typography.titleMedium,
                    modifier = Modifier.padding(bottom = 8.dp)
                )
                
                if (users.isEmpty()) {
                    Box(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(vertical = 32.dp),
                        contentAlignment = Alignment.Center
                    ) {
                        Text(
                            text = "参加者がまだいません",
                            style = MaterialTheme.typography.bodyMedium,
                            textAlign = TextAlign.Center
                        )
                    }
                } else {
                    LazyColumn {
                        items(users) { user ->
                            UserItem(user = user)
                            Divider()
                        }
                    }
                }
            }
        }
    }
}

@Preview(showBackground = true)
@Composable
fun SessionDetailPreview() {
    val appState = AppState()
    val session = appState.addSession("テストセッション")
    appState.addUserToSession(session.id, "ユーザー1")
    appState.addUserToSession(session.id, "ユーザー2")
    
    TestApp2Theme {
        SessionDetailScreen(
            appState = appState,
            sessionId = session.id,
            onStartSession = {}
        )
    }
}
