package com.example.testapp2.data

import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateMapOf
import java.util.Date

// セッション情報を保存するデータクラス
data class Session(
    val id: Int,
    val name: String,
    val elapsedTime: Int,
)

// ユーザー情報を保存するデータクラス
data class User(
    val id: Int,
    val sessionId: Int,
    val name: String,
    val score: Int = 0
)

// スコア履歴を記録するデータクラス
data class ScoreRecord(
    val id: Int,           // スコアレコードのID
    val timestamp: Date,
    val scores: Map<Int, Int> // ユーザーID -> スコア
)

// アプリケーション状態を管理するクラス
class AppState {
    val sessions = mutableStateListOf<Session>()
    val sessionUsers = mutableStateMapOf<Int, List<User>>()
    // セッションごとのスコア履歴を保存
    val sessionScoreHistory = mutableStateMapOf<Int, List<ScoreRecord>>()
    var nextSessionId = 1
    var nextUserId = 1
    var nextScoreId = 1

    fun addSession(name: String): Session {
        val session = Session(nextSessionId++, name, 0)
        sessions.add(session)
        sessionUsers[session.id] = emptyList()
        sessionScoreHistory[session.id] = emptyList()
        return session
    }

    fun addUserToSession(sessionId: Int, userName: String): User {
        val user = User(nextUserId++, sessionId, userName)
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
    
    // スコア記録を追加
    fun addScoreRecord(sessionId: Int, userScores: Map<Int, Int>): Int {
        val scoreId = nextScoreId++
        val record = ScoreRecord(scoreId, Date(), userScores)
        val currentHistory = sessionScoreHistory[sessionId] ?: emptyList()
        sessionScoreHistory[sessionId] = currentHistory + record
        
        // ユーザーのスコアも更新
        val updatedUsers = sessionUsers[sessionId]?.map { user ->
            userScores[user.id]?.let { newScore ->
                user.copy(score = newScore)
            } ?: user
        }
        updatedUsers?.let { sessionUsers[sessionId] = it }
        
        return scoreId
    }
    
    // 特定セッションのスコア履歴を取得
    fun getScoreHistory(sessionId: Int): List<ScoreRecord> {
        return sessionScoreHistory[sessionId] ?: emptyList()
    }
    
    // 特定のスコアレコードを取得
    fun getScoreRecord(sessionId: Int, scoreId: Int): ScoreRecord? {
        val history = getScoreHistory(sessionId)
        return history.find { it.id == scoreId }
    }
    
    // スコアレコードを編集
    fun updateScoreRecord(sessionId: Int, scoreId: Int, newScores: Map<Int, Int>): Boolean {
        val history = getScoreHistory(sessionId).toMutableList()
        val index = history.indexOfFirst { it.id == scoreId }
        if (index == -1) return false
        
        // 元のレコードを更新して置き換え
        val oldRecord = history[index]
        val updatedRecord = oldRecord.copy(scores = newScores)
        history[index] = updatedRecord
        
        // 更新した履歴をセット
        sessionScoreHistory[sessionId] = history
        return true
    }
    
    // スコアレコードを削除
    fun deleteScoreRecord(sessionId: Int, scoreId: Int): Boolean {
        val history = getScoreHistory(sessionId).toMutableList()
        val removed = history.removeIf { it.id == scoreId }
        if (removed) {
            sessionScoreHistory[sessionId] = history
            return true
        }
        return false
    }
}
