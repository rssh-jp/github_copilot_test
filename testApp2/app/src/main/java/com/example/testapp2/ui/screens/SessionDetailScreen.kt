package com.example.testapp2.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.example.testapp2.data.AppState
import com.example.testapp2.data.User
import com.example.testapp2.ui.components.UserItem
import com.example.testapp2.ui.theme.TestApp2Theme
import kotlinx.coroutines.launch

@Composable
fun SessionDetailScreen(
    modifier: Modifier = Modifier,
    appState: AppState,
    db: com.example.testapp2.data.db.AppDatabase? = null,
    sessionId: Int,
    onStartSession: (Int) -> Unit = {}
) {
    // 新規セッションの場合（sessionId = -1）と既存セッションの場合を分ける
    val isNewSession = sessionId == -1
    val session = if (isNewSession) null else appState.sessions.find { it.id == sessionId }
    val users = if (isNewSession) emptyList() else appState.getSessionUsers(sessionId)
    var userName by remember { mutableStateOf("") }
    
    // 一時的にユーザーリストを保持（新規セッション用）
    val tempUsers = remember { mutableStateListOf<String>() }
    val currentUsers = if (isNewSession) tempUsers else users
    
    // コピー成功メッセージ用のSnackbar
    val snackbarHostState = remember { SnackbarHostState() }
    val scopeSnackbar = rememberCoroutineScope()
    // ユーザー削除用のコルーチンスコープ
    val scopeUserDel = rememberCoroutineScope()
    // セッションコピー用のコルーチンスコープ
    val scopeCopy = rememberCoroutineScope()
    
    Scaffold(
        snackbarHost = { SnackbarHost(snackbarHostState) }
    ) { paddingValues ->
    Column(
        modifier = modifier.padding(16.dp).fillMaxSize().padding(paddingValues)
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
                
                val scopeName = rememberCoroutineScope()
                OutlinedTextField(
                    value = sessionNameEditing,
                    onValueChange = { sessionNameEditing = it },
                    label = { Text("セッション名") },
                    placeholder = { Text("セッション名を入力") },
                    textStyle = MaterialTheme.typography.headlineMedium,
                    modifier = Modifier.fillMaxWidth().padding(bottom = 8.dp)
                )
                
                Text(
                    text = if (isNewSession) "新規セッション" else "セッションID: $sessionId",
                    style = MaterialTheme.typography.bodyMedium
                )
                
                Text(
                    text = "参加者数: ${currentUsers.size}名",
                    style = MaterialTheme.typography.bodyMedium,
                    modifier = Modifier.padding(top = 8.dp)
                )
                
                // 開始ボタン
                if (currentUsers.isNotEmpty()) {
                    Button(
                        onClick = { 
                            if (isNewSession) {
                                // 新規セッションの場合：セッションとユーザーを作成してDB登録
                                if (sessionNameEditing.isNotBlank()) {
                                    val newSession = appState.addSession(sessionNameEditing.trim())
                                    // セッションIDの一時保存
                                    val tempSessionId = newSession.id
                                    
                                    // 一時的なユーザーを新しいセッションに追加（メモリ上）
                                    val addedUsers = mutableListOf<User>()
                                    tempUsers.forEach { userName ->
                                        val user = appState.addUserToSession(tempSessionId, userName)
                                        addedUsers.add(user)
                                    }
                                    
                                    if (db != null) {
                                        scopeName.launch {
                                            // セッションをDBに保存し、新しいIDを取得
                                            val newId = appState.persistNewSession(db, newSession)
                                            
                                            // IDの更新が必要な場合（IDが変わった場合）
                                            if (newId != tempSessionId) {
                                                // 作成したユーザーのセッションIDを新しいIDに更新
                                                val usersToUpdate = appState.users.filter { it.sessionId == tempSessionId }
                                                usersToUpdate.forEachIndexed { index, user ->
                                                    // ユーザーのセッションIDを更新
                                                    val updatedUser = user.copy(sessionId = newId)
                                                    val userIdx = appState.users.indexOf(user)
                                                    if (userIdx >= 0) {
                                                        appState.users[userIdx] = updatedUser
                                                    }
                                                }
                                            }
                                            
                                            // 追加したユーザーをデータベースに保存
                                            val sessionUsers = appState.users.filter { it.sessionId == newId }
                                            for (user in sessionUsers) {
                                                appState.persistNewUser(db, user)
                                            }
                                            onStartSession(newId)
                                        }
                                    } else {
                                        onStartSession(newSession.id)
                                    }
                                }
                            } else {
                                // 既存セッションの場合：セッション名の変更のみ保存
                                if (sessionNameEditing.isNotBlank() && sessionNameEditing != session?.name) {
                                    appState.updateSession(sessionId, sessionNameEditing.trim())
                                    if (db != null) {
                                        scopeName.launch { 
                                            appState.persistUpdateSessionName(db, sessionId, sessionNameEditing.trim())
                                        }
                                    }
                                }
                                onStartSession(sessionId)
                            }
                        },
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
                
                // 既存セッションの場合：セッションコピーボタンを表示
                if (!isNewSession && currentUsers.isNotEmpty()) {
                    OutlinedButton(
                        onClick = {
                            val newName = "${session?.name} (コピー)"
                            if (db != null) {
                                scopeCopy.launch {
                                    val copiedSession = appState.copySessionWithDb(db, sessionId, newName)
                                    if (copiedSession != null) {
                                        scopeSnackbar.launch {
                                            snackbarHostState.showSnackbar(
                                                message = "新しいセッション「${copiedSession.name}」を作成しました",
                                                duration = SnackbarDuration.Short
                                            )
                                        }
                                    }
                                }
                            } else {
                                val copiedSession = appState.copySession(sessionId, newName)
                                if (copiedSession != null) {
                                    scopeSnackbar.launch {
                                        snackbarHostState.showSnackbar(
                                            message = "新しいセッション「${copiedSession.name}」を作成しました",
                                            duration = SnackbarDuration.Short
                                        )
                                    }
                                }
                            }
                        },
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(top = 8.dp)
                    ) {
                        Text("同じメンバーで新しいセッションを作成")
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
                
                // 左手操作用：ボタンと入力フィールドを横並びに配置
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    val scope = rememberCoroutineScope()
                    Button(
                        onClick = {
                            if (userName.isNotBlank()) {
                                if (isNewSession) {
                                    // 新規セッションの場合：一時的なリストに追加
                                    tempUsers.add(userName.trim())
                                } else {
                                    // 既存セッションの場合：通常通りDB登録
                                    val user = appState.addUserToSession(sessionId, userName)
                                    if (db != null) {
                                        scope.launch { appState.persistNewUser(db, user) }
                                    }
                                }
                                userName = ""
                            }
                        },
                        modifier = Modifier.wrapContentWidth()
                    ) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Icon(Icons.Default.Add, contentDescription = "追加")
                            Spacer(modifier = Modifier.width(4.dp))
                            Text("追加")
                        }
                    }
                    
                    OutlinedTextField(
                        value = userName,
                        onValueChange = { userName = it },
                        label = { Text("ユーザー名") },
                        modifier = Modifier.weight(1f)
                    )
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
                
                if (currentUsers.isEmpty()) {
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
                        if (isNewSession) {
                            items(tempUsers) { userName ->
                                // 新規セッション用の簡易ユーザー表示
                                Card(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(vertical = 4.dp)
                                ) {
                                    Row(
                                        modifier = Modifier.padding(16.dp),
                                        verticalAlignment = Alignment.CenterVertically
                                    ) {
                                        Text(
                                            text = userName,
                                            style = MaterialTheme.typography.bodyLarge,
                                            modifier = Modifier.weight(1f)
                                        )
                                        Text(
                                            text = "0点",
                                            style = MaterialTheme.typography.bodyMedium
                                        )
                                        IconButton(onClick = { tempUsers.remove(userName) }) {
                                            Icon(Icons.Default.Delete, contentDescription = "削除")
                                        }
                                    }
                                }
                            }
                        } else {
                            items(users) { user ->
                                UserItem(
                                    user = user,
                                    onDelete = {
                                        if (db != null) {
                                            // deleteUser が内部で removeUserLocal も呼ぶ
                                            scopeUserDel.launch { appState.deleteUser(db, user.id) }
                                        } else {
                                            appState.removeUserLocal(user.id)
                                        }
                                    }
                                )
                                HorizontalDivider()
                            }
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
