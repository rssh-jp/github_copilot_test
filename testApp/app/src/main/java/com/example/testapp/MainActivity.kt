package com.example.testapp

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.*
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.lifecycle.viewmodel.compose.viewModel
import com.example.testapp.data.AppStep
import com.example.testapp.ui.screens.*
import com.example.testapp.ui.theme.TestAppTheme
import com.example.testapp.viewmodel.ScoreViewModel

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            TestAppTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    ScoreManagementApp()
                }
            }
        }
    }
}

@Composable
fun ScoreManagementApp(
    viewModel: ScoreViewModel = viewModel()
) {
    val appState = viewModel.appState
    
    // 起動時にデータ復元確認
    if (viewModel.hasSavedData() && appState.currentStep == AppStep.USER_COUNT_INPUT && appState.users.isEmpty()) {
        DataRestoreScreen(
            onRestoreData = {
                // データはすでにloadSavedData()で読み込まれているので何もしない
            },
            onStartNewGameSession = viewModel::startNewGameSession,
            onCompleteReset = viewModel::resetApp
        )
        return
    }
    
    when (appState.currentStep) {
        AppStep.USER_COUNT_INPUT -> {
            UserCountInputScreen(
                onUserCountSet = viewModel::setUserCount
            )
        }
        
        AppStep.USER_NAME_INPUT -> {
            UserNameInputScreen(
                userCount = appState.userCount,
                users = appState.users,
                currentUserName = viewModel.currentUserName,
                onUserNameChange = viewModel::updateCurrentUserName,
                onAddUser = viewModel::addUser
            )
        }
        
        AppStep.MAIN_SCREEN -> {
            MainScreen(
                users = appState.users,
                gameHistory = viewModel.getGameScoreHistory(),
                onAddGameScore = viewModel::navigateToGameScoreInput,
                onDeleteGame = viewModel::deleteGame,
                onEditScore = viewModel::editScore,
                onViewHistory = viewModel::navigateToHistoryView,
                onResetApp = viewModel::resetApp
            )
        }
        
        AppStep.GAME_SCORE_INPUT -> {
            GameScoreInputScreen(
                users = appState.users,
                gameName = viewModel.currentGameName,
                gameScoreInputs = viewModel.gameScoreInputs,
                onGameNameChange = viewModel::updateGameName,
                onScoreInputChange = viewModel::updateGameScoreInput,
                onSaveScores = viewModel::saveGameScores,
                onCancel = viewModel::navigateToMainScreen
            )
        }
        
        AppStep.HISTORY_VIEW -> {
            AllHistoryScreen(
                allSessions = viewModel.getAllSessionsHistory(),
                onGetSessionHistory = viewModel::getSessionScoreHistory,
                onNavigateBack = viewModel::navigateToMainScreen,
                onEditSession = viewModel::editFromHistory,
                onStartNewGame = viewModel::startNewGameFromHistory
            )
        }
    }
}