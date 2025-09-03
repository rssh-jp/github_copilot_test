package com.example.testapp.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.*
import kotlinx.coroutines.launch
import com.example.testapp.data.database.ScoreDatabaseHelper
import com.example.testapp.data.dao.SQLiteDao
import com.example.testapp.data.entity.*

class ScoreViewModel(application: Application) : AndroidViewModel(application) {
    private val database = ScoreDatabaseHelper(application)
    private val dao = SQLiteDao(database)
    
    // StateFlowによる状態管理
    private val _currentSession = MutableStateFlow<Session?>(null)
    val currentSession: StateFlow<Session?> = _currentSession.asStateFlow()
    
    private val _users = MutableStateFlow<List<User>>(emptyList())
    val users: StateFlow<List<User>> = _users.asStateFlow()
    
    private val _availableSessions = MutableStateFlow<List<Session>>(emptyList())
    val availableSessions: StateFlow<List<Session>> = _availableSessions.asStateFlow()
    
    private val _isLoading = MutableStateFlow(false)
    val isLoading: StateFlow<Boolean> = _isLoading.asStateFlow()
    
    // 画面状態管理
    var showSessionSelectScreen = true
        private set
    var showScoreInputScreen = false
        private set
    var showHistoryScreen = false
        private set
    
    // ゲームスコア入力状態
    private val _gameScoreInputs = MutableStateFlow<Map<String, Int>>(emptyMap())
    val gameScoreInputs: StateFlow<Map<String, Int>> = _gameScoreInputs.asStateFlow()
    
    init {
        loadInitialData()
    }
    
    private fun loadInitialData() {
        viewModelScope.launch {
            _isLoading.value = true
            try {
                _availableSessions.value = dao.getAllSessions()
                _users.value = dao.getAllUsers()
            } catch (e: Exception) {
                e.printStackTrace()
            } finally {
                _isLoading.value = false
            }
        }
    }
    
    fun createNewSession(sessionName: String, userNames: List<String>) {
        viewModelScope.launch {
            try {
                // ユーザーを作成または取得
                val userIds = mutableListOf<Long>()
                for (userName in userNames) {
                    val existingUser = dao.getAllUsers().find { it.name == userName }
                    if (existingUser != null) {
                        userIds.add(existingUser.id)
                    } else {
                        val newUserId = dao.insertUser(User(0, userName))
                        userIds.add(newUserId)
                    }
                }
                
                // セッション作成
                val session = Session(
                    id = 0,
                    name = sessionName,
                    startTime = System.currentTimeMillis(),
                    endTime = null,
                    isCompleted = false
                )
                val sessionId = dao.insertSession(session)
                
                // セッションユーザー関連付け
                for (userId in userIds) {
                    dao.insertSessionUser(SessionUser(sessionId, userId))
                }
                
                // 作成したセッションに入る
                enterExistingSession(sessionId)
                
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }
    
    fun enterExistingSession(sessionId: Long) {
        viewModelScope.launch {
            try {
                val session = dao.getSessionById(sessionId)
                val sessionUsers = dao.getUsersForSession(sessionId)
                
                _currentSession.value = session
                _users.value = sessionUsers
                
                // ゲームスコア入力を初期化
                _gameScoreInputs.value = sessionUsers.associate { it.name to 0 }
                
                // スコア入力画面に遷移
                showScoreInputScreen = true
                showSessionSelectScreen = false
                showHistoryScreen = false
                
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }
    
    fun updateGameScoreInput(userName: String, score: Int) {
        val currentInputs = _gameScoreInputs.value.toMutableMap()
        currentInputs[userName] = score
        _gameScoreInputs.value = currentInputs
    }
    
    fun saveGameScores(gameName: String) {
        viewModelScope.launch {
            try {
                val session = _currentSession.value ?: return@launch
                val inputs = _gameScoreInputs.value
                
                // ゲーム作成
                val game = Game(
                    id = 0,
                    sessionId = session.id,
                    name = gameName,
                    timestamp = System.currentTimeMillis()
                )
                val gameId = dao.insertGame(game)
                
                // スコア保存
                for ((userName, score) in inputs) {
                    val user = _users.value.find { it.name == userName } ?: continue
                    val gameScore = GameScore(
                        id = 0,
                        gameId = gameId,
                        userId = user.id,
                        score = score
                    )
                    dao.insertGameScore(gameScore)
                }
                
                // スコア入力をリセット
                _gameScoreInputs.value = _users.value.associate { it.name to 0 }
                
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }
    
    fun completeSession() {
        viewModelScope.launch {
            try {
                val session = _currentSession.value ?: return@launch
                val updatedSession = session.copy(
                    endTime = System.currentTimeMillis(),
                    isCompleted = true
                )
                dao.updateSession(updatedSession)
                
                // セッション選択画面に戻る
                navigateToSessionSelect()
                
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }
    
    fun showHistory() {
        showHistoryScreen = true
        showSessionSelectScreen = false
        showScoreInputScreen = false
    }
    
    fun navigateToSessionSelect() {
        showSessionSelectScreen = true
        showScoreInputScreen = false
        showHistoryScreen = false
        _currentSession.value = null
        loadInitialData()
    }
}
