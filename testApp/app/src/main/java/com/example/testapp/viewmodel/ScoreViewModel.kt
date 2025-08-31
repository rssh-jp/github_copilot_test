package com.example.testapp.viewmodel

import android.app.Application
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import com.example.testapp.data.*

/**
 * スコア管理アプリのメインViewModel
 */
class ScoreViewModel(application: Application) : AndroidViewModel(application) {
    
    private val repository = ScoreRepository(application)
    
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
    
    init {
        // アプリ起動時に保存されたデータを読み込み
        loadSavedData()
    }
    
    /**
     * 保存されたデータがあるかチェック
     */
    fun hasSavedData(): Boolean {
        return repository.hasSavedData()
    }
    
    /**
     * 新しいゲームを開始（保存データをクリア）
     */
    fun startNewGame() {
        repository.clearData()
        appState = AppState()
        currentUserName = ""
        gameScoreInputs = mapOf()
        currentGameName = ""
    }
    
    
    /**
     * 保存されたデータを読み込み
     */
    private fun loadSavedData() {
        repository.loadAppState()?.let { savedState ->
            appState = savedState
        }
    }
    
    /**
     * データを保存
     */
    private fun saveData() {
        repository.saveAppState(appState)
    }
    
    /**
     * ユーザー数を設定（新しいセッション開始）
     */
    fun setUserCount(count: Int) {
        if (count > 0) {
            appState = appState.copy(
                userCount = count,
                currentStep = AppStep.USER_NAME_INPUT
            )
            saveData()
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
            // 新しいセッションを作成（まだ作成されていない場合）
            val currentSession = appState.currentSession ?: run {
                val newSession = GameSession(
                    name = "ゲームセッション ${appState.allSessions.size + 1}",
                    users = emptyList()
                )
                appState = appState.copy(
                    currentSession = newSession,
                    allSessions = appState.allSessions + newSession
                )
                newSession
            }
            
            val newUser = User(
                name = currentUserName.trim(),
                sessionId = currentSession.id
            )
            val updatedUsers = appState.users + newUser
            
            // セッションのユーザーリストも更新
            val updatedSession = currentSession.copy(users = updatedUsers)
            
            appState = appState.copy(
                users = updatedUsers,
                currentSession = updatedSession,
                allSessions = appState.allSessions.map { 
                    if (it.id == updatedSession.id) updatedSession else it 
                }
            )
            currentUserName = ""
            
            // 全ユーザーが入力されたらメイン画面に移行
            if (updatedUsers.size >= appState.userCount) {
                appState = appState.copy(currentStep = AppStep.MAIN_SCREEN)
            }
            
            saveData()
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
        if (currentGameName.isNotBlank() && appState.currentSession != null) {
            // 新しいゲームを作成
            val newGame = Game(
                sessionId = appState.currentSession!!.id,
                name = currentGameName.trim()
            )
            val updatedGames = appState.games + newGame
            
            // 各ユーザーのスコアを作成
            val newScores = gameScoreInputs.mapNotNull { (userId, scoreValue) ->
                val score = scoreValue.toIntOrNull()
                if (score != null) {
                    Score(
                        userId = userId,
                        gameId = newGame.id,
                        sessionId = appState.currentSession!!.id,
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
            
            saveData()
        }
    }
    
    /**
     * メイン画面に戻る
     */
    fun navigateToMainScreen() {
        appState = appState.copy(currentStep = AppStep.MAIN_SCREEN)
        gameScoreInputs = mapOf()
        currentGameName = ""
        saveData()
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
     * ゲーム毎のスコア履歴を取得（現在のセッションのみ）
     */
    fun getGameScoreHistory(): List<Triple<Game, List<Pair<User, Score?>>, Int>> {
        val currentSessionId = appState.currentSession?.id ?: return emptyList()
        
        return appState.games
            .filter { it.sessionId == currentSessionId }
            .sortedByDescending { it.timestamp }
            .map { game ->
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
     * 全セッションの履歴を取得
     */
    fun getAllSessionsHistory(): List<GameSession> {
        return appState.allSessions.sortedByDescending { it.startTime }
    }
    
    /**
     * 特定セッションのスコア履歴を取得
     */
    fun getSessionScoreHistory(sessionId: String): List<Triple<Game, List<Pair<User, Score?>>, Int>> {
        val sessionUsers = appState.allSessions.find { it.id == sessionId }?.users ?: emptyList()
        
        return appState.games
            .filter { it.sessionId == sessionId }
            .sortedByDescending { it.timestamp }
            .map { game ->
                val gameScores = appState.scores.filter { it.gameId == game.id }
                val userScorePairs = sessionUsers.map { user ->
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
        saveData()
    }
    
    /**
     * 特定のゲームを削除
     */
    fun deleteGame(gameId: String) {
        appState = appState.copy(
            games = appState.games.filter { it.id != gameId },
            scores = appState.scores.filter { it.gameId != gameId }
        )
        saveData()
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
        saveData()
    }
    
    /**
     * アプリをリセット（最初から開始）
     */
    fun resetApp() {
        repository.clearData()
        appState = AppState()
        currentUserName = ""
        gameScoreInputs = mapOf()
        currentGameName = ""
    }
    
    /**
     * 新しいゲームセッションを開始（履歴は保持）
     */
    fun startNewGameSession() {
        // 現在のセッションを終了
        appState.currentSession?.let { currentSession ->
            val endedSession = currentSession.copy(
                isActive = false,
                endTime = System.currentTimeMillis()
            )
            appState = appState.copy(
                allSessions = appState.allSessions.map { 
                    if (it.id == endedSession.id) endedSession else it 
                }
            )
        }
        
        // 新しいゲーム用に現在のセッション情報をリセット（履歴は保持）
        appState = appState.copy(
            currentStep = AppStep.USER_COUNT_INPUT,
            userCount = 0,
            currentSession = null,
            users = emptyList(),
            currentGameIndex = 0
        )
        
        currentUserName = ""
        gameScoreInputs = mapOf()
        currentGameName = ""
        
        saveData()
    }
    
    /**
     * 履歴表示画面に移動
     */
    fun navigateToHistoryView() {
        appState = appState.copy(currentStep = AppStep.HISTORY_VIEW)
    }
}
