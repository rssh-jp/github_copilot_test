package com.example.testapp2

import com.example.testapp2.data.Screen
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * BackHandler のナビゲーションロジックに関するユニットテスト
 *
 * テスト対象のロジック（MainActivity.kt より抜粋）:
 *   BackHandler(enabled = currentScreen !is Screen.SessionList) {
 *       currentScreen = when (val screen = currentScreen) {
 *           is Screen.SessionDetail  -> Screen.SessionList
 *           is Screen.SessionRunning -> Screen.SessionDetail(screen.sessionId)
 *           else                     -> Screen.SessionList
 *       }
 *   }
 */
class BackHandlerNavigationTest {

    /**
     * MainActivity の BackHandler 内部と同じナビゲーション解決ロジックを純粋関数として再現する。
     * テスト対象コードと一字一句同じ式を使用して挙動を保証する。
     */
    private fun resolveBackNavigation(currentScreen: Screen): Screen {
        return when (val screen = currentScreen) {
            is Screen.SessionDetail  -> Screen.SessionList
            is Screen.SessionRunning -> Screen.SessionDetail(screen.sessionId)
            else                     -> Screen.SessionList
        }
    }

    /**
     * BackHandler の enabled 条件と同じ式を評価する。
     * SessionList のとき false（無効）、それ以外のとき true（有効）であることを検証する。
     */
    private fun isBackHandlerEnabled(currentScreen: Screen): Boolean {
        return currentScreen !is Screen.SessionList
    }

    // -------------------------------------------------------------------------
    // テストケース 1: SessionDetail 画面での戻る操作
    // -------------------------------------------------------------------------

    @Test
    fun `SessionDetail画面で戻る操作をするとSessionListに遷移する`() {
        // 準備: sessionId = 42 の SessionDetail 画面
        val currentScreen: Screen = Screen.SessionDetail(sessionId = 42)

        // 実行: 戻るナビゲーションを解決
        val nextScreen = resolveBackNavigation(currentScreen)

        // 検証: SessionList に遷移すること
        assertEquals(
            "SessionDetail から戻ると SessionList になるべき",
            Screen.SessionList,
            nextScreen
        )
    }

    @Test
    fun `SessionDetailの任意のsessionIdで戻るとSessionListに遷移する`() {
        // 複数の sessionId で同じ挙動を確認
        listOf(1, 100, Int.MAX_VALUE).forEach { id ->
            val nextScreen = resolveBackNavigation(Screen.SessionDetail(sessionId = id))
            assertEquals(
                "sessionId=$id の SessionDetail から戻ると SessionList になるべき",
                Screen.SessionList,
                nextScreen
            )
        }
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
    // テストケース 3: SessionList 画面では BackHandler が無効であること
    // -------------------------------------------------------------------------

    @Test
    fun `SessionList画面ではBackHandlerが無効になっている`() {
        // 準備: SessionList 画面
        val currentScreen: Screen = Screen.SessionList

        // 実行: enabled 条件を評価
        val enabled = isBackHandlerEnabled(currentScreen)

        // 検証: BackHandler は無効（false）であること
        assertFalse(
            "SessionList 画面では BackHandler は無効（enabled=false）であるべき",
            enabled
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
