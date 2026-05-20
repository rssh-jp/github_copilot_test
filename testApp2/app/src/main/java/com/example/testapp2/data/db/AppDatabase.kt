package com.example.testapp2.data.db

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.room.migration.Migration
import androidx.sqlite.db.SupportSQLiteDatabase

/** DBバージョン 1→2 のマイグレーション: categories テーブル追加、sessions に categoryId を追加 */
val MIGRATION_1_2 = object : Migration(1, 2) {
    override fun migrate(database: SupportSQLiteDatabase) {
        // categories テーブルを作成
        database.execSQL(
            """
            CREATE TABLE IF NOT EXISTS `categories` (
                `id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
                `parentId` INTEGER,
                `name` TEXT NOT NULL,
                `sortOrder` INTEGER NOT NULL DEFAULT 0
            )
            """.trimIndent()
        )
        // parentId インデックスを作成
        database.execSQL(
            "CREATE INDEX IF NOT EXISTS `index_categories_parentId` ON `categories` (`parentId`)"
        )
        // sessions テーブルに categoryId カラムを追加（既存レコードは NULL になる）
        database.execSQL("ALTER TABLE `sessions` ADD COLUMN `categoryId` INTEGER")
    }
}

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
        fun get(context: Context): AppDatabase = INSTANCE ?: synchronized(this) {
            INSTANCE ?: Room.databaseBuilder(
                context.applicationContext,
                AppDatabase::class.java,
                "scores.db"
            )
            .addMigrations(MIGRATION_1_2)
            .build().also { INSTANCE = it }
        }
    }
}
