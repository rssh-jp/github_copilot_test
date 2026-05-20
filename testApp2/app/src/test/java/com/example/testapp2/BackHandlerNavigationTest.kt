package com.example.testapp2

import com.example.testapp2.data.Category
import com.example.testapp2.data.Screen
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * BackHandler のナビゲーションロジックに関するユニットテスト
 *
 * テスト対象のロジック（MainActivity.kt より抜粋）:
 *   BackHandler(
 *       enabled = currentScreen !is Screen.CategoryBrowser ||
 *           (currentScreen as Screen.CategoryBrowser).categoryId != null
 *   ) {
 *       currentScreen = when (val screen = currentScreen) {
 *           is Screen.SessionDetail  -> Screen.CategoryBrowser(session?.categoryId)
 *           is Screen.SessionRunning -> Screen.SessionDetail(screen.sessionId)
 *           is Screen.CategoryBrowser -> Screen.CategoryBrowser(parentId)
 *       }
 *   }
 */
class BackHandlerNavigationTest {

    /** テスト用のカテゴリリスト（親子関係の確認用） */
    private val testCategories = listOf(
        Category(id = 1, parentId = null, name = "ルートカテゴリ"),
        Category(id = 2, parentId = 1, name = "子カテゴリ"),
    )

    /**
     * MainActivity の BackHandler 内部と同じナビゲーション解決ロジックを純粋関数として再現する。
     */
    private fun resolveBackNavigation(
        currentScreen: Screen,
        sessionCategoryId: Int? = null,
        categories: List<Category> = testCategories
    ): Screen {
        return when (val screen = currentScreen) {
            is Screen.SessionDetail -> Screen.CategoryBrowser(sessionCategoryId)
            is Screen.SessionRunning -> Screen.SessionDetail(screen.sessionId)
            is Screen.CategoryBrowser -> {
                val parentId = categories.find { it.id == screen.categoryId }?.parentId
                Screen.CategoryBrowser(parentId)
            }
        }
    }

    /**
     * BackHandler の enabled 条件と同じ式を評価する。
     * ルートの CategoryBrowser（categoryId=null）のとき false（無効）、
     * それ以外のとき true（有効）であることを検証する。
     */
    private fun isBackHandlerEnabled(currentScreen: Screen): Boolean {
        return currentScreen !is Screen.CategoryBrowser ||
            (currentScreen as Screen.CategoryBrowser).categoryId != null
    }

    // -------------------------------------------------------------------------
    // テストケース 1: SessionDetail 画面での戻る操作
    // -------------------------------------------------------------------------

    @Test
    fun `SessionDetail画面で戻る操作をするとCategoryBrowserに遷移する`() {
        // 準備: sessionId = 42 の SessionDetail 画面（categoryId=null のルートセッション）
        val currentScreen: Screen = Screen.SessionDetail(sessionId = 42)

        // 実行: 戻るナビゲーションを解決
        val nextScreen = resolveBackNavigation(currentScreen, sessionCategoryId = null)

        // 検証: CategoryBrowser(null) に遷移すること
        assertEquals(
            "SessionDetail から戻ると CategoryBrowser(null) になるべき",
            Screen.CategoryBrowser(null),
            nextScreen
        )
    }

    @Test
    fun `SessionDetailのカテゴリ付きセッションで戻るとそのカテゴリのCategoryBrowserに遷移する`() {
        val currentScreen: Screen = Screen.SessionDetail(sessionId = 42)
        val nextScreen = resolveBackNavigation(currentScreen, sessionCategoryId = 1)

        assertEquals(
            "categoryId=1 のセッション SessionDetail から戻ると CategoryBrowser(1) になるべき",
            Screen.CategoryBrowser(1),
            nextScreen
        )
    }

    // -------------------------------------------------------------------------
    // テストケース 2: SessionRunning 画面での戻る操作
    // -------------------------------------------------------------------------

    @Test
    fun `SessionRunning画面で戻る操作をするとsessionIdを引き継いだSessionDetailに遷移する`() {
        // 準備: sessionId = 7 の SessionRunning 画面
        val sessionId = 7
        val currentScreen: Screen = Screen.SessionRunning(sessionId = sessionId)

        // 実行: 戻るナビゲーションを解決
        val nextScreen = resolveBackNavigation(currentScreen)

        // 検証: 同じ sessionId を持つ SessionDetail に遷移すること
        assertTrue(
            "SessionRunning から戻ると SessionDetail になるべき",
            nextScreen is Screen.SessionDetail
        )
        assertEquals(
            "SessionDetail の sessionId は元の sessionId と同じであるべき",
            sessionId,
            (nextScreen as Screen.SessionDetail).sessionId
        )
    }

    @Test
    fun `SessionRunningから戻るときにsessionIdが正確に引き継がれる`() {
        // 複数の sessionId で sessionId の引き継ぎを確認
        listOf(1, 99, 1000).forEach { id ->
            val nextScreen = resolveBackNavigation(Screen.SessionRunning(sessionId = id))
            val expectedScreen = Screen.SessionDetail(sessionId = id)
            assertEquals(
                "sessionId=$id の SessionRunning から戻ると SessionDetail(sessionId=$id) になるべき",
                expectedScreen,
                nextScreen
            )
        }
    }

    // -------------------------------------------------------------------------
    // テストケース 3: CategoryBrowser 画面での戻る操作
    // -------------------------------------------------------------------------

    @Test
    fun `ルートCategoryBrowser画面ではBackHandlerが無効になっている`() {
        // 準備: categoryId=null のルートカテゴリブラウザ
        val currentScreen: Screen = Screen.CategoryBrowser(null)

        // 実行: enabled 条件を評価
        val enabled = isBackHandlerEnabled(currentScreen)

        // 検証: BackHandler は無効（false）であること
        assertFalse(
            "ルート CategoryBrowser 画面では BackHandler は無効（enabled=false）であるべき",
            enabled
        )
    }

    @Test
    fun `子CategoryBrowser画面ではBackHandlerが有効になっている`() {
        val currentScreen: Screen = Screen.CategoryBrowser(categoryId = 2)
        val enabled = isBackHandlerEnabled(currentScreen)
        assertTrue(
            "子カテゴリの CategoryBrowser 画面では BackHandler は有効（enabled=true）であるべき",
            enabled
        )
    }

    @Test
    fun `子CategoryBrowser画面で戻ると親カテゴリのCategoryBrowserに遷移する`() {
        // id=2 の子カテゴリ（parentId=1）から戻る
        val currentScreen: Screen = Screen.CategoryBrowser(categoryId = 2)
        val nextScreen = resolveBackNavigation(currentScreen)

        assertEquals(
            "子カテゴリブラウザから戻ると親カテゴリのブラウザになるべき",
            Screen.CategoryBrowser(1),
            nextScreen
        )
    }

    @Test
    fun `ルート直下カテゴリのCategoryBrowser画面で戻るとルートCategoryBrowserに遷移する`() {
        // id=1 の子カテゴリ（parentId=null）から戻る → ルートへ
        val currentScreen: Screen = Screen.CategoryBrowser(categoryId = 1)
        val nextScreen = resolveBackNavigation(currentScreen)

        assertEquals(
            "ルート直下カテゴリのブラウザから戻るとルートCategoryBrowserになるべき",
            Screen.CategoryBrowser(null),
            nextScreen
        )
    }

    @Test
    fun `SessionDetail画面ではBackHandlerが有効になっている`() {
        val currentScreen: Screen = Screen.SessionDetail(sessionId = 1)
        val enabled = isBackHandlerEnabled(currentScreen)
        assertTrue(
            "SessionDetail 画面では BackHandler は有効（enabled=true）であるべき",
            enabled
        )
    }

    @Test
    fun `SessionRunning画面ではBackHandlerが有効になっている`() {
        val currentScreen: Screen = Screen.SessionRunning(sessionId = 1)
        val enabled = isBackHandlerEnabled(currentScreen)
        assertTrue(
            "SessionRunning 画面では BackHandler は有効（enabled=true）であるべき",
            enabled
        )
    }
}
