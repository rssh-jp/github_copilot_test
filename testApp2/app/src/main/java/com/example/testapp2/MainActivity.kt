package com.example.testapp2

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.Alignment
import androidx.compose.ui.unit.dp
import com.example.testapp2.data.AppState
import com.example.testapp2.data.MenuType
import com.example.testapp2.data.Screen
import com.example.testapp2.data.db.AppDatabase
import com.example.testapp2.ui.screens.*
import com.example.testapp2.ui.theme.TestApp2Theme
import kotlinx.coroutines.launch
import androidx.compose.ui.platform.LocalContext

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
    val context = LocalContext.current
    val db = remember { AppDatabase.get(context) }
    var isLoaded by remember { mutableStateOf(false) }
    LaunchedEffect(Unit) {
        appState.loadFromDb(db)
        isLoaded = true
    }
    
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
                            else -> {}
                        }
                    }
                )
            },
            modifier = Modifier.fillMaxSize()
        ) { innerPadding ->
            if (!isLoaded) {
                // 初回ロード中インジケーター
                Box(modifier = Modifier.padding(innerPadding).fillMaxSize(), contentAlignment = Alignment.Center) {
                    CircularProgressIndicator()
                }
            } else when (currentScreen) {
                is Screen.NewSession -> NewSessionScreen(
                    modifier = Modifier.padding(innerPadding),
                    appState = appState,
                    db = db,
                    onSessionCreated = navigateToSessionDetail
                )
                is Screen.SessionList -> SessionListScreen(
                    modifier = Modifier.padding(innerPadding),
                    appState = appState,
                    db = db,
                    onSessionSelected = navigateToSessionDetail
                )
                is Screen.SessionDetail -> {
                    val sessionId = (currentScreen as Screen.SessionDetail).sessionId
                    SessionDetailScreen(
                        modifier = Modifier.padding(innerPadding),
                        appState = appState,
                        db = db,
                        sessionId = sessionId,
                        onStartSession = { sid -> currentScreen = Screen.SessionRunning(sid) }
                    )
                }
                is Screen.SessionRunning -> {
                    val sessionId = (currentScreen as Screen.SessionRunning).sessionId
                    SessionRunningScreen(
                        modifier = Modifier.padding(innerPadding),
                        appState = appState,
                        db = db,
                        sessionId = sessionId
                    )
                }
            }
        }
    }
}