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
 * カテゴリがセッションを内包する階層機能に関するユニットテスト
 *
 * テスト対象メソッド:
 *   - AppState.addCategory(parentId, name)
 *   - AppState.getChildCategories(parentId)
 *   - AppState.getSessionsByCategory(categoryId)
 *   - AppState.deleteCategoryLocal(categoryId)
 */
class CategorySessionHierarchyTest {

    private lateinit var appState: AppState

    @Before
    fun setUp() {
        appState = AppState()
    }

    // =========================================================================
    // addCategory のテスト
    // =========================================================================

    @Test
    fun ルートカテゴリを追加するとカテゴリ一覧に反映される() {
        val category = appState.addCategory(parentId = null, name = "ルートカテゴリ")

        assertEquals("カテゴリ一覧のサイズは1であること", 1, appState.categories.size)
        assertEquals("カテゴリ名が正しいこと", "ルートカテゴリ", category.name)
        assertNull("parentId は null であること", category.parentId)
        assertTrue("IDが1以上であること", category.id >= 1)
    }

    @Test
    fun 子カテゴリを追加するとparentIdが設定される() {
        val parent = appState.addCategory(parentId = null, name = "親カテゴリ")
        val child = appState.addCategory(parentId = parent.id, name = "子カテゴリ")

        assertEquals("カテゴリ一覧のサイズは2であること", 2, appState.categories.size)
        assertEquals("子カテゴリのparentIdが親のIDと一致すること", parent.id, child.parentId)
    }

    @Test
    fun 複数カテゴリ追加時にIDが一意採番される() {
        val cat1 = appState.addCategory(parentId = null, name = "カテゴリ1")
        val cat2 = appState.addCategory(parentId = null, name = "カテゴリ2")
        val cat3 = appState.addCategory(parentId = null, name = "カテゴリ3")

        val ids = listOf(cat1.id, cat2.id, cat3.id)
        assertEquals("IDがすべて異なること", ids.distinct().size, ids.size)
    }

    @Test
    fun カテゴリ追加時に例外が発生しないこと() {
        try {
            appState.addCategory(parentId = null, name = "テストカテゴリ")
        } catch (e: Exception) {
            fail("addCategory で例外が発生してはいけない: ${e.message}")
        }
    }

    // =========================================================================
    // getChildCategories のテスト
    // =========================================================================

    @Test
    fun ルート直下の子カテゴリをnullで取得できる() {
        appState.addCategory(parentId = null, name = "ルートA")
        appState.addCategory(parentId = null, name = "ルートB")

        val children = appState.getChildCategories(parentId = null)

        assertEquals("ルート直下のカテゴリが2件であること", 2, children.size)
        assertTrue("カテゴリAが含まれること", children.any { it.name == "ルートA" })
        assertTrue("カテゴリBが含まれること", children.any { it.name == "ルートB" })
    }

    @Test
    fun 指定カテゴリの直接の子カテゴリのみ返す() {
        val root = appState.addCategory(parentId = null, name = "ルート")
        val child1 = appState.addCategory(parentId = root.id, name = "子1")
        appState.addCategory(parentId = root.id, name = "子2")
        // 孫カテゴリ（rootの子ではなく child1 の子）
        appState.addCategory(parentId = child1.id, name = "孫1")

        val children = appState.getChildCategories(parentId = root.id)

        assertEquals("直接の子カテゴリは2件であること", 2, children.size)
        assertTrue("子1が含まれること", children.any { it.name == "子1" })
        assertTrue("子2が含まれること", children.any { it.name == "子2" })
        assertFalse("孫カテゴリは含まれないこと", children.any { it.name == "孫1" })
    }

    @Test
    fun 子カテゴリが存在しない場合は空リストを返す() {
        val cat = appState.addCategory(parentId = null, name = "葉カテゴリ")

        val children = appState.getChildCategories(parentId = cat.id)

        assertTrue("子カテゴリが存在しない場合は空リストであること", children.isEmpty())
    }

    @Test
    fun カテゴリが一件もない時にルート直下を取得すると空リストになる() {
        val children = appState.getChildCategories(parentId = null)
        assertTrue("カテゴリなし時は空リストであること", children.isEmpty())
    }

    @Test
    fun 異なる親カテゴリの子カテゴリが混在しても正しくフィルタできる() {
        val parentA = appState.addCategory(parentId = null, name = "親A")
        val parentB = appState.addCategory(parentId = null, name = "親B")
        appState.addCategory(parentId = parentA.id, name = "Aの子1")
        appState.addCategory(parentId = parentA.id, name = "Aの子2")
        appState.addCategory(parentId = parentB.id, name = "Bの子1")

        val childrenOfA = appState.getChildCategories(parentId = parentA.id)
        val childrenOfB = appState.getChildCategories(parentId = parentB.id)

        assertEquals("親Aの子カテゴリは2件", 2, childrenOfA.size)
        assertEquals("親Bの子カテゴリは1件", 1, childrenOfB.size)
        assertTrue("Aの子は親AのIDを持つこと", childrenOfA.all { it.parentId == parentA.id })
        assertTrue("Bの子は親BのIDを持つこと", childrenOfB.all { it.parentId == parentB.id })
    }

    // =========================================================================
    // getSessionsByCategory のテスト
    // =========================================================================

    @Test
    fun 指定カテゴリのセッションを取得できる() {
        val cat = appState.addCategory(parentId = null, name = "カテゴリ1")
        appState.addSession("セッションA", categoryId = cat.id)
        appState.addSession("セッションB", categoryId = cat.id)

        val sessions = appState.getSessionsByCategory(categoryId = cat.id)

        assertEquals("カテゴリ内のセッションは2件であること", 2, sessions.size)
        assertTrue("セッションAが含まれること", sessions.any { it.name == "セッションA" })
        assertTrue("セッションBが含まれること", sessions.any { it.name == "セッションB" })
    }

    @Test
    fun ルート直下のセッションをnullで取得できる() {
        appState.addSession("ルートセッション1", categoryId = null)
        appState.addSession("ルートセッション2", categoryId = null)
        val cat = appState.addCategory(parentId = null, name = "カテゴリA")
        appState.addSession("カテゴリ内セッション", categoryId = cat.id)

        val rootSessions = appState.getSessionsByCategory(categoryId = null)

        assertEquals("ルート直下のセッションは2件であること", 2, rootSessions.size)
        assertFalse("カテゴリ内セッションはルート直下に含まれないこと",
            rootSessions.any { it.name == "カテゴリ内セッション" })
    }

    @Test
    fun 別カテゴリのセッションが混在しても正しくフィルタできる() {
        val catA = appState.addCategory(parentId = null, name = "カテゴリA")
        val catB = appState.addCategory(parentId = null, name = "カテゴリB")
        appState.addSession("Aのセッション", categoryId = catA.id)
        appState.addSession("Bのセッション1", categoryId = catB.id)
        appState.addSession("Bのセッション2", categoryId = catB.id)

        val sessionsA = appState.getSessionsByCategory(categoryId = catA.id)
        val sessionsB = appState.getSessionsByCategory(categoryId = catB.id)

        assertEquals("カテゴリAのセッションは1件", 1, sessionsA.size)
        assertEquals("カテゴリBのセッションは2件", 2, sessionsB.size)
        assertTrue("AのセッションはカテゴリAのIDを持つこと", sessionsA.all { it.categoryId == catA.id })
        assertTrue("BのセッションはカテゴリBのIDを持つこと", sessionsB.all { it.categoryId == catB.id })
    }

    @Test
    fun カテゴリ内にセッションがない場合は空リストを返す() {
        val cat = appState.addCategory(parentId = null, name = "空カテゴリ")

        val sessions = appState.getSessionsByCategory(categoryId = cat.id)

        assertTrue("セッションなし時は空リストであること", sessions.isEmpty())
    }

    // =========================================================================
    // deleteCategoryLocal のテスト
    // =========================================================================

    @Test
    fun カテゴリを削除するとカテゴリ一覧から消える() {
        val cat = appState.addCategory(parentId = null, name = "削除対象カテゴリ")

        appState.deleteCategoryLocal(cat.id)

        assertFalse("削除後はカテゴリ一覧に含まれないこと",
            appState.categories.any { it.id == cat.id })
    }

    @Test
    fun カテゴリ削除時に所属セッションも削除される() {
        val cat = appState.addCategory(parentId = null, name = "カテゴリ")
        val session = appState.addSession("セッション1", categoryId = cat.id)

        appState.deleteCategoryLocal(cat.id)

        assertFalse("削除後はセッションが存在しないこと",
            appState.sessions.any { it.id == session.id })
    }

    @Test
    fun カテゴリ削除時に子カテゴリも再帰的に削除される() {
        val parent = appState.addCategory(parentId = null, name = "親")
        val child = appState.addCategory(parentId = parent.id, name = "子")

        appState.deleteCategoryLocal(parent.id)

        assertFalse("親カテゴリが削除されること", appState.categories.any { it.id == parent.id })
        assertFalse("子カテゴリも削除されること", appState.categories.any { it.id == child.id })
    }

    @Test
    fun カテゴリ削除時に孫カテゴリも再帰的に削除される() {
        val root = appState.addCategory(parentId = null, name = "ルート")
        val child = appState.addCategory(parentId = root.id, name = "子")
        val grandChild = appState.addCategory(parentId = child.id, name = "孫")

        appState.deleteCategoryLocal(root.id)

        assertFalse("ルートカテゴリが削除されること", appState.categories.any { it.id == root.id })
        assertFalse("子カテゴリが削除されること", appState.categories.any { it.id == child.id })
        assertFalse("孫カテゴリが削除されること", appState.categories.any { it.id == grandChild.id })
    }

    @Test
    fun カテゴリ削除時に子カテゴリのセッションも削除される() {
        val parent = appState.addCategory(parentId = null, name = "親")
        val child = appState.addCategory(parentId = parent.id, name = "子")
        val sessionInChild = appState.addSession("子のセッション", categoryId = child.id)

        appState.deleteCategoryLocal(parent.id)

        assertFalse("子カテゴリのセッションも削除されること",
            appState.sessions.any { it.id == sessionInChild.id })
    }

    @Test
    fun カテゴリ削除時に所属セッションのユーザーも削除される() {
        val cat = appState.addCategory(parentId = null, name = "カテゴリ")
        val session = appState.addSession("セッション", categoryId = cat.id)
        appState.addUserToSession(session.id, "プレイヤー1")
        appState.addUserToSession(session.id, "プレイヤー2")

        appState.deleteCategoryLocal(cat.id)

        val remainingUsers = appState.users.filter { it.sessionId == session.id }
        assertTrue("削除されたセッションのユーザーも削除されること", remainingUsers.isEmpty())
    }

    @Test
    fun 他のカテゴリのセッションは削除されないこと() {
        val catA = appState.addCategory(parentId = null, name = "カテゴリA")
        val catB = appState.addCategory(parentId = null, name = "カテゴリB")
        appState.addSession("Aのセッション", categoryId = catA.id)
        val sessionB = appState.addSession("Bのセッション", categoryId = catB.id)

        appState.deleteCategoryLocal(catA.id)

        assertTrue("カテゴリBのセッションは削除されないこと",
            appState.sessions.any { it.id == sessionB.id })
    }

    // =========================================================================
    // 階層構造の整合性テスト
    // =========================================================================

    @Test
    fun 三層階層でgetChildCategoriesが各段を正しく返す() {
        // ルート -> レベル1 -> レベル2
        val level0 = appState.addCategory(parentId = null, name = "レベル0(ルート)")
        val level1 = appState.addCategory(parentId = level0.id, name = "レベル1")
        appState.addCategory(parentId = level1.id, name = "レベル2")

        val rootChildren = appState.getChildCategories(parentId = null)
        val level1Children = appState.getChildCategories(parentId = level0.id)
        val level2Children = appState.getChildCategories(parentId = level1.id)

        assertEquals("ルート直下は1件", 1, rootChildren.size)
        assertEquals("レベル0の子は1件", 1, level1Children.size)
        assertEquals("レベル1の子は1件", 1, level2Children.size)
    }

    @Test
    fun カテゴリにセッションを追加後にgetSessionsByCategoryで取得できる() {
        val root = appState.addCategory(parentId = null, name = "ルート")
        val child = appState.addCategory(parentId = root.id, name = "子")
        appState.addSession("ルートのセッション", categoryId = root.id)
        appState.addSession("子のセッション", categoryId = child.id)

        val rootSessions = appState.getSessionsByCategory(root.id)
        val childSessions = appState.getSessionsByCategory(child.id)

        assertEquals("ルートのセッションは1件", 1, rootSessions.size)
        assertEquals("子のセッションは1件", 1, childSessions.size)
        assertEquals("ルートのセッション名が正しいこと", "ルートのセッション", rootSessions[0].name)
        assertEquals("子のセッション名が正しいこと", "子のセッション", childSessions[0].name)
    }

    @Test
    fun addCategoryが返すCategoryオブジェクトとcategoriesリストの内容が一致する() {
        val returned = appState.addCategory(parentId = null, name = "確認カテゴリ")
        val found = appState.categories.find { it.id == returned.id }

        assertNotNull("categoriesリストに同IDのカテゴリが存在すること", found)
        assertEquals("名前が一致すること", returned.name, found!!.name)
        assertEquals("parentIdが一致すること", returned.parentId, found.parentId)
    }

    @Test
    fun カテゴリ削除後にgetChildCategoriesで削除済みIDを指定すると空リストになる() {
        val parent = appState.addCategory(parentId = null, name = "削除される親")
        appState.addCategory(parentId = parent.id, name = "削除される子")

        appState.deleteCategoryLocal(parent.id)

        val children = appState.getChildCategories(parentId = parent.id)
        assertTrue("削除後は子カテゴリが0件になること", children.isEmpty())
    }

    @Test
    fun 複数の子カテゴリを持つ親を削除すると全子カテゴリが削除される() {
        val parent = appState.addCategory(parentId = null, name = "親")
        val child1 = appState.addCategory(parentId = parent.id, name = "子1")
        val child2 = appState.addCategory(parentId = parent.id, name = "子2")
        val child3 = appState.addCategory(parentId = parent.id, name = "子3")

        appState.deleteCategoryLocal(parent.id)

        assertEquals("全カテゴリが削除されること", 0, appState.categories.size)
        assertFalse("子1が削除されること", appState.categories.any { it.id == child1.id })
        assertFalse("子2が削除されること", appState.categories.any { it.id == child2.id })
        assertFalse("子3が削除されること", appState.categories.any { it.id == child3.id })
    }
}
