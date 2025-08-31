package com.example.testapp.viewmodel

import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import com.example.testapp.data.*

/**
 * スコア管理アプリのメインViewModel
 */
class ScoreViewModel : ViewModel() {
    
    // アプリの状態
    var appState by mutableStateOf(AppState())
        private set
    
    // 現在入力中のユーザー名
    var currentUserName by mutableStateOf("")
        private set
    
    // ゲームスコア入力時の各ユーザーのスコア値
    var gameScoreInputs by mutableStateOf(mapOf<String, String>())
        private set
    
    // 現在のゲーム名
    var currentGameName by mutableStateOf("")
        private set
    
    /**
     * ユーザー数を設定
     */
    fun setUserCount(count: Int) {
        if (count > 0) {
            appState = appState.copy(
                userCount = count,
                currentStep = AppStep.USER_NAME_INPUT
            )
        }
    }
    
    /**
     * 現在入力中のユーザー名を更新
     */
    fun updateCurrentUserName(name: String) {
        currentUserName = name
    }
    
    /**
     * ユーザーを追加
     */
    fun addUser() {
        if (currentUserName.isNotBlank()) {
            val newUser = User(name = currentUserName.trim())
            val updatedUsers = appState.users + newUser
            
            appState = appState.copy(users = updatedUsers)
            currentUserName = ""
            
            // 全ユーザーが入力されたらメイン画面に移行
            if (updatedUsers.size >= appState.userCount) {
                appState = appState.copy(currentStep = AppStep.MAIN_SCREEN)
            }
        }
    }
    
    /**
     * ゲーム名を更新
     */
    fun updateGameName(name: String) {
        currentGameName = name
    }
    
    /**
     * 特定ユーザーのゲームスコア入力値を更新
     */
    fun updateGameScoreInput(userId: String, scoreValue: String) {
        gameScoreInputs = gameScoreInputs + (userId to scoreValue)
    }
    
    /**
     * ゲームスコア入力画面に移動
     */
    fun navigateToGameScoreInput() {
        appState = appState.copy(currentStep = AppStep.GAME_SCORE_INPUT)
        // 全ユーザーのスコア入力を空文字で初期化
        gameScoreInputs = appState.users.associate { it.id to "" }
        currentGameName = "ゲーム ${appState.games.size + 1}"
    }
    
    /**
     * ゲームスコアを保存
     */
    fun saveGameScores() {
        if (currentGameName.isNotBlank()) {
            // 新しいゲームを作成
            val newGame = Game(name = currentGameName.trim())
            val updatedGames = appState.games + newGame
            
            // 各ユーザーのスコアを作成
            val newScores = gameScoreInputs.mapNotNull { (userId, scoreValue) ->
                val score = scoreValue.toIntOrNull()
                if (score != null) {
                    Score(
                        userId = userId,
                        gameId = newGame.id,
                        value = score
                    )
                } else null
            }
            
            appState = appState.copy(
                games = updatedGames,
                scores = appState.scores + newScores,
                currentStep = AppStep.MAIN_SCREEN
            )
            
            // 入力フィールドをクリア
            gameScoreInputs = mapOf()
            currentGameName = ""
        }
    }
    
    /**
     * メイン画面に戻る
     */
    fun navigateToMainScreen() {
        appState = appState.copy(currentStep = AppStep.MAIN_SCREEN)
        gameScoreInputs = mapOf()
        currentGameName = ""
    }
    
    /**
     * スコア履歴を取得
     */
    fun getScoreHistory(): List<ScoreHistory> {
        return appState.users.map { user ->
            val userScores = appState.scores.filter { score -> score.userId == user.id }
                .sortedByDescending { it.timestamp }
            ScoreHistory(user = user, scores = userScores)
        }.sortedByDescending { it.totalScore }
    }
    
    /**
     * ゲーム毎のスコア履歴を取得
     */
    fun getGameScoreHistory(): List<Triple<Game, List<Pair<User, Score?>>, Int>> {
        return appState.games.sortedByDescending { it.timestamp }.map { game ->
            val gameScores = appState.scores.filter { it.gameId == game.id }
            val userScorePairs = appState.users.map { user ->
                val userScore = gameScores.find { it.userId == user.id }
                user to userScore
            }
            val gameTotal = gameScores.sumOf { it.value }
            Triple(game, userScorePairs, gameTotal)
        }
    }
    
    /**
     * 特定のスコアを削除
     */
    fun deleteScore(scoreId: String) {
        appState = appState.copy(
            scores = appState.scores.filter { it.id != scoreId }
        )
    }
    
    /**
     * 特定のゲームを削除
     */
    fun deleteGame(gameId: String) {
        appState = appState.copy(
            games = appState.games.filter { it.id != gameId },
            scores = appState.scores.filter { it.gameId != gameId }
        )
    }
    
    /**
     * 特定のスコアを編集
     */
    fun editScore(scoreId: String, newValue: Int) {
        appState = appState.copy(
            scores = appState.scores.map { score ->
                if (score.id == scoreId) {
                    score.copy(value = newValue)
                } else {
                    score
                }
            }
        )
    }
    
    /**
     * アプリをリセット（最初から開始）
     */
    fun resetApp() {
        appState = AppState()
        currentUserName = ""
        gameScoreInputs = mapOf()
        currentGameName = ""
    }
}
