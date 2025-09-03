package com.example.testapp

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.testapp.ui.theme.TestAppTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            TestAppTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    ScoreApp(
                        modifier = Modifier.padding(innerPadding)
                    )
                }
            }
        }
    }
}

@Composable
fun ScoreApp(
    modifier: Modifier = Modifier
) {
    val context = LocalContext.current
    val dbHelper = ScoreDatabaseHelper(context)
    val dao = SQLiteDao(dbHelper)
    val repository = ScoreRepository(dao)
    val scoreViewModel = viewModel<ScoreViewModel> { 
        ScoreViewModel(repository) 
    }
    
    // StateFlowの状態を収集
    val currentScreen by scoreViewModel.currentScreen.collectAsState()
    val currentSession by scoreViewModel.currentSession.collectAsState()
    val sessions by scoreViewModel.sessions.collectAsState()
    val currentSessionUsers by scoreViewModel.currentSessionUsers.collectAsState()
    val message by scoreViewModel.message.collectAsState()
    
    val snackbarHostState = remember { SnackbarHostState() }
    
    // メッセージが更新されたときにSnackbarを表示
    LaunchedEffect(message) {
        message?.let {
            snackbarHostState.showSnackbar(it)
            scoreViewModel.clearMessage()
        }
    }
    
    Scaffold(
        snackbarHost = { SnackbarHost(snackbarHostState) },
        modifier = modifier
    ) { paddingValues ->
    // 画面状態に応じて適切な画面を表示
    when (currentScreen) {
        AppScreen.SESSION_SELECT -> {
            SessionSelectScreen(
                availableSessions = sessions,
                onSelectSession = { session ->
                    scoreViewModel.navigateToScoreInput(session)
                },
                onCreateNewSession = {
                    scoreViewModel.navigateToSessionCreate()
                },
                onViewAllHistory = {
                    scoreViewModel.navigateToHistory()
                },
                onDataRestore = {
                    // データ復元の処理
                },
                modifier = Modifier.padding(paddingValues)
            )
        }
        
        AppScreen.SESSION_CREATE -> {
            SessionCreationScreen(
                onCreateSession = { sessionName, userNames ->
                    scoreViewModel.createNewSession(sessionName, userNames)
                },
                onBack = {
                    scoreViewModel.navigateToSessionSelect()
                },
                modifier = Modifier.padding(paddingValues)
            )
        }
        
        AppScreen.SCORE_INPUT -> {
            currentSession?.let { session ->
                if (currentSessionUsers.isEmpty()) {
                    // ユーザーデータが読み込まれるまでローディング表示
                    Column(
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(paddingValues),
                        horizontalAlignment = Alignment.CenterHorizontally,
                        verticalArrangement = Arrangement.Center
                    ) {
                        CircularProgressIndicator()
                        Spacer(modifier = Modifier.height(16.dp))
                        Text("ユーザー情報を読み込んでいます...")
                    }
                } else {
                    ScoreInputScreen(
                        users = currentSessionUsers.map { user ->
                            DataUser(
                                id = user.id.toString(),
                                name = user.name,
                                totalScore = 0,
                                gameCount = 0,
                                averageScore = 0.0
                            )
                        },
                        gameNumber = scoreViewModel.getCurrentGameNumber(),
                        onScoreSubmit = { userScores ->
                            // スコア保存の処理
                            val gameId = scoreViewModel.createGame(session.id)
                            val scoreList = userScores.mapIndexed { index, score ->
                                Pair(currentSessionUsers[index].id, score ?: 0)
                            }
                            scoreViewModel.saveGameScores(gameId, scoreList)
                            scoreViewModel.showMessage("スコアを登録しました！")
                            // スコア保存後にセッション選択画面に戻る
                            scoreViewModel.navigateToSessionSelect()
                        },
                        onCancel = {
                            scoreViewModel.navigateToSessionSelect()
                        },
                        modifier = Modifier.padding(paddingValues)
                    )
                }
            }
        }
        
        AppScreen.HISTORY -> {
            HistoryScreen(
                sessions = sessions,
                onBackToMain = {
                    scoreViewModel.navigateToSessionSelect()
                },
                onEnterSession = { sessionId ->
                    val session = sessions.find { it.id == sessionId }
                    session?.let { scoreViewModel.navigateToScoreInput(it) }
                },
                modifier = Modifier.padding(paddingValues)
            )
        }
    }
}
}
