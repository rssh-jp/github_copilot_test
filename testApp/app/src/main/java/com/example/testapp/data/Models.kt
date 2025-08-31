package com.example.testapp.data

import java.util.*

/**
 * ユーザーのデータクラス
 */
data class User(
    val id: String = UUID.randomUUID().toString(),
    val name: String,
    val createdAt: Long = System.currentTimeMillis()
)

/**
 * ゲーム（ラウンド）のデータクラス
 */
data class Game(
    val id: String = UUID.randomUUID().toString(),
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
    val users: List<User> = emptyList(),
    val games: List<Game> = emptyList(),
    val scores: List<Score> = emptyList(),
    val currentGameIndex: Int = 0
)

/**
 * アプリのステップを表す列挙型
 */
enum class AppStep {
    USER_COUNT_INPUT,    // ユーザー数入力
    USER_NAME_INPUT,     // ユーザー名入力
    MAIN_SCREEN,         // メイン画面（ユーザー一覧とスコア管理）
    GAME_SCORE_INPUT     // ゲームスコア入力
}
