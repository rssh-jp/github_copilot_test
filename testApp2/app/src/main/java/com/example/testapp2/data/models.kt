package com.example.testapp2.data

import androidx.compose.runtime.mutableStateListOf
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
    val score: Int = 0,
)

// スコア履歴を記録するデータクラス
data class ScoreRecord(
    val id: Int, // スコアレコードのID
    val sessionId: Int,
    val timestamp: Date,
    val scores: Map<Int, Int>, // ユーザーID -> スコア
)

// アプリケーション状態を管理するクラス
class AppState {
    // セッション一覧
    val sessions = mutableStateListOf<Session>()
    // ユーザー一覧
    val users = mutableStateListOf<User>()
    // スコア履歴一覧
    val scoreRecords = mutableStateListOf<ScoreRecord>()

    // セッションごとのユーザー取得
    fun getSessionUsers(sessionId: Int): List<User> {
        return users.filter { it.sessionId == sessionId }
    }

    // セッションごとのスコア履歴取得
    fun getSessionScoreHistory(sessionId: Int): List<ScoreRecord> {
        return scoreRecords.filter { it.sessionId == sessionId }
    }

    // セッションの追加
    fun addSession(name: String): Session {
        val nextId = (sessions.maxOfOrNull { it.id } ?: 0) + 1
        val session = Session(nextId, name, 0)
        sessions.add(session)
        return session
    }

    // セッションにユーザーを追加
    fun addUserToSession(sessionId: Int, userName: String): User {
        val nextId = (users.maxOfOrNull { it.id } ?: 0) + 1
        val user = User(nextId, sessionId, userName)
        users.add(user)
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
        val scoreId = (scoreRecords.maxOfOrNull { it.id } ?: 0) + 1
        val record = ScoreRecord(scoreId, sessionId, Date(), userScores)
        scoreRecords.add(record)
        // 合計値を再計算して反映
        recalcSessionTotals(sessionId)
        return scoreId
    }

    // 特定セッションのスコア履歴を取得
    fun getScoreHistory(sessionId: Int): List<ScoreRecord> {
        return scoreRecords.filter { it.sessionId == sessionId }
    }

    // 特定のスコアレコードを取得
    fun getScoreRecord(sessionId: Int, scoreId: Int): ScoreRecord? {
        val history = getScoreHistory(sessionId)
        return history.find { it.id == scoreId }
    }

    // スコアレコードを編集
    fun updateScoreRecord(sessionId: Int, scoreId: Int, newScores: Map<Int, Int>): Boolean {
        val index = scoreRecords.indexOfFirst { it.sessionId == sessionId && it.id == scoreId }
        if (index == -1) return false
        scoreRecords[index] = scoreRecords[index].copy(scores = newScores)
        // 合計値を再計算して反映
        recalcSessionTotals(sessionId)
        return true
    }

    // スコアレコードを削除
    fun deleteScoreRecord(sessionId: Int, scoreId: Int): Boolean {
        val index = scoreRecords.indexOfFirst { it.sessionId == sessionId && it.id == scoreId }
        if (index == -1) return false
        scoreRecords.removeAt(index)
        // 合計値を再計算して反映
        recalcSessionTotals(sessionId)
        return true
    }

    // 指定セッションのユーザー合計スコアを履歴から再計算して反映
    private fun recalcSessionTotals(sessionId: Int) {
        val totals = mutableMapOf<Int, Int>()
        scoreRecords
            .filter { it.sessionId == sessionId }
            .forEach { record ->
                record.scores.forEach { (userId, delta) ->
                    totals[userId] = (totals[userId] ?: 0) + delta
                }
            }

        for (i in users.indices) {
            val u = users[i]
            if (u.sessionId == sessionId) {
                users[i] = u.copy(score = totals[u.id] ?: 0)
            }
        }
    }
}
