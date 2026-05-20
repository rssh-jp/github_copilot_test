package com.example.testapp2.data

import android.util.Log
import androidx.compose.runtime.mutableStateListOf
import java.util.Date
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * セッション情報を表すデータクラス
 * @param id セッションの一意識別子
 * @param name セッション名
 * @param elapsedTime セッション実行の累積経過時間（秒）
 */
data class Session(
    val id: Int,
    val name: String,
    val elapsedTime: Int,
)

/**
 * セッション参加ユーザー情報を表すデータクラス
 * @param id ユーザーの一意識別子
 * @param sessionId 所属するセッションのID
 * @param name ユーザー名
 * @param score 現在の累積スコア（履歴から計算される）
 */
data class User(
    val id: Int,
    val sessionId: Int,
    val name: String,
    val score: Int = 0,
)

/**
 * スコア登録履歴を記録するデータクラス
 * @param id スコアレコードの一意識別子
 * @param sessionId 対象セッションのID
 * @param timestamp 記録日時
 * @param scores ユーザーID -> そのラウンドでの得点のマップ
 * @param sectionId 紐づくセクション（SECTION型カテゴリ）のID（null=セクション未分類）
 */
data class ScoreRecord(
    val id: Int,
    val sessionId: Int,
    val timestamp: Date,
    val scores: Map<Int, Int>,
    val sectionId: Int? = null,
)

/** カテゴリ種別 */
enum class CategoryType { FOLDER, SECTION }

/**
 * カテゴリ（フォルダまたはセクション）を表すデータクラス
 * @param id カテゴリの一意識別子
 * @param sessionId 所属するセッションのID
 * @param parentId 親カテゴリのID（null はルート）
 * @param name カテゴリ名
 * @param type FOLDER（入れ子可能）または SECTION（スコア投入）
 * @param sortOrder 同階層内での表示順
 */
data class Category(
    val id: Int,
    val sessionId: Int,
    val parentId: Int?,
    val name: String,
    val type: CategoryType,
    val sortOrder: Int = 0,
)

/**
 * アプリケーションの状態を管理するクラス
 * - セッション、ユーザー、スコア履歴をメモリ上で管理
 * - Room データベースとの同期機能を提供
 * - ユーザーの累積スコアを履歴から自動計算
 */
class AppState {
    /** セッション一覧（アプリ起動時にDBから読み込み） */
    val sessions = mutableStateListOf<Session>()
    /** ユーザー一覧（各セッションの参加者） */
    val users = mutableStateListOf<User>()
    /** スコア登録履歴一覧（ゲームの記録） */
    val scoreRecords = mutableStateListOf<ScoreRecord>()
    /** カテゴリ一覧（全セッションのカテゴリ・セクションを保持） */
    val categories = mutableStateListOf<Category>()

    // =====================
    // 基本的なデータ取得メソッド
    // =====================

    /**
     * 指定セッションに参加しているユーザー一覧を取得
     * @param sessionId セッションID
     * @return そのセッションのユーザーリスト
     */
    fun getSessionUsers(sessionId: Int): List<User> {
        return users.filter { it.sessionId == sessionId }
    }

    /**
     * 指定セッションのスコア履歴を取得
     * @param sessionId セッションID
     * @return そのセッションのスコア記録リスト
     */
    fun getSessionScoreHistory(sessionId: Int): List<ScoreRecord> {
        return scoreRecords.filter { it.sessionId == sessionId }
    }

    /**
     * 指定セッションのスコア履歴を取得（getSessionScoreHistoryのエイリアス）
     */
    fun getScoreHistory(sessionId: Int): List<ScoreRecord> {
        return getSessionScoreHistory(sessionId)
    }

    /**
     * 特定のスコアレコードを取得
     * @param sessionId セッションID
     * @param scoreId スコアレコードID
     * @return 該当するスコアレコード、見つからない場合はnull
     */
    fun getScoreRecord(sessionId: Int, scoreId: Int): ScoreRecord? {
        val history = getScoreHistory(sessionId)
        return history.find { it.id == scoreId }
    }

    // =====================
    // データ変更メソッド（メモリ操作）
    // =====================

    /**
     * 新しいセッションを追加
     * @param name セッション名
     * @return 作成されたセッションオブジェクト
     */
    fun addSession(name: String): Session {
        val nextId = (sessions.maxOfOrNull { it.id } ?: 0) + 1
        val session = Session(nextId, name, 0)
        sessions.add(session)
        return session
    }

    /**
     * 既存のセッションをコピーして新しいセッションを作成（ユーザーのみコピー）
     * @param sourceSessionId コピー元のセッションID
     * @param newName 新しいセッション名
     * @return 作成されたセッションオブジェクト、コピー元が見つからない場合はnull
     */
    fun copySession(sourceSessionId: Int, newName: String): Session? {
        val sourceSession = sessions.find { it.id == sourceSessionId } ?: return null
        val sourceUsers = getSessionUsers(sourceSessionId)
        
        // 新しいセッションを作成
        val newSession = addSession(newName)
        
        // ユーザーをコピー（スコアは0で初期化）
        sourceUsers.forEach { user ->
            addUserToSession(newSession.id, user.name)
        }
        
        return newSession
    }

    /**
     * セッションにユーザーを追加
     * @param sessionId 対象セッションID
     * @param userName ユーザー名
     * @return 作成されたユーザーオブジェクト
     */
    fun addUserToSession(sessionId: Int, userName: String): User {
        val nextId = (users.maxOfOrNull { it.id } ?: 0) + 1
        val user = User(nextId, sessionId, userName)
        users.add(user)
        return user
    }

    /**
     * セッション名を更新
     * @param sessionId 対象セッションID
     * @param newName 新しいセッション名
     */
    fun updateSession(sessionId: Int, newName: String) {
        val index = sessions.indexOfFirst { it.id == sessionId }
        if (index >= 0) {
            sessions[index] = sessions[index].copy(name = newName)
        }
    }

    /**
     * セッションの経過時間を更新
     * @param sessionId 対象セッションID
     * @param elapsedSeconds 累積経過時間（秒）
     */
    fun updateSessionElapsed(sessionId: Int, elapsedSeconds: Int) {
        val index = sessions.indexOfFirst { it.id == sessionId }
        if (index >= 0) {
            sessions[index] = sessions[index].copy(elapsedTime = elapsedSeconds)
        }
    }

    /**
     * スコア記録を追加し、ユーザーの累積スコアを再計算
     * @param sessionId 対象セッションID
     * @param userScores ユーザーID -> そのラウンドの得点のマップ
     * @param sectionId 紐づくセクションID（null=セクション未分類）
     * @return 作成されたスコアレコードのID
     */
    fun addScoreRecord(sessionId: Int, userScores: Map<Int, Int>, sectionId: Int? = null): Int {
        val scoreId = (scoreRecords.maxOfOrNull { it.id } ?: 0) + 1
        val record = ScoreRecord(scoreId, sessionId, Date(), userScores, sectionId)
        scoreRecords.add(record)
        // 合計値を再計算して反映
        recalcSessionTotals(sessionId)
        return scoreId
    }

    /**
     * セッションをメモリから削除（関連データも含む）
     * @param sessionId 削除対象セッションID
     */
    fun deleteSessionLocal(sessionId: Int) {
        // セッション削除
        sessions.removeAll { it.id == sessionId }
        // 関連ユーザー削除
        users.removeAll { it.sessionId == sessionId }
        // 関連スコア履歴削除
        scoreRecords.removeAll { it.sessionId == sessionId }
        // 関連カテゴリ削除
        categories.removeAll { it.sessionId == sessionId }
    }

    /**
     * スコアレコードを編集
     * @param sessionId 対象セッションID
     * @param scoreId 編集対象のスコアレコードID
     * @param newScores 新しいスコアマップ
     * @return 編集成功時true、レコードが見つからない場合false
     */
    fun updateScoreRecord(sessionId: Int, scoreId: Int, newScores: Map<Int, Int>): Boolean {
        val index = scoreRecords.indexOfFirst { it.sessionId == sessionId && it.id == scoreId }
        if (index == -1) return false
        scoreRecords[index] = scoreRecords[index].copy(scores = newScores)
        // 合計値を再計算して反映
        recalcSessionTotals(sessionId)
        return true
    }

    /**
     * スコアレコードを削除
     * @param sessionId 対象セッションID
     * @param scoreId 削除対象のスコアレコードID
     * @return 削除成功時true、レコードが見つからない場合false
     */
    fun deleteScoreRecord(sessionId: Int, scoreId: Int): Boolean {
        val index = scoreRecords.indexOfFirst { it.sessionId == sessionId && it.id == scoreId }
        if (index == -1) return false
        scoreRecords.removeAt(index)
        // 合計値を再計算して反映
        recalcSessionTotals(sessionId)
        return true
    }

    /**
     * 指定セッションのユーザー累積スコアを履歴から再計算
     * スコア記録の追加/編集/削除時に自動的に呼ばれる
     * @param sessionId 対象セッションID
     */
    private fun recalcSessionTotals(sessionId: Int) {
        val totals = mutableMapOf<Int, Int>()
        scoreRecords
            .filter { it.sessionId == sessionId }
            .forEach { record ->
                record.scores.forEach { (userId, delta) ->
                    totals[userId] = (totals[userId] ?: 0) + delta
                }
            }

        for (i in users.indices) {
            val u = users[i]
            if (u.sessionId == sessionId) {
                users[i] = u.copy(score = totals[u.id] ?: 0)
            }
        }
    }

    // =====================
    // Room データベース連携（永続化）
    // =====================

    /**
     * データベースから全データを読み込んでメモリ状態を初期化
     * アプリ起動時に呼ばれる
     * @param db Room データベースインスタンス
     */
    suspend fun loadFromDb(db: com.example.testapp2.data.db.AppDatabase) {
        try {
            val sessionEntities = withContext(Dispatchers.IO) { db.sessionDao().getAll() }
            val userEntities = withContext(Dispatchers.IO) { db.userDao().getAll() }
            val recordEntities = withContext(Dispatchers.IO) { db.scoreDao().getAllRecords() }
            val items = withContext(Dispatchers.IO) { db.scoreDao().getAllItems() }
            val categoryEntities = withContext(Dispatchers.IO) { db.categoryDao().getAll() }

            sessions.clear(); users.clear(); scoreRecords.clear(); categories.clear()

            sessions.addAll(sessionEntities.map { Session(it.id, it.name, it.elapsedTime) })
            users.addAll(userEntities.map { com.example.testapp2.data.User(it.id, it.sessionId, it.name, it.score) })

            val itemsByRecord = items.groupBy { it.recordId }
            scoreRecords.addAll(recordEntities.map { rec ->
                val map = itemsByRecord[rec.id]?.associate { it.userId to it.delta } ?: emptyMap()
                ScoreRecord(id = rec.id, sessionId = rec.sessionId, timestamp = Date(rec.timestamp), scores = map, sectionId = rec.sectionId)
            })

            categories.addAll(categoryEntities.map {
                Category(
                    id = it.id,
                    sessionId = it.sessionId,
                    parentId = it.parentId,
                    name = it.name,
                    // valueOf は不正値で IllegalArgumentException を投げるため safe lookup を使用
                    type = runCatching { CategoryType.valueOf(it.type) }.getOrDefault(CategoryType.FOLDER),
                    sortOrder = it.sortOrder,
                )
            })

            sessions.map { it.id }.forEach { recalcSessionTotals(it) }
        } catch (e: Exception) {
            Log.e("AppState", "DB読み込みエラー: ${e.message}", e)
        }
    }

    /**
     * 新しいセッションをデータベースに保存
     * @param db Room データベースインスタンス
     * @param session 保存対象のセッション
     * @return データベースで生成されたID
     */
    suspend fun persistNewSession(db: com.example.testapp2.data.db.AppDatabase, session: Session): Int {
        return try {
            val newId = withContext(Dispatchers.IO) {
                db.sessionDao().insert(
                    com.example.testapp2.data.db.SessionEntity(name = session.name, elapsedTime = session.elapsedTime)
                ).toInt()
            }
            // 生成IDをメモリに反映
            val idx = sessions.indexOfFirst { it === session }
            if (idx >= 0) sessions[idx] = session.copy(id = newId)
            newId
        } catch (e: Exception) {
            Log.e("AppState", "セッション保存エラー: ${e.message}", e)
            session.id
        }
    }

    /**
     * セッションをコピーしてデータベースにも保存
     * @param db Room データベースインスタンス
     * @param sourceSessionId コピー元のセッションID
     * @param newName 新しいセッション名
     * @return 作成されたセッションオブジェクト、コピー元が見つからない場合はnull
     */
    suspend fun copySessionWithDb(db: com.example.testapp2.data.db.AppDatabase, sourceSessionId: Int, newName: String): Session? {
        val newSession = copySession(sourceSessionId, newName) ?: return null
        
        // データベースに保存
        val dbSessionId = persistNewSession(db, newSession)
        val newUsers = getSessionUsers(newSession.id)
        newUsers.forEach { user ->
            persistNewUser(db, user)
        }
        
        return sessions.find { it.id == dbSessionId }
    }

    /**
     * 新しいユーザーをデータベースに保存
     * @param db Room データベースインスタンス
     * @param user 保存対象のユーザー
     */
    suspend fun persistNewUser(db: com.example.testapp2.data.db.AppDatabase, user: User) {
        try {
            val newId = withContext(Dispatchers.IO) {
                db.userDao().insert(
                    com.example.testapp2.data.db.UserEntity(sessionId = user.sessionId, name = user.name, score = user.score)
                ).toInt()
            }
            val idx = users.indexOfFirst { it === user }
            if (idx >= 0) users[idx] = user.copy(id = newId)
        } catch (e: Exception) {
            Log.e("AppState", "ユーザー保存エラー: ${e.message}", e)
        }
    }

    /**
     * 新しいスコア記録をデータベースに保存
     * @param db Room データベースインスタンス
     * @param sessionId 対象セッションID
     * @param recordId スコアレコードID
     * @param userScores ユーザースコアマップ
     * @param timestamp 記録時刻（ミリ秒）
     * @param sectionId 紐づくセクションID（null=セクション未分類）
     */
    suspend fun persistNewScoreRecord(db: com.example.testapp2.data.db.AppDatabase, sessionId: Int, recordId: Int, userScores: Map<Int, Int>, timestamp: Long, sectionId: Int? = null) {
        try {
            withContext(Dispatchers.IO) {
                db.scoreDao().insertRecordWithItems(sessionId, recordId, timestamp, userScores, sectionId)
            }
        } catch (e: Exception) {
            Log.e("AppState", "スコア記録保存エラー: ${e.message}", e)
        }
    }

    /**
     * セッションのユーザー累積スコアをデータベースに同期
     * @param db Room データベースインスタンス
     * @param sessionId 対象セッションID
     */
    suspend fun persistSessionTotals(db: com.example.testapp2.data.db.AppDatabase, sessionId: Int) {
        try {
            withContext(Dispatchers.IO) {
                users.filter { it.sessionId == sessionId }.forEach { u ->
                    db.userDao().updateScore(u.id, u.score)
                }
            }
        } catch (e: Exception) {
            Log.e("AppState", "スコア合計保存エラー: ${e.message}", e)
        }
    }

    /**
     * セッション名をデータベースに保存
     * @param db Room データベースインスタンス
     * @param sessionId 対象セッションID
     * @param name 新しいセッション名
     */
    suspend fun persistUpdateSessionName(db: com.example.testapp2.data.db.AppDatabase, sessionId: Int, name: String) {
        try {
            withContext(Dispatchers.IO) { db.sessionDao().updateName(sessionId, name) }
        } catch (e: Exception) {
            Log.e("AppState", "セッション名更新エラー: ${e.message}", e)
        }
    }

    /**
     * セッションの経過時間をデータベースに保存
     * @param db Room データベースインスタンス
     * @param sessionId 対象セッションID
     * @param elapsedSeconds 累積経過時間（秒）
     */
    suspend fun persistUpdateSessionElapsed(db: com.example.testapp2.data.db.AppDatabase, sessionId: Int, elapsedSeconds: Int) {
        try {
            withContext(Dispatchers.IO) { db.sessionDao().updateElapsed(sessionId, elapsedSeconds) }
        } catch (e: Exception) {
            Log.e("AppState", "経過時間更新エラー: ${e.message}", e)
        }
    }

    /**
     * セッションをデータベースとメモリの両方から削除
     * 外部キー制約により関連データ（ユーザー、スコア記録）も連鎖削除される
     * @param db Room データベースインスタンス
     * @param sessionId 削除対象セッションID
     */
    suspend fun deleteSession(db: com.example.testapp2.data.db.AppDatabase, sessionId: Int) {
        try {
            withContext(Dispatchers.IO) { db.sessionDao().deleteById(sessionId) }
            deleteSessionLocal(sessionId)
        } catch (e: Exception) {
            Log.e("AppState", "セッション削除エラー: ${e.message}", e)
        }
    }

    // =====================
    // ユーザー削除
    // =====================

    /**
     * ユーザーをメモリから削除
     * @param userId 削除対象ユーザーID
     */
    fun removeUserLocal(userId: Int) {
        users.removeAll { it.id == userId }
    }

    /**
     * ユーザーをデータベースとメモリの両方から削除
     * @param db Room データベースインスタンス
     * @param userId 削除対象ユーザーID
     */
    suspend fun deleteUser(db: com.example.testapp2.data.db.AppDatabase, userId: Int) {
        try {
            withContext(Dispatchers.IO) { db.userDao().deleteById(userId) }
            removeUserLocal(userId)
        } catch (e: Exception) {
            Log.e("AppState", "ユーザー削除エラー: ${e.message}", e)
        }
    }

    // =====================
    // スコアレコードDB永続化
    // =====================

    /**
     * スコアレコードをデータベースから削除
     * score_items は CASCADE により自動削除される
     * @param db Room データベースインスタンス
     * @param scoreId 削除対象スコアレコードID
     */
    suspend fun persistDeleteScoreRecord(db: com.example.testapp2.data.db.AppDatabase, scoreId: Int) {
        try {
            withContext(Dispatchers.IO) { db.scoreDao().deleteRecord(scoreId) }
        } catch (e: Exception) {
            Log.e("AppState", "スコアレコードDB削除エラー: ${e.message}", e)
        }
    }

    /**
     * スコアレコードの内容をデータベースで更新
     * 既存の score_items を削除して新しい内容で再挿入する
     * @param db Room データベースインスタンス
     * @param scoreId 更新対象スコアレコードID
     * @param newScores 新しいスコアマップ（ユーザーID -> 得点）
     */
    suspend fun persistUpdateScoreRecord(db: com.example.testapp2.data.db.AppDatabase, scoreId: Int, newScores: Map<Int, Int>) {
        try {
            withContext(Dispatchers.IO) {
                db.scoreDao().deleteItemsByRecordId(scoreId)
                val items = newScores.map { (userId, delta) ->
                    com.example.testapp2.data.db.ScoreItemEntity(recordId = scoreId, userId = userId, delta = delta)
                }
                db.scoreDao().insertItems(items)
            }
        } catch (e: Exception) {
            Log.e("AppState", "スコアレコードDB更新エラー: ${e.message}", e)
        }
    }

    // =====================
    // カテゴリ管理（メモリ操作）
    // =====================

    /**
     * 指定親配下の直接子カテゴリ一覧を取得（sortOrder 順）
     * @param sessionId セッションID
     * @param parentId 親カテゴリID（null=ルート）
     */
    fun getChildCategories(sessionId: Int, parentId: Int?): List<Category> {
        return categories
            .filter { it.sessionId == sessionId && it.parentId == parentId }
            .sortedWith(compareBy({ it.sortOrder }, { it.id }))
    }

    /**
     * カテゴリを追加してメモリに反映
     * @param sessionId セッションID
     * @param parentId 親カテゴリID（null=ルート）
     * @param name カテゴリ名
     * @param type FOLDER または SECTION
     * @return 作成されたカテゴリ
     */
    fun addCategory(sessionId: Int, parentId: Int?, name: String, type: CategoryType): Category {
        val nextId = (categories.maxOfOrNull { it.id } ?: 0) + 1
        val category = Category(nextId, sessionId, parentId, name, type)
        categories.add(category)
        return category
    }

    /**
     * カテゴリをメモリから削除（子孫も含む再帰的削除）
     * @param categoryId 削除対象カテゴリID
     */
    fun deleteCategoryLocal(categoryId: Int) {
        // 子カテゴリを再帰的に削除
        categories.filter { it.parentId == categoryId }.map { it.id }
            .forEach { deleteCategoryLocal(it) }
        categories.removeAll { it.id == categoryId }
    }

    /**
     * カテゴリ名をメモリで更新
     * @param categoryId 対象カテゴリID
     * @param newName 新しいカテゴリ名
     */
    fun updateCategory(categoryId: Int, newName: String) {
        val index = categories.indexOfFirst { it.id == categoryId }
        if (index >= 0) categories[index] = categories[index].copy(name = newName)
    }

    /**
     * カテゴリとその全子孫の ID を収集する（DB 削除用）
     * deleteCategoryLocal の呼び出し前に実行すること
     * @param categoryId 対象カテゴリID
     * @return 対象カテゴリ + 全子孫の ID リスト
     */
    fun collectCategoryAndDescendantIds(categoryId: Int): List<Int> {
        val result = mutableListOf<Int>()
        fun collect(id: Int) {
            result.add(id)
            categories.filter { it.parentId == id }.forEach { collect(it.id) }
        }
        collect(categoryId)
        return result
    }

    // =====================
    // カテゴリ DB 永続化
    // =====================

    /**
     * 新しいカテゴリをデータベースに保存
     * @param db Room データベースインスタンス
     * @param category 保存対象カテゴリ
     * @return データベースで生成された ID
     */
    suspend fun persistNewCategory(db: com.example.testapp2.data.db.AppDatabase, category: Category): Int {
        return try {
            val newId = withContext(Dispatchers.IO) {
                db.categoryDao().insert(
                    com.example.testapp2.data.db.CategoryEntity(
                        sessionId = category.sessionId,
                        parentId = category.parentId,
                        name = category.name,
                        type = category.type.name,
                        sortOrder = category.sortOrder,
                    )
                ).toInt()
            }
            val idx = categories.indexOfFirst { it === category }
            if (idx >= 0) categories[idx] = category.copy(id = newId)
            newId
        } catch (e: Exception) {
            Log.e("AppState", "カテゴリ保存エラー: ${e.message}", e)
            category.id
        }
    }

    /**
     * カテゴリツリー（指定IDとその子孫）をデータベースから削除
     * deleteCategoryLocal と collectCategoryAndDescendantIds と組み合わせて使用する
     * @param db Room データベースインスタンス
     * @param categoryIds 削除対象の全カテゴリ ID リスト
     */
    suspend fun persistDeleteCategoryTree(db: com.example.testapp2.data.db.AppDatabase, categoryIds: List<Int>) {
        try {
            withContext(Dispatchers.IO) {
                // 1件ずつではなく一括削除でトランザクション安全に処理
                db.categoryDao().deleteByIds(categoryIds)
            }
        } catch (e: Exception) {
            Log.e("AppState", "カテゴリDB削除エラー: ${e.message}", e)
        }
    }

    /**
     * カテゴリ名をデータベースで更新
     * @param db Room データベースインスタンス
     * @param categoryId 対象カテゴリID
     * @param name 新しいカテゴリ名
     */
    suspend fun persistUpdateCategoryName(db: com.example.testapp2.data.db.AppDatabase, categoryId: Int, name: String) {
        try {
            withContext(Dispatchers.IO) { db.categoryDao().updateName(categoryId, name) }
        } catch (e: Exception) {
            Log.e("AppState", "カテゴリ名更新エラー: ${e.message}", e)
        }
    }
}
