package com.example.testapp2

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertIsEnabled
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithContentDescription
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.Assert.*

/**
 * インストゥルメンテーションテスト（Androidデバイス上で実行）
 */
@RunWith(AndroidJUnit4::class)
class ExampleInstrumentedTest {

    @get:Rule
    val composeTestRule = createAndroidComposeRule<MainActivity>()

    @Test
    fun アプリのPackageNameが正しいことを確認する() {
        // テスト対象アプリのコンテキストを取得
        val appContext = InstrumentationRegistry.getInstrumentation().targetContext
        assertEquals("jp.rssh.testapp2", appContext.packageName)
    }

    @Test
    fun 起動時にセッション一覧画面が表示される() {
        // TopAppBar のタイトルが「セッション一覧」であることを確認
        composeTestRule.onNodeWithText("セッション一覧").assertIsDisplayed()
    }

    @Test
    fun 新規セッションボタンが表示されて有効である() {
        composeTestRule.onNodeWithText("新規セッション").assertIsDisplayed()
        composeTestRule.onNodeWithText("新規セッション").assertIsEnabled()
    }

    @Test
    fun メニューアイコンが表示される() {
        composeTestRule.onNodeWithContentDescription("メニューを開く").assertIsDisplayed()
    }

    @Test
    fun 新規セッションボタンをタップするとセッション詳細画面に遷移する() {
        composeTestRule.onNodeWithText("新規セッション").performClick()
        // セッション詳細画面のUI要素「セッションを開始」ボタンが表示されることを確認
        composeTestRule.onNodeWithText("参加者を追加してください").assertIsDisplayed()
    }
}
