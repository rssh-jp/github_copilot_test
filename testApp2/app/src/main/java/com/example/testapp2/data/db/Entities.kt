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
    val sectionId: Int? = null,   // 紐づくセクション（SECTION型カテゴリ）のID
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

// カテゴリ（フォルダ/セクション）エンティティ
// 自己参照の外部キーは Room の制限を避けるため設定しない（parentId は nullable Int で管理）
@Entity(
    tableName = "categories",
    foreignKeys = [
        ForeignKey(
            entity = SessionEntity::class,
            parentColumns = ["id"],
            childColumns = ["sessionId"],
            onDelete = ForeignKey.CASCADE,
        ),
    ],
    indices = [
        Index(value = ["sessionId"]),
        Index(value = ["parentId"]),
    ],
)
data class CategoryEntity(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    val sessionId: Int,
    val parentId: Int?,       // null はルートカテゴリ（セッション直下）
    val name: String,
    val type: String,         // "FOLDER" または "SECTION"
    val sortOrder: Int = 0,
)
