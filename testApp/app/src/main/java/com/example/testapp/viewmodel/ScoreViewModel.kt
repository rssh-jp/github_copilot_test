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
    
    // 現在入力中のスコア値
    var currentScoreValue by mutableStateOf("")
        private set
    
    // 現在入力中のスコア説明
    var currentScoreDescription by mutableStateOf("")
        private set
    
    /**
     * ユーザー数を設定
     */
    fun setUserCount(count: Int) {
        if (count > 0) {
            appState = appState.copy(
                userCount = count,
                currentStep = AppStep.USER_NAME_INPUT,
                currentUserIndex = 0
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
            
            appState = appState.copy(
                users = updatedUsers,
                currentUserIndex = appState.currentUserIndex + 1
            )
            
            currentUserName = ""
            
            // 全ユーザーが入力されたらスコア入力ステップに移行
            if (updatedUsers.size >= appState.userCount) {
                appState = appState.copy(
                    currentStep = AppStep.SCORE_INPUT,
                    currentUserIndex = 0
                )
            }
        }
    }
    
    /**
     * 現在入力中のスコア値を更新
     */
    fun updateCurrentScoreValue(value: String) {
        currentScoreValue = value
    }
    
    /**
     * 現在入力中のスコア説明を更新
     */
    fun updateCurrentScoreDescription(description: String) {
        currentScoreDescription = description
    }
    
    /**
     * スコアを追加
     */
    fun addScore() {
        val scoreValue = currentScoreValue.toIntOrNull()
        if (scoreValue != null && appState.currentUserIndex < appState.users.size) {
            val currentUser = appState.users[appState.currentUserIndex]
            val newScore = Score(
                userId = currentUser.id,
                value = scoreValue,
                description = currentScoreDescription.trim()
            )
            
            val updatedScores = appState.scores + newScore
            appState = appState.copy(scores = updatedScores)
            
            // 入力フィールドをクリア
            currentScoreValue = ""
            currentScoreDescription = ""
        }
    }
    
    /**
     * 次のユーザーに移動
     */
    fun nextUser() {
        if (appState.currentUserIndex < appState.users.size - 1) {
            appState = appState.copy(currentUserIndex = appState.currentUserIndex + 1)
        }
    }
    
    /**
     * 前のユーザーに移動
     */
    fun previousUser() {
        if (appState.currentUserIndex > 0) {
            appState = appState.copy(currentUserIndex = appState.currentUserIndex - 1)
        }
    }
    
    /**
     * スコア履歴画面に移動
     */
    fun navigateToScoreHistory() {
        appState = appState.copy(currentStep = AppStep.SCORE_HISTORY)
    }
    
    /**
     * スコア入力画面に戻る
     */
    fun navigateToScoreInput() {
        appState = appState.copy(currentStep = AppStep.SCORE_INPUT)
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
     * 特定のスコアを削除
     */
    fun deleteScore(scoreId: String) {
        appState = appState.copy(
            scores = appState.scores.filter { it.id != scoreId }
        )
    }
    
    /**
     * 特定のスコアを編集
     */
    fun editScore(scoreId: String, newValue: Int, newDescription: String) {
        appState = appState.copy(
            scores = appState.scores.map { score ->
                if (score.id == scoreId) {
                    score.copy(value = newValue, description = newDescription.trim())
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
        currentScoreValue = ""
        currentScoreDescription = ""
    }
    
    /**
     * 現在のユーザーを取得
     */
    fun getCurrentUser(): User? {
        return if (appState.currentUserIndex < appState.users.size) {
            appState.users[appState.currentUserIndex]
        } else null
    }
    
    /**
     * 特定のユーザーのスコア履歴を取得
     */
    fun getUserScores(userId: String): List<Score> {
        return appState.scores.filter { it.userId == userId }
            .sortedByDescending { it.timestamp }
    }
}
