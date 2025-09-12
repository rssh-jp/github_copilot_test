package com.example.testgame

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.MotionEvent
import android.view.View
import android.widget.*
import com.google.androidgamesdk.GameActivity

class MainActivity : GameActivity() {
    companion object {
        init {
            System.loadLibrary("testgame")
        }
    }

    // ステータス更新用ハンドラ
    private val statusHandler = Handler(Looper.getMainLooper())
    private var statusUpdateRunnable: Runnable? = null
    private var isStatusVisible = false

    // JNI Native functions
    private external fun onTouch(x: Float, y: Float): Boolean
    private external fun getUnitName(): String
    private external fun getCurrentHp(): Int
    private external fun getMaxHp(): Int
    private external fun getMinAttack(): Int
    private external fun getMaxAttack(): Int
    private external fun getDefense(): Int
    private external fun getPositionX(): Float
    private external fun getPositionY(): Float
    private external fun getUnitStatusString(): String
    private external fun moveUnit()
    private external fun stopUnit()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // GameActivityは独自のサーフェスを持つため、カスタムレイアウトは不要
        // ネイティブ側でレンダリングが処理される
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            hideSystemUi()
        }
    }

    private fun hideSystemUi() {
        val decorView = window.decorView
        decorView.systemUiVisibility = (View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                or View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_FULLSCREEN)
    }

    override fun onDestroy() {
        super.onDestroy()
        statusUpdateRunnable?.let {
            statusHandler.removeCallbacks(it)
        }
    }
}