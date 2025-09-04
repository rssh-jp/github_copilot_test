package com.example.testapp2.data

import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateMapOf

// セッション情報を保存するデータクラス
data class Session(
    val id: Int,
    val name: String
)

// ユーザー情報を保存するデータクラス
data class User(
    val id: Int,
    val name: String,
    val score: Int = 0
)

// アプリケーション状態を管理するクラス
class AppState {
    val sessions = mutableStateListOf<Session>()
    val sessionUsers = mutableStateMapOf<Int, List<User>>()
    var nextSessionId = 1
    var nextUserId = 1

    fun addSession(name: String): Session {
        val session = Session(nextSessionId++, name)
        sessions.add(session)
        sessionUsers[session.id] = emptyList()
        return session
    }

    fun addUserToSession(sessionId: Int, userName: String): User {
        val user = User(nextUserId++, userName)
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
}
