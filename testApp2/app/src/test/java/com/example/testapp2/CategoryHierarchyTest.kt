package com.example.testapp2

import com.example.testapp2.data.AppState
import com.example.testapp2.data.Category
import com.example.testapp2.data.CategoryType
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test

/**
 * カテゴリ階層機能に関するユニットテスト
 *
 * テスト対象:
 *  - AppState.addCategory()             : カテゴリ追加
 *  - AppState.getChildCategories()      : 子カテゴリ取得
 *  - AppState.deleteCategoryLocal()     : カテゴリ削除（子孫も含む再帰的削除）
 *  - AppState.collectCategoryAndDescendantIds() : 子孫IDの収集
 */
class CategoryHierarchyTest {

    private lateinit var appState: AppState
    /** テスト用セッションID */
    private val sessionId = 1

    @Before
    fun setUp() {
        appState = AppState()
    }

    // =========================================================================
    // addCategory のテスト
    // =========================================================================

    @Test
    fun ルートカテゴリを追加するとカテゴリ一覧に反映される() {
        val cat = appState.addCategory(sessionId, parentId = null, name = "ルート", type = CategoryType.FOLDER)

        assertEquals(1, appState.categories.size)
        assertEquals("ルート", cat.name)
        assertEquals(sessionId, cat.sessionId)
        assertNull(cat.parentId)
        assertEquals(CategoryType.FOLDER, cat.type)
    }

    @Test
    fun 子カテゴリを追加すると親IDが正しく設定される() {
        val parent = appState.addCategory(sessionId, parentId = null, name = "親", type = CategoryType.FOLDER)
        val child = appState.addCategory(sessionId, parentId = parent.id, name = "子", type = CategoryType.SECTION)

        assertEquals(parent.id, child.parentId)
        assertEquals(CategoryType.SECTION, child.type)
    }

    @Test
    fun 複数カテゴリを追加するとIDが重複しない() {
        val cat1 = appState.addCategory(sessionId, null, "カテゴリ1", CategoryType.FOLDER)
        val cat2 = appState.addCategory(sessionId, null, "カテゴリ2", CategoryType.FOLDER)
        val cat3 = appState.addCategory(sessionId, null, "カテゴリ3", CategoryType.SECTION)

        val ids = listOf(cat1.id, cat2.id, cat3.id)
        assertEquals("IDが重複しないこと", ids.distinct().size, ids.size)
        assertEquals(3, appState.categories.size)
    }

    @Test
    fun セクション型カテゴリを追加できる() {
        val section = appState.addCategory(sessionId, null, "第1セクション", CategoryType.SECTION)

        assertEquals(CategoryType.SECTION, section.type)
        assertTrue(appState.categories.contains(section))
    }

    @Test
    fun 異なるセッションにカテゴリを追加できる() {
        val cat1 = appState.addCategory(sessionId = 1, parentId = null, name = "セッション1のカテゴリ", type = CategoryType.FOLDER)
        val cat2 = appState.addCategory(sessionId = 2, parentId = null, name = "セッション2のカテゴリ", type = CategoryType.FOLDER)

        assertEquals(2, appState.categories.size)
        assertEquals(1, cat1.sessionId)
        assertEquals(2, cat2.sessionId)
    }

    // =========================================================================
    // getChildCategories のテスト
    // =========================================================================

    @Test
    fun ルート直下の子カテゴリ一覧を取得できる() {
        val child1 = appState.addCategory(sessionId, null, "子1", CategoryType.FOLDER)
        val child2 = appState.addCategory(sessionId, null, "子2", CategoryType.SECTION)

        val result = appState.getChildCategories(sessionId, parentId = null)

        assertEquals(2, result.size)
        assertTrue(result.any { it.id == child1.id })
        assertTrue(result.any { it.id == child2.id })
    }

    @Test
    fun 特定の親IDを持つ子カテゴリのみ取得できる() {
        val parent = appState.addCategory(sessionId, null, "親", CategoryType.FOLDER)
        val child1 = appState.addCategory(sessionId, parent.id, "子1", CategoryType.FOLDER)
        val child2 = appState.addCategory(sessionId, parent.id, "子2", CategoryType.SECTION)
        // 別の親（関係ない）
        val otherParent = appState.addCategory(sessionId, null, "別の親", CategoryType.FOLDER)
        appState.addCategory(sessionId, otherParent.id, "別の子", CategoryType.FOLDER)

        val result = appState.getChildCategories(sessionId, parentId = parent.id)

        assertEquals(2, result.size)
        assertTrue(result.any { it.id == child1.id })
        assertTrue(result.any { it.id == child2.id })
        assertFalse("別の親の子は含まれないこと", result.any { it.parentId == otherParent.id })
    }

    @Test
    fun 孫カテゴリはgetChildCategoriesに含まれない() {
        val parent = appState.addCategory(sessionId, null, "親", CategoryType.FOLDER)
        val child = appState.addCategory(sessionId, parent.id, "子", CategoryType.FOLDER)
        appState.addCategory(sessionId, child.id, "孫", CategoryType.SECTION)

        val result = appState.getChildCategories(sessionId, parentId = null)

        assertEquals("ルート直下は親のみ", 1, result.size)
        assertEquals(parent.id, result[0].id)
    }

    @Test
    fun 別セッションのカテゴリはgetChildCategoriesに含まれない() {
        appState.addCategory(sessionId = 1, parentId = null, name = "セッション1", type = CategoryType.FOLDER)
        appState.addCategory(sessionId = 2, parentId = null, name = "セッション2", type = CategoryType.FOLDER)

        val result = appState.getChildCategories(sessionId = 1, parentId = null)

        assertEquals(1, result.size)
        assertEquals(1, result[0].sessionId)
    }

    @Test
    fun 子カテゴリが存在しない場合は空リストを返す() {
        appState.addCategory(sessionId, null, "親のみ", CategoryType.FOLDER)

        val result = appState.getChildCategories(sessionId, parentId = 999)

        assertTrue("存在しない親IDの子は空リスト", result.isEmpty())
    }

    @Test
    fun getChildCategoriesはsortOrder順で返す() {
        // sortOrder を指定せずに追加し、IDの昇順でソートされることを確認
        val cat1 = appState.addCategory(sessionId, null, "アルファ", CategoryType.FOLDER)
        val cat2 = appState.addCategory(sessionId, null, "ベータ", CategoryType.FOLDER)
        val cat3 = appState.addCategory(sessionId, null, "ガンマ", CategoryType.FOLDER)

        val result = appState.getChildCategories(sessionId, null)

        // sortOrder=0 の場合、id の昇順になる（compareBy({ sortOrder }, { id })）
        assertEquals(cat1.id, result[0].id)
        assertEquals(cat2.id, result[1].id)
        assertEquals(cat3.id, result[2].id)
    }

    // =========================================================================
    // deleteCategoryLocal のテスト
    // =========================================================================

    @Test
    fun 単体カテゴリを削除できる() {
        val cat = appState.addCategory(sessionId, null, "削除対象", CategoryType.FOLDER)
        assertEquals(1, appState.categories.size)

        appState.deleteCategoryLocal(cat.id)

        assertEquals(0, appState.categories.size)
    }

    @Test
    fun 親カテゴリを削除すると子カテゴリも削除される() {
        val parent = appState.addCategory(sessionId, null, "親", CategoryType.FOLDER)
        appState.addCategory(sessionId, parent.id, "子1", CategoryType.FOLDER)
        appState.addCategory(sessionId, parent.id, "子2", CategoryType.SECTION)
        assertEquals(3, appState.categories.size)

        appState.deleteCategoryLocal(parent.id)

        assertEquals(0, appState.categories.size)
    }

    @Test
    fun 親カテゴリを削除すると孫カテゴリも再帰的に削除される() {
        val parent = appState.addCategory(sessionId, null, "親", CategoryType.FOLDER)
        val child = appState.addCategory(sessionId, parent.id, "子", CategoryType.FOLDER)
        appState.addCategory(sessionId, child.id, "孫", CategoryType.SECTION)
        assertEquals(3, appState.categories.size)

        appState.deleteCategoryLocal(parent.id)

        assertEquals(0, appState.categories.size)
    }

    @Test
    fun 削除対象以外のカテゴリは残る() {
        val toDelete = appState.addCategory(sessionId, null, "削除", CategoryType.FOLDER)
        val toKeep1 = appState.addCategory(sessionId, null, "保持1", CategoryType.FOLDER)
        val toKeep2 = appState.addCategory(sessionId, null, "保持2", CategoryType.SECTION)

        appState.deleteCategoryLocal(toDelete.id)

        assertEquals(2, appState.categories.size)
        assertTrue(appState.categories.any { it.id == toKeep1.id })
        assertTrue(appState.categories.any { it.id == toKeep2.id })
        assertFalse(appState.categories.any { it.id == toDelete.id })
    }

    @Test
    fun 存在しないIDを削除しても例外が発生しない() {
        appState.addCategory(sessionId, null, "既存", CategoryType.FOLDER)

        // 存在しないIDを指定しても例外が投げられないことを確認（JUnit 4 互換の書き方）
        try {
            appState.deleteCategoryLocal(9999)
        } catch (e: Exception) {
            fail("例外が発生してはならない: ${e.message}")
        }

        // 既存カテゴリは変わらず残っている
        assertEquals(1, appState.categories.size)
    }

    @Test
    fun 子カテゴリのみを削除しても親は残る() {
        val parent = appState.addCategory(sessionId, null, "親", CategoryType.FOLDER)
        val child = appState.addCategory(sessionId, parent.id, "子", CategoryType.SECTION)

        appState.deleteCategoryLocal(child.id)

        assertEquals(1, appState.categories.size)
        assertTrue(appState.categories.any { it.id == parent.id })
    }

    @Test
    fun 複数ブランチを持つ親カテゴリを削除すると全子孫が削除される() {
        // 構造:
        //   root
        //   ├── branch1
        //   │   ├── leaf1a
        //   │   └── leaf1b
        //   └── branch2
        //       └── leaf2a
        val root = appState.addCategory(sessionId, null, "root", CategoryType.FOLDER)
        val branch1 = appState.addCategory(sessionId, root.id, "branch1", CategoryType.FOLDER)
        appState.addCategory(sessionId, branch1.id, "leaf1a", CategoryType.SECTION)
        appState.addCategory(sessionId, branch1.id, "leaf1b", CategoryType.SECTION)
        val branch2 = appState.addCategory(sessionId, root.id, "branch2", CategoryType.FOLDER)
        appState.addCategory(sessionId, branch2.id, "leaf2a", CategoryType.SECTION)
        assertEquals(6, appState.categories.size)

        appState.deleteCategoryLocal(root.id)

        assertEquals(0, appState.categories.size)
    }

    // =========================================================================
    // collectCategoryAndDescendantIds のテスト
    // =========================================================================

    @Test
    fun 子のないカテゴリは自身のIDのみ返す() {
        val cat = appState.addCategory(sessionId, null, "単独", CategoryType.FOLDER)

        val ids = appState.collectCategoryAndDescendantIds(cat.id)

        assertEquals(1, ids.size)
        assertEquals(cat.id, ids[0])
    }

    @Test
    fun 子カテゴリのIDも収集される() {
        val parent = appState.addCategory(sessionId, null, "親", CategoryType.FOLDER)
        val child1 = appState.addCategory(sessionId, parent.id, "子1", CategoryType.SECTION)
        val child2 = appState.addCategory(sessionId, parent.id, "子2", CategoryType.SECTION)

        val ids = appState.collectCategoryAndDescendantIds(parent.id)

        assertEquals(3, ids.size)
        assertTrue(ids.contains(parent.id))
        assertTrue(ids.contains(child1.id))
        assertTrue(ids.contains(child2.id))
    }

    @Test
    fun 孫カテゴリのIDも再帰的に収集される() {
        val parent = appState.addCategory(sessionId, null, "親", CategoryType.FOLDER)
        val child = appState.addCategory(sessionId, parent.id, "子", CategoryType.FOLDER)
        val grandChild = appState.addCategory(sessionId, child.id, "孫", CategoryType.SECTION)

        val ids = appState.collectCategoryAndDescendantIds(parent.id)

        assertEquals(3, ids.size)
        assertTrue(ids.contains(parent.id))
        assertTrue(ids.contains(child.id))
        assertTrue(ids.contains(grandChild.id))
    }

    @Test
    fun 子ノードから収集すると親は含まれない() {
        val parent = appState.addCategory(sessionId, null, "親", CategoryType.FOLDER)
        val child = appState.addCategory(sessionId, parent.id, "子", CategoryType.FOLDER)
        val grandChild = appState.addCategory(sessionId, child.id, "孫", CategoryType.SECTION)

        val ids = appState.collectCategoryAndDescendantIds(child.id)

        assertEquals(2, ids.size)
        assertFalse("親IDは含まれないこと", ids.contains(parent.id))
        assertTrue(ids.contains(child.id))
        assertTrue(ids.contains(grandChild.id))
    }

    @Test
    fun 複数ブランチを持つ場合全子孫IDを収集できる() {
        // 構造: root → branch1 → leaf1, leaf2 / root → branch2 → leaf3
        val root = appState.addCategory(sessionId, null, "root", CategoryType.FOLDER)
        val branch1 = appState.addCategory(sessionId, root.id, "branch1", CategoryType.FOLDER)
        val leaf1 = appState.addCategory(sessionId, branch1.id, "leaf1", CategoryType.SECTION)
        val leaf2 = appState.addCategory(sessionId, branch1.id, "leaf2", CategoryType.SECTION)
        val branch2 = appState.addCategory(sessionId, root.id, "branch2", CategoryType.FOLDER)
        val leaf3 = appState.addCategory(sessionId, branch2.id, "leaf3", CategoryType.SECTION)

        val ids = appState.collectCategoryAndDescendantIds(root.id)

        assertEquals(6, ids.size)
        assertTrue(ids.containsAll(listOf(root.id, branch1.id, leaf1.id, leaf2.id, branch2.id, leaf3.id)))
    }

    @Test
    fun 対象カテゴリ自身のIDは必ず含まれる() {
        val cat = appState.addCategory(sessionId, null, "対象", CategoryType.SECTION)

        val ids = appState.collectCategoryAndDescendantIds(cat.id)

        assertTrue("対象カテゴリ自身のIDが含まれること", ids.contains(cat.id))
    }

    // =========================================================================
    // 統合シナリオ: 階層構造の正確な動作を確認
    // =========================================================================

    @Test
    fun 多段階層でのaddCategoryとgetChildCategoriesが整合する() {
        // 3段階の階層を構築
        val lv1 = appState.addCategory(sessionId, null, "Lv1", CategoryType.FOLDER)
        val lv2 = appState.addCategory(sessionId, lv1.id, "Lv2", CategoryType.FOLDER)
        val lv3 = appState.addCategory(sessionId, lv2.id, "Lv3", CategoryType.SECTION)

        // 各階層の子が1件ずつのみ返ることを確認
        assertEquals(1, appState.getChildCategories(sessionId, null).size)
        assertEquals(lv1.id, appState.getChildCategories(sessionId, null)[0].id)

        assertEquals(1, appState.getChildCategories(sessionId, lv1.id).size)
        assertEquals(lv2.id, appState.getChildCategories(sessionId, lv1.id)[0].id)

        assertEquals(1, appState.getChildCategories(sessionId, lv2.id).size)
        assertEquals(lv3.id, appState.getChildCategories(sessionId, lv2.id)[0].id)

        // lv3 は葉なので子なし
        assertTrue(appState.getChildCategories(sessionId, lv3.id).isEmpty())
    }

    @Test
    fun 削除前にcollectして削除後にカテゴリが残っていないことを確認() {
        val parent = appState.addCategory(sessionId, null, "親", CategoryType.FOLDER)
        val child = appState.addCategory(sessionId, parent.id, "子", CategoryType.FOLDER)
        appState.addCategory(sessionId, child.id, "孫1", CategoryType.SECTION)
        appState.addCategory(sessionId, child.id, "孫2", CategoryType.SECTION)

        // collect してから delete
        val idsToDelete = appState.collectCategoryAndDescendantIds(parent.id)
        assertEquals(4, idsToDelete.size)

        appState.deleteCategoryLocal(parent.id)

        // collect した全IDがカテゴリ一覧に存在しないこと
        idsToDelete.forEach { id ->
            assertFalse("ID=$id が削除されていること", appState.categories.any { it.id == id })
        }
        assertEquals(0, appState.categories.size)
    }

    @Test
    fun セッション削除時にそのセッションのカテゴリも削除される() {
        val session = appState.addSession("テストセッション")
        appState.addCategory(session.id, null, "カテゴリA", CategoryType.FOLDER)
        appState.addCategory(session.id, null, "カテゴリB", CategoryType.SECTION)

        appState.deleteSessionLocal(session.id)

        assertTrue("セッション削除でカテゴリも消えること", appState.categories.isEmpty())
    }

    @Test
    fun 異なるセッションのカテゴリはセッション削除の影響を受けない() {
        val session1 = appState.addSession("セッション1")
        val session2 = appState.addSession("セッション2")
        appState.addCategory(session1.id, null, "S1カテゴリ", CategoryType.FOLDER)
        appState.addCategory(session2.id, null, "S2カテゴリ", CategoryType.FOLDER)

        appState.deleteSessionLocal(session1.id)

        assertEquals("セッション2のカテゴリは残ること", 1, appState.categories.size)
        assertEquals(session2.id, appState.categories[0].sessionId)
    }

    // =========================================================================
    // ユーティリティ: assertDoesNotThrow (JUnit4 互換)
    // =========================================================================

    /**
     * JUnit4 には assertDoesNotThrow がないため、ラムダが例外を投げないことを検証するヘルパー
     */
    private fun assertDoesNotThrow(block: () -> Unit) {
        try {
            block()
        } catch (e: Throwable) {
            fail("例外が発生しないはずが ${e::class.simpleName} が投げられた: ${e.message}")
        }
    }
}
