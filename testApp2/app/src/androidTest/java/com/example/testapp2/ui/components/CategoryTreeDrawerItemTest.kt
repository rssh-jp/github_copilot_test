package com.example.testapp2.ui.components

import androidx.compose.runtime.mutableStateOf
import androidx.compose.ui.semantics.SemanticsProperties
import androidx.compose.ui.test.*
import androidx.compose.ui.test.junit4.createComposeRule
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.example.testapp2.data.AppState
import com.example.testapp2.data.Category
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

/**
 * CategoryTreeDrawerContent / CategoryTreeItem のインストゥルメンテーションテスト
 *
 * テスト観点:
 *  1. カテゴリ0件のとき「カテゴリなし」が表示される
 *  2. カテゴリが存在するときカテゴリ名が表示される（「カテゴリなし」は非表示）
 *  3. 子カテゴリを持つノードに展開アイコン（▶）が表示される
 *  4. 子カテゴリを持たないノードに展開アイコンが表示されない
 *  5. 展開アイコンをタップすると子カテゴリが AnimatedVisibility で展開される
 *  6. 展開中の▼アイコンをタップすると子カテゴリが折りたたまれる
 *  7. カテゴリ名をタップすると onCategoryClick コールバックが呼ばれる
 *  8. 選択中カテゴリが selected=true（ハイライト）で表示される
 *  9. 非選択カテゴリが selected=false で表示される
 * 10. 全カテゴリIDを展開済みとして渡すと子カテゴリが初期状態から表示される
 * 11. 複数の親カテゴリが全展開済みのとき全ての子カテゴリが初期状態から表示される
 * 12. 多階層カテゴリで全展開済みのとき全レベルのカテゴリが初期状態から表示される
 */
@RunWith(AndroidJUnit4::class)
class CategoryTreeDrawerItemTest {

    @get:Rule
    val composeTestRule = createComposeRule()

    /** 指定カテゴリリストを持つ AppState を生成するヘルパー */
    private fun buildAppState(categories: List<Category>): AppState =
        AppState().also { it.categories.addAll(categories) }

    // ─────────────────────────────────────────────────────────────────────────
    // テスト観点 1: カテゴリ0件 → 「カテゴリなし」表示
    // ─────────────────────────────────────────────────────────────────────────

    @Test
    fun カテゴリが0件のときカテゴリなしテキストが表示される() {
        composeTestRule.setContent {
            CategoryTreeDrawerContent(
                appState = buildAppState(emptyList()),
                selectedCategoryId = null,
                expandedCategoryIds = emptySet(),
                onCategoryClick = {},
                onToggleExpand = {},
            )
        }

        composeTestRule.onNodeWithText("カテゴリなし").assertIsDisplayed()
    }

    // ─────────────────────────────────────────────────────────────────────────
    // テスト観点 2: カテゴリが存在するときカテゴリ名が表示される
    // ─────────────────────────────────────────────────────────────────────────

    @Test
    fun カテゴリが存在するときカテゴリ名が表示されカテゴリなしは非表示になる() {
        val categories = listOf(
            Category(id = 1, parentId = null, name = "スポーツ"),
        )
        composeTestRule.setContent {
            CategoryTreeDrawerContent(
                appState = buildAppState(categories),
                selectedCategoryId = null,
                expandedCategoryIds = emptySet(),
                onCategoryClick = {},
                onToggleExpand = {},
            )
        }

        composeTestRule.onNodeWithText("スポーツ").assertIsDisplayed()
        composeTestRule.onNodeWithText("カテゴリなし").assertDoesNotExist()
    }

    // ─────────────────────────────────────────────────────────────────────────
    // テスト観点 3: 子カテゴリを持つノードに展開アイコン（▶）が表示される
    // ─────────────────────────────────────────────────────────────────────────

    @Test
    fun 子カテゴリを持つノードに展開アイコンが表示される() {
        val categories = listOf(
            Category(id = 1, parentId = null, name = "親カテゴリ"),
            Category(id = 2, parentId = 1, name = "子カテゴリ"),
        )
        composeTestRule.setContent {
            CategoryTreeDrawerContent(
                appState = buildAppState(categories),
                selectedCategoryId = null,
                expandedCategoryIds = emptySet(),
                onCategoryClick = {},
                onToggleExpand = {},
            )
        }

        // 折りたたみ状態のアイコン「▶」が表示されること
        composeTestRule.onNodeWithText("▶").assertIsDisplayed()
    }

    // ─────────────────────────────────────────────────────────────────────────
    // テスト観点 4: 子カテゴリを持たないノードに展開アイコンが表示されない
    // ─────────────────────────────────────────────────────────────────────────

    @Test
    fun 子カテゴリを持たないノードに展開折りたたみアイコンが表示されない() {
        val categories = listOf(
            Category(id = 1, parentId = null, name = "葉ノードカテゴリ"),
        )
        composeTestRule.setContent {
            CategoryTreeDrawerContent(
                appState = buildAppState(categories),
                selectedCategoryId = null,
                expandedCategoryIds = emptySet(),
                onCategoryClick = {},
                onToggleExpand = {},
            )
        }

        composeTestRule.onNodeWithText("▶").assertDoesNotExist()
        composeTestRule.onNodeWithText("▼").assertDoesNotExist()
    }

    // ─────────────────────────────────────────────────────────────────────────
    // テスト観点 5: 展開アイコンをタップすると子カテゴリが AnimatedVisibility で展開される
    // ─────────────────────────────────────────────────────────────────────────

    @Test
    fun 展開アイコンをタップすると子カテゴリが表示される() {
        val categories = listOf(
            Category(id = 1, parentId = null, name = "親カテゴリ"),
            Category(id = 2, parentId = 1, name = "子カテゴリ"),
        )
        // expandedIds を mutableState で管理し、onToggleExpand で更新する
        val expandedIds = mutableStateOf(emptySet<Int>())

        composeTestRule.setContent {
            CategoryTreeDrawerContent(
                appState = buildAppState(categories),
                selectedCategoryId = null,
                expandedCategoryIds = expandedIds.value,
                onCategoryClick = {},
                onToggleExpand = { id -> expandedIds.value = expandedIds.value + id },
            )
        }

        // 初期状態では子カテゴリは非表示
        composeTestRule.onNodeWithText("子カテゴリ").assertDoesNotExist()

        // 展開アイコン「▶」をタップ
        composeTestRule.onNodeWithText("▶").performClick()
        composeTestRule.waitForIdle()

        // アニメーション完了後に子カテゴリが表示される
        composeTestRule.onNodeWithText("子カテゴリ").assertIsDisplayed()
        // アイコンが▼に切り替わる
        composeTestRule.onNodeWithText("▼").assertIsDisplayed()
    }

    // ─────────────────────────────────────────────────────────────────────────
    // テスト観点 6: 展開中の▼アイコンをタップすると子カテゴリが折りたたまれる
    // ─────────────────────────────────────────────────────────────────────────

    @Test
    fun 展開中の折りたたみアイコンをタップすると子カテゴリが折りたたまれる() {
        val categories = listOf(
            Category(id = 1, parentId = null, name = "親カテゴリ"),
            Category(id = 2, parentId = 1, name = "子カテゴリ"),
        )
        // 最初から展開済み状態
        val expandedIds = mutableStateOf(setOf(1))

        composeTestRule.setContent {
            CategoryTreeDrawerContent(
                appState = buildAppState(categories),
                selectedCategoryId = null,
                expandedCategoryIds = expandedIds.value,
                onCategoryClick = {},
                onToggleExpand = { id ->
                    expandedIds.value =
                        if (id in expandedIds.value) expandedIds.value - id
                        else expandedIds.value + id
                },
            )
        }

        // 初期状態では子カテゴリが表示されている
        composeTestRule.onNodeWithText("子カテゴリ").assertIsDisplayed()

        // 折りたたみアイコン「▼」をタップ
        composeTestRule.onNodeWithText("▼").performClick()
        composeTestRule.waitForIdle()

        // アニメーション完了後に子カテゴリが非表示になる
        composeTestRule.onNodeWithText("子カテゴリ").assertDoesNotExist()
        // アイコンが▶に戻る
        composeTestRule.onNodeWithText("▶").assertIsDisplayed()
    }

    // ─────────────────────────────────────────────────────────────────────────
    // テスト観点 7: カテゴリ名をタップすると onCategoryClick コールバックが呼ばれる
    // ─────────────────────────────────────────────────────────────────────────

    @Test
    fun カテゴリ名をタップするとonCategoryClickが正しいカテゴリで呼ばれる() {
        val targetCategory = Category(id = 10, parentId = null, name = "タップ対象カテゴリ")
        var clickedCategory: Category? = null

        composeTestRule.setContent {
            CategoryTreeDrawerContent(
                appState = buildAppState(listOf(targetCategory)),
                selectedCategoryId = null,
                expandedCategoryIds = emptySet(),
                onCategoryClick = { cat -> clickedCategory = cat },
                onToggleExpand = {},
            )
        }

        composeTestRule.onNodeWithText("タップ対象カテゴリ").performClick()

        // コールバックが呼ばれ、正しいカテゴリが渡されることを確認
        assertNotNull("onCategoryClick が呼ばれていない", clickedCategory)
        assertEquals("渡されたカテゴリIDが不正", 10, clickedCategory?.id)
        assertEquals("渡されたカテゴリ名が不正", "タップ対象カテゴリ", clickedCategory?.name)
    }

    @Test
    fun 子カテゴリ名をタップするとonCategoryClickが子カテゴリで呼ばれる() {
        val parentCategory = Category(id = 1, parentId = null, name = "親カテゴリ")
        val childCategory = Category(id = 2, parentId = 1, name = "子カテゴリ")
        var clickedCategory: Category? = null
        val expandedIds = mutableStateOf(setOf(1)) // 展開済み状態

        composeTestRule.setContent {
            CategoryTreeDrawerContent(
                appState = buildAppState(listOf(parentCategory, childCategory)),
                selectedCategoryId = null,
                expandedCategoryIds = expandedIds.value,
                onCategoryClick = { cat -> clickedCategory = cat },
                onToggleExpand = {},
            )
        }

        // 子カテゴリ名をタップ
        composeTestRule.onNodeWithText("子カテゴリ").performClick()

        assertNotNull("子カテゴリの onCategoryClick が呼ばれていない", clickedCategory)
        assertEquals("渡されたカテゴリIDが不正", 2, clickedCategory?.id)
    }

    // ─────────────────────────────────────────────────────────────────────────
    // テスト観点 8 & 9: 選択中カテゴリが selected=true、非選択カテゴリが selected=false
    // ─────────────────────────────────────────────────────────────────────────

    @Test
    fun 選択中カテゴリのノードがselected状態でハイライト表示される() {
        val categories = listOf(
            Category(id = 1, parentId = null, name = "選択カテゴリ"),
            Category(id = 2, parentId = null, name = "非選択カテゴリ"),
        )

        composeTestRule.setContent {
            CategoryTreeDrawerContent(
                appState = buildAppState(categories),
                selectedCategoryId = 1, // カテゴリ1が選択中
                expandedCategoryIds = emptySet(),
                onCategoryClick = {},
                onToggleExpand = {},
            )
        }

        // 選択中カテゴリは SemanticsProperties.Selected = true
        composeTestRule.onNodeWithText("選択カテゴリ")
            .assertIsDisplayed()
            .assert(SemanticsMatcher.expectValue(SemanticsProperties.Selected, true))

        // 非選択カテゴリは SemanticsProperties.Selected = false
        composeTestRule.onNodeWithText("非選択カテゴリ")
            .assertIsDisplayed()
            .assert(SemanticsMatcher.expectValue(SemanticsProperties.Selected, false))
    }

    @Test
    fun 選択カテゴリIDがnullのとき全カテゴリがselectedでない() {
        val categories = listOf(
            Category(id = 1, parentId = null, name = "カテゴリA"),
            Category(id = 2, parentId = null, name = "カテゴリB"),
        )

        composeTestRule.setContent {
            CategoryTreeDrawerContent(
                appState = buildAppState(categories),
                selectedCategoryId = null, // 選択なし
                expandedCategoryIds = emptySet(),
                onCategoryClick = {},
                onToggleExpand = {},
            )
        }

        composeTestRule.onNodeWithText("カテゴリA")
            .assert(SemanticsMatcher.expectValue(SemanticsProperties.Selected, false))
        composeTestRule.onNodeWithText("カテゴリB")
            .assert(SemanticsMatcher.expectValue(SemanticsProperties.Selected, false))
    }

    // ─────────────────────────────────────────────────────────────────────────
    // テスト観点 10: 初期展開済みの expandedCategoryIds を渡すと子カテゴリが最初から表示される
    //   （MainActivity.kt の LaunchedEffect で全カテゴリIDが expandedCategoryIds に追加されることに対応）
    // ─────────────────────────────────────────────────────────────────────────

    @Test
    fun 全カテゴリIDを展開済みとして渡すと子カテゴリが初期状態から表示される() {
        val categories = listOf(
            Category(id = 1, parentId = null, name = "親カテゴリ"),
            Category(id = 2, parentId = 1, name = "子カテゴリ"),
        )
        // MainActivity の変更後の初期値と同様に、全カテゴリIDのセットを渡す
        val allIds = categories.map { it.id }.toSet()

        composeTestRule.setContent {
            CategoryTreeDrawerContent(
                appState = buildAppState(categories),
                selectedCategoryId = null,
                expandedCategoryIds = allIds,
                onCategoryClick = {},
                onToggleExpand = {},
            )
        }

        // タップ操作なしで最初から子カテゴリが表示されていること
        composeTestRule.onNodeWithText("子カテゴリ").assertIsDisplayed()
        // 展開中を示す▼アイコンが表示されていること
        composeTestRule.onNodeWithText("▼").assertIsDisplayed()
        // 折りたたみ状態の▶アイコンは表示されないこと
        composeTestRule.onNodeWithText("▶").assertDoesNotExist()
    }

    // ─────────────────────────────────────────────────────────────────────────
    // テスト観点 11: 複数の親カテゴリが全展開済みのとき全ての子カテゴリが初期状態から表示される
    // ─────────────────────────────────────────────────────────────────────────

    @Test
    fun 複数の親カテゴリが全展開済みのとき全ての子カテゴリが初期状態から表示される() {
        val categories = listOf(
            Category(id = 1, parentId = null, name = "親カテゴリA"),
            Category(id = 2, parentId = 1, name = "子カテゴリA1"),
            Category(id = 3, parentId = null, name = "親カテゴリB"),
            Category(id = 4, parentId = 3, name = "子カテゴリB1"),
        )
        val allIds = categories.map { it.id }.toSet()

        composeTestRule.setContent {
            CategoryTreeDrawerContent(
                appState = buildAppState(categories),
                selectedCategoryId = null,
                expandedCategoryIds = allIds,
                onCategoryClick = {},
                onToggleExpand = {},
            )
        }

        // 全ての親・子カテゴリがタップなしで最初から表示されていること
        composeTestRule.onNodeWithText("親カテゴリA").assertIsDisplayed()
        composeTestRule.onNodeWithText("子カテゴリA1").assertIsDisplayed()
        composeTestRule.onNodeWithText("親カテゴリB").assertIsDisplayed()
        composeTestRule.onNodeWithText("子カテゴリB1").assertIsDisplayed()
    }

    // ─────────────────────────────────────────────────────────────────────────
    // テスト観点 12: 多階層カテゴリで全展開済みのとき全レベルのカテゴリが初期状態から表示される
    // ─────────────────────────────────────────────────────────────────────────

    @Test
    fun 多階層カテゴリで全展開済みのとき全レベルのカテゴリが初期状態から表示される() {
        val categories = listOf(
            Category(id = 1, parentId = null, name = "祖父カテゴリ"),
            Category(id = 2, parentId = 1, name = "親カテゴリ"),
            Category(id = 3, parentId = 2, name = "孫カテゴリ"),
        )
        val allIds = categories.map { it.id }.toSet()

        composeTestRule.setContent {
            CategoryTreeDrawerContent(
                appState = buildAppState(categories),
                selectedCategoryId = null,
                expandedCategoryIds = allIds,
                onCategoryClick = {},
                onToggleExpand = {},
            )
        }

        // 3階層全てがタップなしで最初から表示されていること
        composeTestRule.onNodeWithText("祖父カテゴリ").assertIsDisplayed()
        composeTestRule.onNodeWithText("親カテゴリ").assertIsDisplayed()
        composeTestRule.onNodeWithText("孫カテゴリ").assertIsDisplayed()
    }
}
