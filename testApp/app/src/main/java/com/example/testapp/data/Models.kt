package com.example.testapp.data

import java.util.*

/**
 * ゲームセッション（一連のゲームをまとめる単位）
 */
data class GameSession(
    val id: String = UUID.randomUUID().toString(),
    val name: String,
    val users: List<User>,
    val startTime: Long = System.currentTimeMillis(),
    val endTime: Long? = null,
    val isActive: Boolean = true
)

/**
 * ユーザーのデータクラス
 */
data class User(
    val id: String = UUID.randomUUID().toString(),
    val name: String,
    val sessionId: String = "",
    val createdAt: Long = System.currentTimeMillis()
)

/**
 * ゲーム（ラウンド）のデータクラス
 */
data class Game(
    val id: String = UUID.randomUUID().toString(),
    val sessionId: String,
    val name: String,
    val timestamp: Long = System.currentTimeMillis()
)

/**
 * スコアのデータクラス
 */
data class Score(
    val id: String = UUID.randomUUID().toString(),
    val userId: String,
    val gameId: String,
    val sessionId: String,
    val value: Int,
    val timestamp: Long = System.currentTimeMillis(),
    val description: String = ""
)

/**
 * スコア履歴のデータクラス（表示用）
 */
data class ScoreHistory(
    val user: User,
    val scores: List<Score>,
    val totalScore: Int = scores.sumOf { it.value }
)

/**
 * アプリの状態を表すデータクラス
 */
data class AppState(
    val currentStep: AppStep = AppStep.USER_COUNT_INPUT,
    val userCount: Int = 0,
    val currentSession: GameSession? = null,
    val users: List<User> = emptyList(),
    val games: List<Game> = emptyList(),
    val scores: List<Score> = emptyList(),
    val allSessions: List<GameSession> = emptyList(),
    val currentGameIndex: Int = 0
)

/**
 * アプリのステップを表す列挙型
 */
enum class AppStep {
    USER_COUNT_INPUT,    // ユーザー数入力
    USER_NAME_INPUT,     // ユーザー名入力
    MAIN_SCREEN,         // メイン画面（ユーザー一覧とスコア管理）
    GAME_SCORE_INPUT,    // ゲームスコア入力
    HISTORY_VIEW         // 全履歴表示
}
