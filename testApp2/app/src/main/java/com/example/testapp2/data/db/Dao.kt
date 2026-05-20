package com.example.testapp2.data.db

import androidx.room.*

@Dao
interface SessionDao {
    @Insert
    suspend fun insert(session: SessionEntity): Long

    @Query("SELECT * FROM sessions")
    suspend fun getAll(): List<SessionEntity>

    @Query("UPDATE sessions SET name = :name WHERE id = :id")
    suspend fun updateName(id: Int, name: String)

    @Query("UPDATE sessions SET elapsedTime = :elapsed WHERE id = :id")
    suspend fun updateElapsed(id: Int, elapsed: Int)

    @Query("DELETE FROM sessions WHERE id = :id")
    suspend fun deleteById(id: Int)
}

@Dao
interface UserDao {
    @Insert
    suspend fun insert(user: UserEntity): Long

    @Query("SELECT * FROM users")
    suspend fun getAll(): List<UserEntity>

    @Query("SELECT * FROM users WHERE sessionId = :sessionId")
    suspend fun getBySession(sessionId: Int): List<UserEntity>

    @Query("UPDATE users SET score = :score WHERE id = :id")
    suspend fun updateScore(id: Int, score: Int)

    @Query("DELETE FROM users WHERE id = :id")
    suspend fun deleteById(id: Int)
}

@Dao
interface ScoreDao {
    @Insert
    suspend fun insertRecord(record: ScoreRecordEntity): Long

    @Insert
    suspend fun insertItems(items: List<ScoreItemEntity>)

    @Query("SELECT * FROM score_records WHERE sessionId = :sessionId ORDER BY id")
    suspend fun getRecords(sessionId: Int): List<ScoreRecordEntity>

    @Query("SELECT * FROM score_records")
    suspend fun getAllRecords(): List<ScoreRecordEntity>

    @Query("SELECT * FROM score_items WHERE recordId = :recordId")
    suspend fun getItems(recordId: Int): List<ScoreItemEntity>

    @Query("SELECT * FROM score_items")
    suspend fun getAllItems(): List<ScoreItemEntity>

    @Query("DELETE FROM score_records WHERE id = :id")
    suspend fun deleteRecord(id: Int)

    @Query("DELETE FROM score_items WHERE recordId = :recordId")
    suspend fun deleteItemsByRecordId(recordId: Int)

    @Transaction
    suspend fun insertRecordWithItems(sessionId: Int, recordId: Int, timestamp: Long, userDeltas: Map<Int, Int>, sectionId: Int? = null) {
        insertRecord(ScoreRecordEntity(id = recordId, sessionId = sessionId, timestamp = timestamp, sectionId = sectionId))
        val items = userDeltas.map { (userId, delta) ->
            ScoreItemEntity(recordId = recordId, userId = userId, delta = delta)
        }
        insertItems(items)
    }
}

// カテゴリ（FOLDER/SECTION）の CRUD 操作
@Dao
interface CategoryDao {
    @Insert
    suspend fun insert(category: CategoryEntity): Long

    @Query("SELECT * FROM categories")
    suspend fun getAll(): List<CategoryEntity>

    @Query("SELECT * FROM categories WHERE sessionId = :sessionId ORDER BY sortOrder, id")
    suspend fun getAllBySession(sessionId: Int): List<CategoryEntity>

    @Query("SELECT * FROM categories WHERE parentId = :parentId ORDER BY sortOrder, id")
    suspend fun getByParentId(parentId: Int): List<CategoryEntity>

    @Query("UPDATE categories SET name = :name WHERE id = :id")
    suspend fun updateName(id: Int, name: String)

    @Query("DELETE FROM categories WHERE id = :id")
    suspend fun deleteById(id: Int)

    // 複数カテゴリを一括削除（トランザクション安全）
    @Query("DELETE FROM categories WHERE id IN (:ids)")
    suspend fun deleteByIds(ids: List<Int>)
}
