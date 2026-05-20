package com.example.testapp2.data

// メニュータイプを定義
enum class MenuType(val title: String) {
    CATEGORY_BROWSER("ブラウザ")
}

// アプリの画面状態を定義
sealed class Screen {
    /** カテゴリブラウザ画面（categoryId=null でルート表示） */
    data class CategoryBrowser(val categoryId: Int?) : Screen()
    data class SessionDetail(val sessionId: Int) : Screen()
    data class SessionRunning(val sessionId: Int) : Screen()
}
