package com.example.testapp.data

import android.content.Context
import android.content.SharedPreferences
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken

/**
 * データの永続化を管理するRepository
 */
class ScoreRepository(context: Context) {
    
    private val sharedPreferences: SharedPreferences = 
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    private val gson = Gson()
    
    companion object {
        private const val PREFS_NAME = "score_app_prefs"
        private const val KEY_APP_STATE = "app_state"
        private const val KEY_CURRENT_SESSION = "current_session"
        private const val KEY_USERS = "users"
        private const val KEY_GAMES = "games"
        private const val KEY_SCORES = "scores"
        private const val KEY_ALL_SESSIONS = "all_sessions"
    }
    
    /**
     * アプリ状態を保存
     */
    fun saveAppState(appState: AppState) {
        val editor = sharedPreferences.edit()
        
        // AppStateの基本情報を保存
        val stateJson = gson.toJson(
            mapOf(
                "currentStep" to appState.currentStep.name,
                "userCount" to appState.userCount,
                "currentGameIndex" to appState.currentGameIndex
            )
        )
        editor.putString(KEY_APP_STATE, stateJson)
        
        // Current Session を保存
        val currentSessionJson = gson.toJson(appState.currentSession)
        editor.putString(KEY_CURRENT_SESSION, currentSessionJson)
        
        // Users を保存
        val usersJson = gson.toJson(appState.users)
        editor.putString(KEY_USERS, usersJson)
        
        // Games を保存
        val gamesJson = gson.toJson(appState.games)
        editor.putString(KEY_GAMES, gamesJson)
        
        // Scores を保存
        val scoresJson = gson.toJson(appState.scores)
        editor.putString(KEY_SCORES, scoresJson)
        
        // All Sessions を保存
        val allSessionsJson = gson.toJson(appState.allSessions)
        editor.putString(KEY_ALL_SESSIONS, allSessionsJson)
        
        editor.apply()
    }
    
    /**
     * アプリ状態を読み込み
     */
    fun loadAppState(): AppState? {
        return try {
            val stateJson = sharedPreferences.getString(KEY_APP_STATE, null) ?: return null
            val stateMap = gson.fromJson(stateJson, object : TypeToken<Map<String, Any>>() {}.type) as Map<String, Any>
            
            val currentSessionJson = sharedPreferences.getString(KEY_CURRENT_SESSION, null)
            val usersJson = sharedPreferences.getString(KEY_USERS, null)
            val gamesJson = sharedPreferences.getString(KEY_GAMES, null)
            val scoresJson = sharedPreferences.getString(KEY_SCORES, null)
            val allSessionsJson = sharedPreferences.getString(KEY_ALL_SESSIONS, null)
            
            val currentSession = if (currentSessionJson != null && currentSessionJson != "null") {
                gson.fromJson(currentSessionJson, GameSession::class.java)
            } else null
            
            val users = if (usersJson != null) {
                gson.fromJson(usersJson, object : TypeToken<List<User>>() {}.type) ?: emptyList()
            } else emptyList<User>()
            
            val games = if (gamesJson != null) {
                gson.fromJson(gamesJson, object : TypeToken<List<Game>>() {}.type) ?: emptyList()
            } else emptyList<Game>()
            
            val scores = if (scoresJson != null) {
                gson.fromJson(scoresJson, object : TypeToken<List<Score>>() {}.type) ?: emptyList()
            } else emptyList<Score>()
            
            val allSessions = if (allSessionsJson != null) {
                gson.fromJson(allSessionsJson, object : TypeToken<List<GameSession>>() {}.type) ?: emptyList()
            } else emptyList<GameSession>()
            
            AppState(
                currentStep = AppStep.valueOf(stateMap["currentStep"] as String),
                userCount = (stateMap["userCount"] as Double).toInt(),
                currentSession = currentSession,
                users = users,
                games = games,
                scores = scores,
                allSessions = allSessions,
                currentGameIndex = (stateMap["currentGameIndex"] as Double).toInt()
            )
        } catch (e: Exception) {
            e.printStackTrace()
            null
        }
    }
    
    /**
     * 保存されたデータをクリア
     */
    fun clearData() {
        sharedPreferences.edit().clear().apply()
    }
    
    /**
     * 保存されたデータが存在するかチェック
     */
    fun hasSavedData(): Boolean {
        return sharedPreferences.getString(KEY_APP_STATE, null) != null
    }
}
