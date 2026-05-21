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
import com.example.testapp2.data.Screen
import com.example.testapp2.ui.components.CategoryTreeDrawerContent
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

    // アプリケーション状態を作成・管理
    val appState = remember { AppState() }
    val context = LocalContext.current
    val db = remember { AppDatabase.get(context) }
    var isLoaded by remember { mutableStateOf(false) }
    LaunchedEffect(Unit) {
        appState.loadFromDb(db)
        isLoaded = true
    }
    
    // 現在の画面状態 - 初期値をカテゴリブラウザ（ルート）に設定
    var currentScreen by remember { mutableStateOf<Screen>(Screen.CategoryBrowser(null)) }

    // 展開中のカテゴリIDセット（初期値は空、LaunchedEffect で新規追加分のみ展開に追加）
    var expandedCategoryIds by remember { mutableStateOf(setOf<Int>()) }
    // 前回のカテゴリIDセットを追跡（新規追加分の差分計算に使用）
    var prevCategoryIds by remember { mutableStateOf(setOf<Int>()) }

    // カテゴリが変化したとき、新規追加分のIDのみ展開に追加（ユーザーの折りたたみ操作はリセットしない）
    val currentCategoryIds = appState.categories.map { it.id }.toSet()
    LaunchedEffect(currentCategoryIds) {
        val addedIds = currentCategoryIds - prevCategoryIds
        expandedCategoryIds = expandedCategoryIds + addedIds
        prevCategoryIds = currentCategoryIds
    }

    // 現在表示中のカテゴリID（CategoryBrowser 画面以外は null）
    val selectedCategoryId: Int? = when (val s = currentScreen) {
        is Screen.CategoryBrowser -> s.categoryId
        else -> null
    }

    // CategoryBrowserScreen から SessionDetail に遷移するコールバックは
    // CategoryBrowserScreen の onNavigateToSession から直接処理する

    // システムの「戻る」ジェスチャー/ボタンをインターセプトしてアプリ内ナビゲーションに委譲
    // ルートカテゴリブラウザ（categoryId=null）以外で有効
    BackHandler(
        enabled = currentScreen !is Screen.CategoryBrowser ||
            (currentScreen as Screen.CategoryBrowser).categoryId != null
    ) {
        currentScreen = when (val screen = currentScreen) {
            is Screen.SessionDetail -> {
                // セッションが属するカテゴリへ戻る
                val session = appState.sessions.find { it.id == screen.sessionId }
                Screen.CategoryBrowser(session?.categoryId)
            }
            is Screen.SessionRunning -> Screen.SessionDetail(screen.sessionId)
            is Screen.CategoryBrowser -> {
                // 親カテゴリへ戻る
                val parentId = appState.categories.find { it.id == screen.categoryId }?.parentId
                Screen.CategoryBrowser(parentId)
            }
        }
    }

    ModalNavigationDrawer(
        drawerState = drawerState,
        drawerContent = {
            ModalDrawerSheet {
                Text("メニュー", style = MaterialTheme.typography.titleMedium, modifier = Modifier.padding(16.dp))
                HorizontalDivider()
                CategoryTreeDrawerContent(
                    appState = appState,
                    selectedCategoryId = selectedCategoryId,
                    expandedCategoryIds = expandedCategoryIds,
                    onCategoryClick = { category ->
                        currentScreen = Screen.CategoryBrowser(category.id)
                        scope.launch { drawerState.close() }
                    },
                    onToggleExpand = { id ->
                        expandedCategoryIds = if (id in expandedCategoryIds)
                            expandedCategoryIds - id
                        else
                            expandedCategoryIds + id
                    }
                )
            }
        }
    ) {
        Scaffold(
            topBar = {
                TopAppBar(
                    title = {
                        Text(
                            when (val screen = currentScreen) {
                                is Screen.CategoryBrowser -> {
                                    val name = screen.categoryId?.let { id ->
                                        appState.categories.find { it.id == id }?.name
                                    }
                                    // ルート時は「得点集計」、子カテゴリ時はカテゴリ名を表示
                                    name ?: "得点集計"
                                }
                                is Screen.SessionDetail -> {
                                    val sessionId = screen.sessionId
                                    val session = appState.sessions.find { it.id == sessionId }
                                    "セッション: ${session?.name ?: ""}"
                                }
                                is Screen.SessionRunning -> {
                                    val sessionId = screen.sessionId
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
                        when (val screen = currentScreen) {
                            is Screen.SessionDetail -> {
                                IconButton(onClick = {
                                    val session = appState.sessions.find { it.id == screen.sessionId }
                                    currentScreen = Screen.CategoryBrowser(session?.categoryId)
                                }) {
                                    Text("戻る")
                                }
                            }
                            is Screen.SessionRunning -> {
                                IconButton(onClick = {
                                    currentScreen = Screen.SessionDetail(screen.sessionId)
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
            } else when (val screen = currentScreen) {
                is Screen.CategoryBrowser -> CategoryBrowserScreen(
                    modifier = Modifier.padding(innerPadding),
                    appState = appState,
                    db = db,
                    categoryId = screen.categoryId,
                    onNavigateToCategory = { id -> currentScreen = Screen.CategoryBrowser(id) },
                    onNavigateToSession = { sid -> currentScreen = Screen.SessionDetail(sid) }
                )
                is Screen.SessionDetail -> {
                    SessionDetailScreen(
                        modifier = Modifier.padding(innerPadding),
                        appState = appState,
                        db = db,
                        sessionId = screen.sessionId,
                        onStartSession = { sid -> currentScreen = Screen.SessionRunning(sid) },
                        onNavigateToSession = { sid -> currentScreen = Screen.SessionDetail(sid) }
                    )
                }
                is Screen.SessionRunning -> {
                    SessionRunningScreen(
                        modifier = Modifier.padding(innerPadding),
                        appState = appState,
                        db = db,
                        sessionId = screen.sessionId
                    )
                }
            }
        }
    }
}