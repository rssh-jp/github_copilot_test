package com.example.testapp2

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material.icons.filled.Person
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import com.example.testapp2.ui.theme.TestApp2Theme

// セッション情報を保存するデータクラス
data class Session(
    val id: Int,
    val name: String
)

// ユーザー情報を保存するデータクラス
data class User(
    val id: Int,
    val name: String,
    val score: Int = 0
)

// アプリケーション状態を管理するクラス
class AppState {
    val sessions = mutableStateListOf<Session>()
    val sessionUsers = mutableStateMapOf<Int, List<User>>()
    var nextSessionId = 1
    var nextUserId = 1

    fun addSession(name: String): Session {
        val session = Session(nextSessionId++, name)
        sessions.add(session)
        sessionUsers[session.id] = emptyList()
        return session
    }

    fun addUserToSession(sessionId: Int, userName: String): User {
        val user = User(nextUserId++, userName)
        val currentUsers = sessionUsers[sessionId] ?: emptyList()
        sessionUsers[sessionId] = currentUsers + user
        return user
    }
    
    // セッションの更新
    fun updateSession(sessionId: Int, newName: String) {
        val index = sessions.indexOfFirst { it.id == sessionId }
        if (index >= 0) {
            sessions[index] = sessions[index].copy(name = newName)
        }
    }
}

// メニュータイプを定義
enum class MenuType(val title: String) {
    NEW_SESSION("新しいセッション"),
    SESSION_LIST("一覧")
}

// アプリの画面状態を定義
sealed class Screen {
    object NewSession : Screen()
    object SessionList : Screen()
    data class SessionDetail(val sessionId: Int) : Screen()
    data class SessionRunning(val sessionId: Int) : Screen() // セッション実行中の画面
}

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            TestApp2Theme {
                MainScreen()
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen() {
    val drawerState = rememberDrawerState(initialValue = DrawerValue.Closed)
    val scope = rememberCoroutineScope()
    val menuItems = MenuType.values().toList()
    
    // アプリケーション状態を作成・管理
    val appState = remember { AppState() }
    
    // 現在の画面状態 - 初期値を一覧画面に変更
    var currentScreen by remember { mutableStateOf<Screen>(Screen.SessionList) }
    
    // 初期選択メニューも一覧に合わせる
    var selectedMenu by remember { mutableStateOf(MenuType.SESSION_LIST) }

    // メニュー選択時に画面状態を更新
    val onMenuItemSelected: (MenuType) -> Unit = { menuType ->
        selectedMenu = menuType
        currentScreen = when (menuType) {
            MenuType.NEW_SESSION -> Screen.NewSession
            MenuType.SESSION_LIST -> Screen.SessionList
        }
        scope.launch { drawerState.close() }
    }
    
    // セッション詳細画面に移動
    val navigateToSessionDetail: (Int) -> Unit = { sessionId ->
        currentScreen = Screen.SessionDetail(sessionId)
    }

    ModalNavigationDrawer(
        drawerState = drawerState,
        drawerContent = {
            ModalDrawerSheet {
                Text("メニュー", style = MaterialTheme.typography.titleMedium, modifier = Modifier.padding(16.dp))
                Divider()
                menuItems.forEach { menuType ->
                    NavigationDrawerItem(
                        label = { Text(menuType.title) },
                        selected = selectedMenu == menuType,
                        onClick = { onMenuItemSelected(menuType) }
                    )
                }
            }
        }
    ) {
        Scaffold(
            topBar = {
                TopAppBar(
                    title = {
                        Text(
                            when (currentScreen) {
                                is Screen.NewSession -> "新しいセッション"
                                is Screen.SessionList -> "セッション一覧"
                                is Screen.SessionDetail -> {
                                    val sessionId = (currentScreen as Screen.SessionDetail).sessionId
                                    val session = appState.sessions.find { it.id == sessionId }
                                    "セッション: ${session?.name ?: ""}"
                                }
                                is Screen.SessionRunning -> {
                                    val sessionId = (currentScreen as Screen.SessionRunning).sessionId
                                    val session = appState.sessions.find { it.id == sessionId }
                                    "実行中: ${session?.name ?: ""}"
                                }
                            }
                        )
                    },
                    navigationIcon = {
                        IconButton(onClick = { scope.launch { drawerState.open() } }) {
                            Icon(Icons.Filled.Menu, contentDescription = "メニューを開く")
                        }
                    },
                    // セッション詳細画面/実行画面の場合は戻るボタンを表示
                    actions = {
                        when (currentScreen) {
                            is Screen.SessionDetail -> {
                                IconButton(onClick = { 
                                    currentScreen = when (selectedMenu) {
                                        MenuType.NEW_SESSION -> Screen.NewSession
                                        MenuType.SESSION_LIST -> Screen.SessionList
                                    }
                                }) {
                                    Text("戻る")
                                }
                            }
                            is Screen.SessionRunning -> {
                                IconButton(onClick = { 
                                    // 実行画面から詳細画面に戻る
                                    val sessionId = (currentScreen as Screen.SessionRunning).sessionId
                                    currentScreen = Screen.SessionDetail(sessionId)
                                }) {
                                    Text("詳細に戻る")
                                }
                            }
                            else -> {}
                        }
                    }
                )
            },
            modifier = Modifier.fillMaxSize()
        ) { innerPadding ->
            when (currentScreen) {
                is Screen.NewSession -> NewSessionScreen(
                    modifier = Modifier.padding(innerPadding),
                    appState = appState,
                    onSessionCreated = navigateToSessionDetail
                )
                is Screen.SessionList -> SessionListScreen(
                    modifier = Modifier.padding(innerPadding),
                    appState = appState,
                    onSessionSelected = navigateToSessionDetail
                )
                is Screen.SessionDetail -> {
                    val sessionId = (currentScreen as Screen.SessionDetail).sessionId
                    SessionDetailScreen(
                        modifier = Modifier.padding(innerPadding),
                        appState = appState,
                        sessionId = sessionId,
                        onStartSession = { sid -> currentScreen = Screen.SessionRunning(sid) }
                    )
                }
                is Screen.SessionRunning -> {
                    val sessionId = (currentScreen as Screen.SessionRunning).sessionId
                    SessionRunningScreen(
                        modifier = Modifier.padding(innerPadding),
                        appState = appState,
                        sessionId = sessionId
                    )
                }
            }
        }
    }
}

@Composable
fun NewSessionScreen(
    modifier: Modifier = Modifier, 
    appState: AppState,
    onSessionCreated: (Int) -> Unit
) {
    var sessionName by remember { mutableStateOf("") }

    Column(
        modifier = modifier.padding(16.dp).fillMaxSize(),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        // セッション作成UI
        Text(
            text = "新しいセッションを作成",
            style = MaterialTheme.typography.headlineMedium,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        OutlinedTextField(
            value = sessionName,
            onValueChange = { sessionName = it },
            label = { Text("セッション名") },
            placeholder = { Text("新しいセッション名を入力") },
            modifier = Modifier.fillMaxWidth().padding(vertical = 8.dp)
        )
        
        Button(
            onClick = {
                if (sessionName.isNotBlank()) {
                    val session = appState.addSession(sessionName)
                    // セッション詳細画面に遷移
                    onSessionCreated(session.id)
                }
            },
            modifier = Modifier.padding(vertical = 16.dp)
        ) {
            Text("セッションを作成して次へ")
        }
    }
}

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

@Composable
fun SessionItem(
    session: Session, 
    users: List<User>,
    onClick: () -> Unit = {}
) {
    var expanded by remember { mutableStateOf(false) }
    
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp)
            .clickable { onClick() }
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = session.name,
                    style = MaterialTheme.typography.titleMedium
                )
                
                Text(
                    text = "参加者: ${users.size}名",
                    style = MaterialTheme.typography.bodyMedium
                )
            }
            
            TextButton(onClick = { expanded = !expanded }) {
                Text(if (expanded) "詳細を隠す" else "詳細を表示")
            }
            
            if (expanded && users.isNotEmpty()) {
                Divider(modifier = Modifier.padding(vertical = 8.dp))
                
                Column {
                    users.forEach { user ->
                        UserItem(user = user)
                    }
                }
            }
            
            Text(
                text = "タップしてセッション詳細を表示",
                style = MaterialTheme.typography.bodySmall,
                modifier = Modifier.align(Alignment.End).padding(top = 8.dp),
                color = MaterialTheme.colorScheme.secondary
            )
        }
    }
}

@Composable
fun UserItem(user: User) {
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
            style = MaterialTheme.typography.bodyLarge
        )
        if (user.score > 0) {
            Spacer(modifier = Modifier.weight(1f))
            Text(
                text = "${user.score}点",
                style = MaterialTheme.typography.bodyMedium
            )
        }
    }
}

@Composable
fun SessionDetailScreen(
    modifier: Modifier = Modifier,
    appState: AppState,
    sessionId: Int,
    onStartSession: (Int) -> Unit = {}
) {
    val session = appState.sessions.find { it.id == sessionId }
    val users = appState.sessionUsers[sessionId] ?: emptyList()
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
fun NewSessionPreview() {
    TestApp2Theme {
        NewSessionScreen(
            appState = AppState(),
            onSessionCreated = {}
        )
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