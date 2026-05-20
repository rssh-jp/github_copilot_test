package com.example.testapp2.data.db

import androidx.room.Entity
import androidx.room.PrimaryKey
import androidx.room.ForeignKey
import androidx.room.Index

@Entity(tableName = "sessions")
data class SessionEntity(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    val name: String,
    val elapsedTime: Int,
    val categoryId: Int? = null, // 所属カテゴリID（null = ルート直下）
)

@Entity(
    tableName = "users",
    foreignKeys = [
        ForeignKey(
            entity = SessionEntity::class,
            parentColumns = ["id"],
            childColumns = ["sessionId"],
            onDelete = ForeignKey.CASCADE,
        )
    ],
    indices = [Index(value = ["sessionId"])],
)
data class UserEntity(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    val sessionId: Int,
    val name: String,
    val score: Int,
)

@Entity(
    tableName = "score_records",
    foreignKeys = [
        ForeignKey(
            entity = SessionEntity::class,
            parentColumns = ["id"],
            childColumns = ["sessionId"],
            onDelete = ForeignKey.CASCADE,
        )
    ],
    indices = [Index(value = ["sessionId"])],
)
data class ScoreRecordEntity(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    val sessionId: Int,
    val timestamp: Long,
)

@Entity(
    tableName = "score_items",
    foreignKeys = [
        ForeignKey(
            entity = ScoreRecordEntity::class,
            parentColumns = ["id"],
            childColumns = ["recordId"],
            onDelete = ForeignKey.CASCADE,
        )
    ],
    indices = [Index(value = ["recordId"])],
)
data class ScoreItemEntity(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    val recordId: Int,
    val userId: Int,
    val delta: Int,
)

/**
 * カテゴリエンティティ（ツリー構造でセッションを整理するためのカテゴリ）
 * parentId が null の場合はルートカテゴリ
 */
@Entity(
    tableName = "categories",
    indices = [Index(value = ["parentId"])],
)
data class CategoryEntity(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    val parentId: Int?,        // null = ルート直下
    val name: String,
    val sortOrder: Int = 0,
)
