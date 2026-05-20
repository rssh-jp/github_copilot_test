package com.example.testapp2.data.db

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.room.migration.Migration
import androidx.sqlite.db.SupportSQLiteDatabase

@Database(
    entities = [
        SessionEntity::class,
        UserEntity::class,
        ScoreRecordEntity::class,
        ScoreItemEntity::class,
        CategoryEntity::class,
    ],
    version = 2,
    exportSchema = false,
)
abstract class AppDatabase : RoomDatabase() {
    abstract fun sessionDao(): SessionDao
    abstract fun userDao(): UserDao
    abstract fun scoreDao(): ScoreDao
    abstract fun categoryDao(): CategoryDao

    companion object {
        @Volatile private var INSTANCE: AppDatabase? = null

        // バージョン 1 → 2 へのマイグレーション
        // 1. categories テーブルを新規作成
        // 2. score_records テーブルに sectionId カラムを追加
        val MIGRATION_1_2 = object : Migration(1, 2) {
            override fun migrate(database: SupportSQLiteDatabase) {
                database.execSQL(
                    """
                    CREATE TABLE IF NOT EXISTS `categories` (
                        `id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
                        `sessionId` INTEGER NOT NULL,
                        `parentId` INTEGER,
                        `name` TEXT NOT NULL,
                        `type` TEXT NOT NULL DEFAULT 'FOLDER',
                        `sortOrder` INTEGER NOT NULL DEFAULT 0,
                        FOREIGN KEY(`sessionId`) REFERENCES `sessions`(`id`) ON DELETE CASCADE
                    )
                    """.trimIndent()
                )
                database.execSQL(
                    "CREATE INDEX IF NOT EXISTS `index_categories_sessionId` ON `categories` (`sessionId`)"
                )
                database.execSQL(
                    "CREATE INDEX IF NOT EXISTS `index_categories_parentId` ON `categories` (`parentId`)"
                )
                // 既存の score_records に sectionId カラムを追加（既存レコードは NULL）
                database.execSQL("ALTER TABLE `score_records` ADD COLUMN `sectionId` INTEGER")
            }
        }

        fun get(context: Context): AppDatabase = INSTANCE ?: synchronized(this) {
            INSTANCE ?: Room.databaseBuilder(
                context.applicationContext,
                AppDatabase::class.java,
                "scores.db"
            )
                .addMigrations(MIGRATION_1_2)
                .build()
                .also { INSTANCE = it }
        }
    }
}
