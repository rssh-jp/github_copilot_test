package com.example.testapp2

import com.example.testapp2.data.AppState
import org.junit.Test
import org.junit.Assert.*

/**
 * AppState のビジネスロジックに関するユニットテスト
 */
class AppStateTest {

    @Test
    fun セッションを追加するとセッション一覧に反映される() {
        val appState = AppState()
        val session = appState.addSession("テストセッション")
        assertEquals(1, appState.sessions.size)
        assertEquals("テストセッション", session.name)
        assertTrue(appState.sessions.contains(session))
    }

    @Test
    fun ユーザーを追加するとユーザー一覧に反映される() {
        val appState = AppState()
        val session = appState.addSession("テスト")
        val user = appState.addUserToSession(session.id, "プレイヤー1")
        assertEquals(1, appState.getSessionUsers(session.id).size)
        assertEquals("プレイヤー1", user.name)
        assertEquals(session.id, user.sessionId)
    }

    @Test
    fun スコアを登録すると合計スコアが計算される() {
        val appState = AppState()
        val session = appState.addSession("テスト")
        val user = appState.addUserToSession(session.id, "プレイヤー1")
        appState.addScoreRecord(session.id, mapOf(user.id to 10))
        appState.addScoreRecord(session.id, mapOf(user.id to 20))
        val updatedUser = appState.getSessionUsers(session.id).first()
        assertEquals(30, updatedUser.score)
    }

    @Test
    fun セッションを削除すると関連データもすべて削除される() {
        val appState = AppState()
        val session = appState.addSession("テスト")
        val user = appState.addUserToSession(session.id, "プレイヤー1")
        appState.addScoreRecord(session.id, mapOf(user.id to 10))
        appState.deleteSessionLocal(session.id)
        assertEquals(0, appState.sessions.size)
        assertEquals(0, appState.users.size)
        assertEquals(0, appState.scoreRecords.size)
    }

    @Test
    fun getScoreHistoryとgetSessionScoreHistoryが同じ結果を返す() {
        val appState = AppState()
        val session = appState.addSession("テスト")
        val user = appState.addUserToSession(session.id, "プレイヤー1")
        appState.addScoreRecord(session.id, mapOf(user.id to 5))
        assertEquals(
            appState.getSessionScoreHistory(session.id),
            appState.getScoreHistory(session.id)
        )
    }

    @Test
    fun ユーザーを削除するとユーザー一覧から消える() {
        val appState = AppState()
        val session = appState.addSession("テスト")
        val user = appState.addUserToSession(session.id, "プレイヤー1")
        assertEquals(1, appState.getSessionUsers(session.id).size)
        appState.removeUserLocal(user.id)
        assertEquals(0, appState.getSessionUsers(session.id).size)
    }

    @Test
    fun セッションをコピーするとユーザーが引き継がれる() {
        val appState = AppState()
        val original = appState.addSession("オリジナル")
        appState.addUserToSession(original.id, "プレイヤー1")
        appState.addUserToSession(original.id, "プレイヤー2")
        val copied = appState.copySession(original.id, "コピー")
        assertNotNull(copied)
        assertEquals(2, appState.getSessionUsers(copied!!.id).size)
    }

    @Test
    fun スコアレコードを削除すると合計スコアが再計算される() {
        val appState = AppState()
        val session = appState.addSession("テスト")
        val user = appState.addUserToSession(session.id, "プレイヤー1")
        val scoreId = appState.addScoreRecord(session.id, mapOf(user.id to 15))
        appState.addScoreRecord(session.id, mapOf(user.id to 10))
        // 1ラウンド目を削除
        appState.deleteScoreRecord(session.id, scoreId)
        val updatedUser = appState.getSessionUsers(session.id).first()
        assertEquals(10, updatedUser.score)
    }
}
