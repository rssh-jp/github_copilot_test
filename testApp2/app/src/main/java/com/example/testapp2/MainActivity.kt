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
import androidx.activity.compose.BackHandler
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
            MenuType.SESSION_LIST -> Screen.SessionList
        }
        scope.launch { drawerState.close() }
    }
    
    // セッション詳細画面に移動
    val navigateToSessionDetail: (Int) -> Unit = { sessionId ->
        currentScreen = Screen.SessionDetail(sessionId)
    }

    // カテゴリブラウザ画面に移動
    val navigateToCategoryBrowser: (Int, Int?) -> Unit = { sessionId, parentCategoryId ->
        currentScreen = Screen.CategoryBrowser(sessionId, parentCategoryId)
    }

    // セッション実行画面に移動（sectionId 付き）
    val navigateToSessionRunning: (Int, Int?) -> Unit = { sessionId, sectionId ->
        currentScreen = Screen.SessionRunning(sessionId, sectionId)
    }

    // システムの「戻る」ジェスチャー/ボタンをインターセプトしてアプリ内ナビゲーションに委譲
    BackHandler(enabled = currentScreen !is Screen.SessionList) {
        currentScreen = when (val screen = currentScreen) {
            is Screen.SessionDetail  -> Screen.SessionList
            is Screen.CategoryBrowser -> {
                if (screen.parentCategoryId == null) {
                    // ルートカテゴリ → セッション詳細へ
                    Screen.SessionDetail(screen.sessionId)
                } else {
                    // 親カテゴリの parentId を求めて上階層の CategoryBrowser へ
                    val parentCat = appState.categories.find { it.id == screen.parentCategoryId }
                    Screen.CategoryBrowser(screen.sessionId, parentCat?.parentId)
                }
            }
            is Screen.SessionRunning -> {
                if (screen.sectionId != null) {
                    // セクションの親カテゴリを持つ CategoryBrowser へ
                    val section = appState.categories.find { it.id == screen.sectionId }
                    Screen.CategoryBrowser(screen.sessionId, section?.parentId)
                } else {
                    // 後方互換: sectionId なし → SessionDetail へ
                    Screen.SessionDetail(screen.sessionId)
                }
            }
            is Screen.SessionList    -> Screen.SessionList
        }
    }

    ModalNavigationDrawer(
        drawerState = drawerState,
        drawerContent = {
            ModalDrawerSheet {
                Text("メニュー", style = MaterialTheme.typography.titleMedium, modifier = Modifier.padding(16.dp))
                HorizontalDivider()
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
                                is Screen.SessionList -> "セッション一覧"
                                is Screen.SessionDetail -> {
                                    val sessionId = (currentScreen as Screen.SessionDetail).sessionId
                                    val session = appState.sessions.find { it.id == sessionId }
                                    "セッション: ${session?.name ?: ""}"
                                }
                                is Screen.CategoryBrowser -> {
                                    val screen = currentScreen as Screen.CategoryBrowser
                                    if (screen.parentCategoryId == null) {
                                        "カテゴリ"
                                    } else {
                                        val cat = appState.categories.find { it.id == screen.parentCategoryId }
                                        cat?.name ?: "カテゴリ"
                                    }
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
                    // セッション詳細画面/実行画面/カテゴリブラウザの場合は戻るボタンを表示
                    actions = {
                        when (currentScreen) {
                            is Screen.SessionDetail -> {
                                IconButton(onClick = {
                                    currentScreen = Screen.SessionList
                                }) {
                                    Text("戻る")
                                }
                            }
                            is Screen.CategoryBrowser -> {
                                val screen = currentScreen as Screen.CategoryBrowser
                                IconButton(onClick = {
                                    currentScreen = if (screen.parentCategoryId == null) {
                                        Screen.SessionDetail(screen.sessionId)
                                    } else {
                                        val parentCat = appState.categories.find { it.id == screen.parentCategoryId }
                                        Screen.CategoryBrowser(screen.sessionId, parentCat?.parentId)
                                    }
                                }) {
                                    Text("戻る")
                                }
                            }
                            is Screen.SessionRunning -> {
                                val screen = currentScreen as Screen.SessionRunning
                                IconButton(onClick = {
                                    currentScreen = if (screen.sectionId != null) {
                                        val section = appState.categories.find { it.id == screen.sectionId }
                                        Screen.CategoryBrowser(screen.sessionId, section?.parentId)
                                    } else {
                                        Screen.SessionDetail(screen.sessionId)
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
                        onStartSession = { sid -> navigateToSessionRunning(sid, null) },
                        onManageCategories = { sid -> navigateToCategoryBrowser(sid, null) },
                    )
                }
                is Screen.CategoryBrowser -> {
                    val screen = currentScreen as Screen.CategoryBrowser
                    CategoryBrowserScreen(
                        modifier = Modifier.padding(innerPadding),
                        appState = appState,
                        db = db,
                        sessionId = screen.sessionId,
                        parentCategoryId = screen.parentCategoryId,
                        onNavigateToCategory = { sid, parentId -> navigateToCategoryBrowser(sid, parentId) },
                        onNavigateToSection = { sid, sectionId -> navigateToSessionRunning(sid, sectionId) },
                    )
                }
                is Screen.SessionRunning -> {
                    val screen = currentScreen as Screen.SessionRunning
                    SessionRunningScreen(
                        modifier = Modifier.padding(innerPadding),
                        appState = appState,
                        db = db,
                        sessionId = screen.sessionId,
                        sectionId = screen.sectionId,
                    )
                }
            }
        }
    }
}