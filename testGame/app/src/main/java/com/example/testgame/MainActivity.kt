package com.example.testgame

import android.os.Bundle
import android.view.View
import com.google.androidgamesdk.GameActivity

/**
 * GameActivity専用のメインアクティビティ
 * ネイティブレンダリングによる2Dシミュレーションゲーム
 */
class MainActivity : GameActivity() {
    companion object {
        init {
            System.loadLibrary("testgame")
        }
    }

    // JNI Native functions - ゲームコントロール用
    private external fun onTouch(x: Float, y: Float): Boolean
    
    // JNI Native functions - ユニット情報取得用
    private external fun getUnitName(): String
    private external fun getCurrentHp(): Int
    private external fun getMaxHp(): Int
    private external fun getMinAttack(): Int
    private external fun getMaxAttack(): Int
    private external fun getDefense(): Int
    private external fun getPositionX(): Float
    private external fun getPositionY(): Float
    private external fun getUnitStatusString(): String
    
    // JNI Native functions - ユニットコマンド用
    private external fun moveUnit()
    private external fun stopUnit()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // GameActivityは独自のサーフェスでネイティブレンダリングを行う
        // カスタムレイアウトやUI要素は不要
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            hideSystemUi()
        }
    }

    /**
     * イマーシブモードでシステムUIを非表示にする
     * ゲーム体験を向上させるためフルスクリーン表示
     */
    private fun hideSystemUi() {
        val decorView = window.decorView
        decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_FULLSCREEN)
    }
}