package com.example.testapp2

import com.example.testapp2.data.AppState
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Assert.fail
import org.junit.Before
import org.junit.Test

/**
 * セッションコピー修正とドラッグ＆ドロップ移動機能に関するユニットテスト
 *
 * テスト対象メソッド:
 *   - AppState.copySession(sourceSessionId, newName)
 *   - AppState.moveSession(sessionId, newCategoryId)
 *   - AppState.moveCategory(categoryId, newParentId)
 *   - AppState.isDescendant(targetId, potentialAncestorId)
 */
class SessionCopyAndMoveTest {

    private lateinit var appState: AppState

    @Before
    fun setUp() {
        appState = AppState()
    }

    // =========================================================================
    // copySession のテスト
    // =========================================================================

    @Test
    fun コピー元のcategoryIdを持つセッションをコピーすると同じcategoryIdになること() {
        val cat = appState.addCategory(parentId = null, name = "カテゴリA")
        val source = appState.addSession("元セッション", categoryId = cat.id)
        appState.addUserToSession(source.id, "プレイヤー1")

        val copied = appState.copySession(source.id, "コピーセッション")

        assertNotNull("コピー結果はnullでないこと", copied)
        assertEquals("コピー先のcategoryIdがコピー元と同じであること", cat.id, copied!!.categoryId)
    }

    @Test
    fun コピー元のcategoryIdがnullの場合コピー先もnullであること() {
        val source = appState.addSession("ルートセッション", categoryId = null)
        appState.addUserToSession(source.id, "プレイヤー1")

        val copied = appState.copySession(source.id, "コピーセッション")

        assertNotNull("コピー結果はnullでないこと", copied)
        assertNull("コピー先のcategoryIdがnull（ルート）であること", copied!!.categoryId)
    }

    @Test
    fun コピー時にユーザーもコピーされること() {
        val source = appState.addSession("元セッション", categoryId = null)
        appState.addUserToSession(source.id, "ユーザーA")
        appState.addUserToSession(source.id, "ユーザーB")

        val copied = appState.copySession(source.id, "コピーセッション")

        assertNotNull("コピー結果はnullでないこと", copied)
        val copiedUsers = appState.getSessionUsers(copied!!.id)
        assertEquals("コピー先にも2名のユーザーが存在すること", 2, copiedUsers.size)
        assertTrue("ユーザーAがコピーされること", copiedUsers.any { it.name == "ユーザーA" })
        assertTrue("ユーザーBがコピーされること", copiedUsers.any { it.name == "ユーザーB" })
    }

    @Test
    fun コピー時にユーザーのスコアは0で初期化されること() {
        val source = appState.addSession("元セッション", categoryId = null)
        val user = appState.addUserToSession(source.id, "プレイヤー1")
        appState.addScoreRecord(source.id, mapOf(user.id to 100))

        val copied = appState.copySession(source.id, "コピーセッション")

        assertNotNull("コピー結果はnullでないこと", copied)
        val copiedUsers = appState.getSessionUsers(copied!!.id)
        assertTrue("コピー先ユーザーのスコアはすべて0であること", copiedUsers.all { it.score == 0 })
    }

    @Test
    fun コピー時にスコア履歴はコピーされないこと() {
        val source = appState.addSession("元セッション", categoryId = null)
        val user = appState.addUserToSession(source.id, "プレイヤー1")
        appState.addScoreRecord(source.id, mapOf(user.id to 50))

        val copied = appState.copySession(source.id, "コピーセッション")

        assertNotNull("コピー結果はnullでないこと", copied)
        val copiedHistory = appState.getSessionScoreHistory(copied!!.id)
        assertTrue("コピー先のスコア履歴は空であること", copiedHistory.isEmpty())
    }

    @Test
    fun 存在しないセッションIDをコピーするとnullを返すこと() {
        val result = appState.copySession(9999, "コピーセッション")
        assertNull("存在しないセッションIDのコピーはnullを返すこと", result)
    }

    @Test
    fun コピー後のセッション名が指定した名前になること() {
        val source = appState.addSession("元セッション", categoryId = null)

        val copied = appState.copySession(source.id, "新しいセッション名")

        assertNotNull("コピー結果はnullでないこと", copied)
        assertEquals("コピー先のセッション名が指定した名前であること", "新しいセッション名", copied!!.name)
    }

    @Test
    fun コピー後のセッションIDが元のIDとは異なること() {
        val source = appState.addSession("元セッション", categoryId = null)

        val copied = appState.copySession(source.id, "コピーセッション")

        assertNotNull("コピー結果はnullでないこと", copied)
        assertTrue("コピー先のIDはコピー元のIDと異なること", source.id != copied!!.id)
    }

    // =========================================================================
    // moveSession のテスト
    // =========================================================================

    @Test
    fun セッションを別カテゴリに移動できること() {
        val catA = appState.addCategory(parentId = null, name = "カテゴリA")
        val catB = appState.addCategory(parentId = null, name = "カテゴリB")
        val session = appState.addSession("セッション1", categoryId = catA.id)

        appState.moveSession(session.id, catB.id)

        val movedSession = appState.sessions.find { it.id == session.id }
        assertNotNull("移動後のセッションが存在すること", movedSession)
        assertEquals("移動先カテゴリIDが正しいこと", catB.id, movedSession!!.categoryId)
    }

    @Test
    fun セッションをnullルート直下に移動できること() {
        val cat = appState.addCategory(parentId = null, name = "カテゴリA")
        val session = appState.addSession("セッション1", categoryId = cat.id)

        appState.moveSession(session.id, null)

        val movedSession = appState.sessions.find { it.id == session.id }
        assertNotNull("移動後のセッションが存在すること", movedSession)
        assertNull("ルートへの移動でcategoryIdがnullになること", movedSession!!.categoryId)
    }

    @Test
    fun 存在しないセッションIDを渡した場合に例外が発生しないこと() {
        try {
            appState.moveSession(9999, null)
        } catch (e: Exception) {
            fail("存在しないセッションIDでmoveSessioが例外を投げてはいけない: ${e.message}")
        }
    }

    @Test
    fun セッション移動後にgetSessionsByCategoryで正しく反映されること() {
        val catA = appState.addCategory(parentId = null, name = "カテゴリA")
        val catB = appState.addCategory(parentId = null, name = "カテゴリB")
        val session = appState.addSession("セッション1", categoryId = catA.id)

        appState.moveSession(session.id, catB.id)

        val sessionsInA = appState.getSessionsByCategory(catA.id)
        val sessionsInB = appState.getSessionsByCategory(catB.id)

        assertTrue("移動元カテゴリにセッションが存在しないこと", sessionsInA.isEmpty())
        assertEquals("移動先カテゴリにセッションが1件あること", 1, sessionsInB.size)
        assertEquals("移動先カテゴリのセッションIDが正しいこと", session.id, sessionsInB[0].id)
    }

    // =========================================================================
    // moveCategory のテスト
    // =========================================================================

    @Test
    fun カテゴリを別カテゴリに移動できること() {
        val catA = appState.addCategory(parentId = null, name = "カテゴリA")
        val catB = appState.addCategory(parentId = null, name = "カテゴリB")
        val child = appState.addCategory(parentId = catA.id, name = "子カテゴリ")

        val result = appState.moveCategory(child.id, catB.id)

        assertTrue("移動成功時はtrueを返すこと", result)
        val movedCategory = appState.categories.find { it.id == child.id }
        assertNotNull("移動後のカテゴリが存在すること", movedCategory)
        assertEquals("移動先の親IDが正しいこと", catB.id, movedCategory!!.parentId)
    }

    @Test
    fun カテゴリをnullルート直下に移動できること() {
        val parent = appState.addCategory(parentId = null, name = "親カテゴリ")
        val child = appState.addCategory(parentId = parent.id, name = "子カテゴリ")

        val result = appState.moveCategory(child.id, null)

        assertTrue("移動成功時はtrueを返すこと", result)
        val movedCategory = appState.categories.find { it.id == child.id }
        assertNotNull("移動後のカテゴリが存在すること", movedCategory)
        assertNull("ルートへの移動でparentIdがnullになること", movedCategory!!.parentId)
    }

    @Test
    fun 自分自身に移動しようとするとfalseを返すこと() {
        val cat = appState.addCategory(parentId = null, name = "カテゴリA")

        val result = appState.moveCategory(cat.id, cat.id)

        assertFalse("自己移動はfalseを返すこと", result)
    }

    @Test
    fun 自分の子孫カテゴリに移動しようとするとfalseを返すこと_直接の子() {
        val parent = appState.addCategory(parentId = null, name = "親カテゴリ")
        val child = appState.addCategory(parentId = parent.id, name = "子カテゴリ")

        val result = appState.moveCategory(parent.id, child.id)

        assertFalse("直接の子への移動はfalseを返すこと（循環参照防止）", result)
    }

    @Test
    fun 自分の孫カテゴリに移動しようとするとfalseを返すこと() {
        val grandParent = appState.addCategory(parentId = null, name = "祖父カテゴリ")
        val parent = appState.addCategory(parentId = grandParent.id, name = "親カテゴリ")
        val grandChild = appState.addCategory(parentId = parent.id, name = "孫カテゴリ")

        val result = appState.moveCategory(grandParent.id, grandChild.id)

        assertFalse("孫への移動はfalseを返すこと（循環参照防止）", result)
    }

    @Test
    fun カテゴリ移動後にgetChildCategoriesで正しく反映されること() {
        val catA = appState.addCategory(parentId = null, name = "カテゴリA")
        val catB = appState.addCategory(parentId = null, name = "カテゴリB")
        val child = appState.addCategory(parentId = catA.id, name = "子カテゴリ")

        appState.moveCategory(child.id, catB.id)

        val childrenOfA = appState.getChildCategories(catA.id)
        val childrenOfB = appState.getChildCategories(catB.id)

        assertTrue("移動元カテゴリに子が存在しないこと", childrenOfA.isEmpty())
        assertEquals("移動先カテゴリに子が1件あること", 1, childrenOfB.size)
        assertEquals("移動先の子のIDが正しいこと", child.id, childrenOfB[0].id)
    }

    // =========================================================================
    // isDescendant のテスト
    // =========================================================================

    @Test
    fun 直接の子が子孫として判定されること() {
        val parent = appState.addCategory(parentId = null, name = "親カテゴリ")
        val child = appState.addCategory(parentId = parent.id, name = "子カテゴリ")

        val result = appState.isDescendant(child.id, parent.id)

        assertTrue("直接の子は子孫として判定されること", result)
    }

    @Test
    fun 孫が子孫として判定されること() {
        val grandParent = appState.addCategory(parentId = null, name = "祖父カテゴリ")
        val parent = appState.addCategory(parentId = grandParent.id, name = "親カテゴリ")
        val grandChild = appState.addCategory(parentId = parent.id, name = "孫カテゴリ")

        val result = appState.isDescendant(grandChild.id, grandParent.id)

        assertTrue("孫は子孫として判定されること", result)
    }

    @Test
    fun 兄弟カテゴリは子孫でないこと() {
        val parent = appState.addCategory(parentId = null, name = "親カテゴリ")
        val sibling1 = appState.addCategory(parentId = parent.id, name = "兄弟1")
        val sibling2 = appState.addCategory(parentId = parent.id, name = "兄弟2")

        val result = appState.isDescendant(sibling2.id, sibling1.id)

        assertFalse("兄弟カテゴリは子孫でないこと", result)
    }

    @Test
    fun 親カテゴリは子孫でないこと() {
        val parent = appState.addCategory(parentId = null, name = "親カテゴリ")
        val child = appState.addCategory(parentId = parent.id, name = "子カテゴリ")

        val result = appState.isDescendant(parent.id, child.id)

        assertFalse("親カテゴリは子の子孫でないこと", result)
    }

    @Test
    fun 自分自身は子孫として判定されること() {
        val cat = appState.addCategory(parentId = null, name = "カテゴリA")

        val result = appState.isDescendant(cat.id, cat.id)

        assertTrue("自分自身は子孫として判定されること（同一ID判定）", result)
    }

    @Test
    fun 全く関係ないカテゴリは子孫でないこと() {
        val catA = appState.addCategory(parentId = null, name = "カテゴリA")
        val catB = appState.addCategory(parentId = null, name = "カテゴリB")

        val result = appState.isDescendant(catB.id, catA.id)

        assertFalse("全く関係ないカテゴリは子孫でないこと", result)
    }

    @Test
    fun 深いネストの子孫が正しく判定されること() {
        val level1 = appState.addCategory(parentId = null, name = "レベル1")
        val level2 = appState.addCategory(parentId = level1.id, name = "レベル2")
        val level3 = appState.addCategory(parentId = level2.id, name = "レベル3")
        val level4 = appState.addCategory(parentId = level3.id, name = "レベル4")

        assertTrue("4階層の子孫が正しく判定されること", appState.isDescendant(level4.id, level1.id))
        assertTrue("3階層の子孫が正しく判定されること", appState.isDescendant(level4.id, level2.id))
        assertTrue("2階層の子孫が正しく判定されること", appState.isDescendant(level4.id, level3.id))
        assertFalse("祖先は子孫でないこと", appState.isDescendant(level1.id, level4.id))
    }
}
