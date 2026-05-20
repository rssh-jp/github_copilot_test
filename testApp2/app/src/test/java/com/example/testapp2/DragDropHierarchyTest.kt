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
 * ドラッグ＆ドロップによる階層移動機能に関するユニットテスト
 *
 * テスト対象メソッド:
 *   - AppState.moveSession(sessionId, newCategoryId)
 *   - AppState.moveCategory(categoryId, newParentId)
 *   - AppState.isDescendant(targetId, potentialAncestorId)
 *
 * 対応する修正:
 *   - CategoryBrowserScreen.kt の .pointerInput(categoryId) 変更
 *   - LaunchedEffect(categoryId) でのドラッグ状態リセット
 */
class DragDropHierarchyTest {

    private lateinit var appState: AppState

    @Before
    fun setUp() {
        appState = AppState()
    }

    // =========================================================================
    // moveSession のテスト
    // =========================================================================

    /**
     * セッションを子カテゴリに移動できること
     */
    @Test
    fun moveSession_toChildCategory_succeeds() {
        // 親カテゴリと子カテゴリを作成
        val parentCat = appState.addCategory(parentId = null, name = "親カテゴリ")
        val childCat = appState.addCategory(parentId = parentCat.id, name = "子カテゴリ")
        // セッションを親カテゴリに追加
        val session = appState.addSession("テストセッション", categoryId = parentCat.id)

        // 子カテゴリへ移動
        try {
            appState.moveSession(session.id, childCat.id)
        } catch (e: Exception) {
            fail("moveSession が例外を投げてはいけない: ${e.message}")
        }

        // 移動後の検証
        val movedSession = appState.sessions.find { it.id == session.id }
        assertNotNull("移動後のセッションが存在すること", movedSession)
        assertEquals("移動先の子カテゴリIDが設定されていること", childCat.id, movedSession!!.categoryId)

        // getSessionsByCategory による検証
        val sessionsInParent = appState.getSessionsByCategory(parentCat.id)
        val sessionsInChild = appState.getSessionsByCategory(childCat.id)
        assertTrue("移動元（親カテゴリ）にセッションが存在しないこと", sessionsInParent.isEmpty())
        assertEquals("移動先（子カテゴリ）にセッションが1件あること", 1, sessionsInChild.size)
        assertEquals("移動先セッションのIDが正しいこと", session.id, sessionsInChild[0].id)
    }

    /**
     * セッションをルートに移動するとcategoryId=nullになること
     */
    @Test
    fun moveSession_toRoot_categoryIdBecomesNull() {
        // カテゴリを作成してセッションを配置
        val cat = appState.addCategory(parentId = null, name = "カテゴリA")
        val session = appState.addSession("カテゴリ内セッション", categoryId = cat.id)

        // ルート（null）へ移動
        try {
            appState.moveSession(session.id, null)
        } catch (e: Exception) {
            fail("ルートへのmoveSession が例外を投げてはいけない: ${e.message}")
        }

        // categoryId が null になっていることを確認
        val movedSession = appState.sessions.find { it.id == session.id }
        assertNotNull("移動後のセッションが存在すること", movedSession)
        assertNull("ルートへの移動後に categoryId が null になっていること", movedSession!!.categoryId)

        // ルート（null）のセッション一覧に含まれること
        val rootSessions = appState.getSessionsByCategory(null)
        assertTrue("ルート配下にセッションが含まれること", rootSessions.any { it.id == session.id })
    }

    // =========================================================================
    // moveCategory のテスト
    // =========================================================================

    /**
     * カテゴリを別カテゴリに移動できること
     */
    @Test
    fun moveCategory_toAnotherCategory_succeeds() {
        // 2つのルートカテゴリと移動対象の子カテゴリを作成
        val catA = appState.addCategory(parentId = null, name = "カテゴリA")
        val catB = appState.addCategory(parentId = null, name = "カテゴリB")
        val movingCat = appState.addCategory(parentId = catA.id, name = "移動対象カテゴリ")

        // catB への移動
        val result = appState.moveCategory(movingCat.id, catB.id)

        assertTrue("移動成功時は true を返すこと", result)

        val movedCategory = appState.categories.find { it.id == movingCat.id }
        assertNotNull("移動後のカテゴリが存在すること", movedCategory)
        assertEquals("移動先の親カテゴリIDが正しいこと", catB.id, movedCategory!!.parentId)

        // getChildCategories による検証
        val childrenOfA = appState.getChildCategories(catA.id)
        val childrenOfB = appState.getChildCategories(catB.id)
        assertTrue("移動元カテゴリ（A）の子リストが空になること", childrenOfA.isEmpty())
        assertEquals("移動先カテゴリ（B）の子リストに1件あること", 1, childrenOfB.size)
        assertEquals("移動先の子のIDが正しいこと", movingCat.id, childrenOfB[0].id)
    }

    /**
     * 循環参照となる移動が拒否されること
     */
    @Test
    fun moveCategory_preventsCircularReference() {
        // 親 → 子 → 孫 の3階層を作成
        val grandParent = appState.addCategory(parentId = null, name = "祖父カテゴリ")
        val parent = appState.addCategory(parentId = grandParent.id, name = "親カテゴリ")
        val child = appState.addCategory(parentId = parent.id, name = "子カテゴリ")

        // ケース1: 親を自分の直接の子へ移動しようとする（循環参照）
        val result1 = appState.moveCategory(grandParent.id, child.id)
        assertFalse("祖父を孫カテゴリに移動しようとすると false を返すこと（循環参照防止）", result1)

        // 元の親IDが変わっていないこと
        val grandParentAfter = appState.categories.find { it.id == grandParent.id }
        assertNull("祖父の parentId が変わっていないこと（null のまま）", grandParentAfter!!.parentId)

        // ケース2: 自分自身への移動は禁止
        val result2 = appState.moveCategory(parent.id, parent.id)
        assertFalse("自分自身への移動は false を返すこと", result2)

        // ケース3: 親を直接の子へ移動しようとする
        val result3 = appState.moveCategory(parent.id, child.id)
        assertFalse("親を直接の子に移動しようとすると false を返すこと（循環参照防止）", result3)
    }

    /**
     * 深い階層でカテゴリを移動できること
     */
    @Test
    fun moveCategory_inDeepHierarchy_succeeds() {
        // 5階層の深い階層を作成
        val level1 = appState.addCategory(parentId = null, name = "レベル1")
        val level2 = appState.addCategory(parentId = level1.id, name = "レベル2")
        val level3 = appState.addCategory(parentId = level2.id, name = "レベル3")
        val level4 = appState.addCategory(parentId = level3.id, name = "レベル4")
        val level5 = appState.addCategory(parentId = level4.id, name = "レベル5")

        // レベル5のカテゴリをレベル1の直下に移動
        val result = appState.moveCategory(level5.id, level1.id)

        assertTrue("深い階層からの移動が成功すること", result)

        val movedCat = appState.categories.find { it.id == level5.id }
        assertNotNull("移動後のカテゴリが存在すること", movedCat)
        assertEquals("移動先の親IDがレベル1であること", level1.id, movedCat!!.parentId)

        // level4の子リストが空になること
        val childrenOfLevel4 = appState.getChildCategories(level4.id)
        assertTrue("移動元（レベル4）の子リストが空になること", childrenOfLevel4.isEmpty())

        // level1の子リストにレベル2とレベル5が含まれること
        val childrenOfLevel1 = appState.getChildCategories(level1.id)
        assertEquals("レベル1の子リストに2件（レベル2とレベル5）あること", 2, childrenOfLevel1.size)
        assertTrue("レベル5がレベル1の子に含まれること", childrenOfLevel1.any { it.id == level5.id })

        // 循環参照チェック: レベル1をレベル5に移動しようとする（禁止のはず）
        val circularResult = appState.moveCategory(level1.id, level5.id)
        assertFalse("深い階層での循環参照移動が拒否されること", circularResult)
    }

    // =========================================================================
    // categoryId 切り替え後の AppState 整合性検証
    // =========================================================================

    /**
     * categoryId が変わった際に移動済みセッションが正しく反映されること
     *
     * 注意: CategoryBrowserScreen の LaunchedEffect(categoryId) によるドラッグ状態リセット自体は
     * UI コンポーザブル内の動作であり、ユニットテストでは検証不可。
     * ここでは moveSession 後に getSessionsByCategory が正しい結果を返すことを検証する。
     */
    @Test
    fun moveSession_afterCategorySwitch_sessionIsInNewCategory() {
        // カテゴリAとカテゴリBを作成
        val catA = appState.addCategory(parentId = null, name = "カテゴリA")
        val catB = appState.addCategory(parentId = null, name = "カテゴリB")
        val session = appState.addSession("移動テストセッション", categoryId = catA.id)

        // カテゴリAを閲覧中（simulated）に catB へ移動
        appState.moveSession(session.id, catB.id)

        // catB に切り替えて表示（LaunchedEffect でリセット後）
        val sessionsInCatB = appState.getSessionsByCategory(catB.id)
        assertEquals("カテゴリB切り替え後にセッションが1件あること", 1, sessionsInCatB.size)
        assertEquals("移動したセッションのIDが正しいこと", session.id, sessionsInCatB[0].id)

        // catA には存在しないこと
        val sessionsInCatA = appState.getSessionsByCategory(catA.id)
        assertTrue("カテゴリA切り替え後にセッションが存在しないこと", sessionsInCatA.isEmpty())
    }
}
