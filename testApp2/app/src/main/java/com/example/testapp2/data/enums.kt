package com.example.testapp2.data

// メニュータイプを定義
enum class MenuType(val title: String) {
    NEW_SESSION("新しいセッション"),
    SESSION_LIST("一覧")
}

// アプリの画面状態を定義
sealed class Screen {
    object NewSession : Screen()
    object SessionList : Screen()
    data class SessionDetail(val sessionId: Int) : Screen()
    data class SessionRunning(val sessionId: Int) : Screen() // セッション実行中の画面
}
