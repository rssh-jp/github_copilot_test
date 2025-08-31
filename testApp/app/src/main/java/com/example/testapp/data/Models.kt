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
 * スコアのデータクラス
 */
data class Score(
    val id: String = UUID.randomUUID().toString(),
    val userId: String,
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
    val scores: List<Score> = emptyList(),
    val currentUserIndex: Int = 0
)

/**
 * アプリのステップを表す列挙型
 */
enum class AppStep {
    USER_COUNT_INPUT,    // ユーザー数入力
    USER_NAME_INPUT,     // ユーザー名入力
    SCORE_INPUT,         // スコア入力
    SCORE_HISTORY        // スコア履歴表示
}
