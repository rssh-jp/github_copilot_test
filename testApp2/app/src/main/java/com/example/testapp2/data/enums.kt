package com.example.testapp2.data

// メニュータイプを定義
enum class MenuType(val title: String) {
    SESSION_LIST("一覧")
}

// アプリの画面状態を定義
sealed class Screen {
    object SessionList : Screen()
    data class SessionDetail(val sessionId: Int) : Screen()
    // カテゴリ一覧画面（parentCategoryId=null でルート表示）
    data class CategoryBrowser(
        val sessionId: Int,
        val parentCategoryId: Int?,
    ) : Screen()
    // セッション実行中の画面（sectionId: 紐づくセクションID、null なら旧フロー互換）
    data class SessionRunning(
        val sessionId: Int,
        val sectionId: Int? = null,
    ) : Screen()
}
