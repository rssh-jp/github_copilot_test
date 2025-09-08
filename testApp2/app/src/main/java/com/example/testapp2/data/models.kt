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

    // =====================
    // Room I/O (suspend APIs)
    // =====================
    suspend fun loadFromDb(db: com.example.testapp2.data.db.AppDatabase) {
        sessions.clear()
        users.clear()
        scoreRecords.clear()

        // Load sessions and users
        val sessionEntities = db.sessionDao().getAll()
        sessions.addAll(sessionEntities.map { Session(it.id, it.name, it.elapsedTime) })

        val userEntities = db.userDao().getAll()
        users.addAll(userEntities.map { com.example.testapp2.data.User(it.id, it.sessionId, it.name, it.score) })

        // Load score records and items -> reconstruct ScoreRecord(scores map)
        val recordEntities = db.scoreDao().getAllRecords()
        val items = db.scoreDao().getAllItems()
        val itemsByRecord = items.groupBy { it.recordId }
        scoreRecords.addAll(recordEntities.map { rec ->
            val map = itemsByRecord[rec.id]?.associate { it.userId to it.delta } ?: emptyMap()
            ScoreRecord(id = rec.id, sessionId = rec.sessionId, timestamp = Date(rec.timestamp), scores = map)
        })

        // Make sure totals reflect history
        sessions.map { it.id }.forEach { recalcSessionTotals(it) }
    }

    suspend fun persistNewSession(db: com.example.testapp2.data.db.AppDatabase, session: Session): Int {
        val newId = db.sessionDao().insert(
            com.example.testapp2.data.db.SessionEntity(name = session.name, elapsedTime = session.elapsedTime)
        ).toInt()
        // Reflect generated ID in memory
        val idx = sessions.indexOfFirst { it === session }
        if (idx >= 0) sessions[idx] = session.copy(id = newId)
        return newId
    }

    suspend fun persistNewUser(db: com.example.testapp2.data.db.AppDatabase, user: User) {
        val newId = db.userDao().insert(
            com.example.testapp2.data.db.UserEntity(sessionId = user.sessionId, name = user.name, score = user.score)
        ).toInt()
        val idx = users.indexOfFirst { it === user }
        if (idx >= 0) users[idx] = user.copy(id = newId)
    }

    suspend fun persistNewScoreRecord(db: com.example.testapp2.data.db.AppDatabase, sessionId: Int, recordId: Int, userScores: Map<Int, Int>, timestamp: Long) {
        db.scoreDao().insertRecordWithItems(sessionId, recordId, timestamp, userScores)
    }

    suspend fun persistSessionTotals(db: com.example.testapp2.data.db.AppDatabase, sessionId: Int) {
        users.filter { it.sessionId == sessionId }.forEach { u ->
            db.userDao().updateScore(u.id, u.score)
        }
    }

    suspend fun persistUpdateSessionName(db: com.example.testapp2.data.db.AppDatabase, sessionId: Int, name: String) {
        db.sessionDao().updateName(sessionId, name)
    }
}
