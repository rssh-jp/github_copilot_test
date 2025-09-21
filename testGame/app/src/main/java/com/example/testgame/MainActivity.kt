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
    // JNI Native functions - Renderer status (camera / HUD)
    private external fun getCameraOffsetX(): Float
    private external fun getCameraOffsetY(): Float
    private external fun getElapsedTime(): Float
    private external fun getFactionCountsPacked(): Int
    
    // JNI Native functions - ユニットコマンド用
    private external fun moveUnit()
    private external fun stopUnit()
    // JNI Native functions - Camera pan commands (delta in world units)
    private external fun panCameraBy(dx: Float, dy: Float)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Do not replace the GameActivity's native surface. Instead inflate an overlay layout
        // and add it on top so the native GL surface remains visible.
        val overlay = layoutInflater.inflate(R.layout.overlay_status, null)
        val params = android.view.ViewGroup.LayoutParams(
            android.view.ViewGroup.LayoutParams.MATCH_PARENT,
            android.view.ViewGroup.LayoutParams.MATCH_PARENT
        )
        addContentView(overlay, params)

    // Start a periodic UI updater to poll native state and update the status board
    val statusBoard = overlay.findViewById<com.example.testgame.StatusBoardView>(R.id.status_board)
        val handler = android.os.Handler(mainLooper)
        val updateTask = object : Runnable {
            override fun run() {
                try {
                    // Query native state via JNI
                    val camX = getCameraOffsetX()
                    val camY = getCameraOffsetY()
                    val elapsed = getElapsedTime()
                    val packed = getFactionCountsPacked()
                    val f1 = packed and 0xFF
                    val f2 = (packed shr 8) and 0xFF
                    val f3 = (packed shr 16) and 0xFF
                    val f4 = (packed shr 24) and 0xFF
                    val text = String.format("Center: %.2f, %.2f\nF1: %d F2: %d F3: %d F4: %d\nTime: %.1fs", camX, camY, f1, f2, f3, f4, elapsed)
                    statusBoard.setText(text)
                } catch (e: Throwable) {
                    // ignore
                }
                handler.postDelayed(this, 250)
            }
        }
        handler.postDelayed(updateTask, 250)

        // Configure the status board appearance and buttons. Caller can change these at runtime.
        statusBoard.setBoardSize(200, 120) // dp units
        statusBoard.setBackgroundColorHex("#66000000")
        statusBoard.setTextColorHex("#FFFFFFFF")

        // Example: dynamic buttons - only Close, OK, NG
        statusBoard.configureButtons(
            Triple("閉じる", View.generateViewId()) { /* close */ },
            Triple("OK", View.generateViewId()) { /* ok */ },
            Triple("NG", View.generateViewId()) { /* ng */ }
        )

        // Attach listeners to simple black pan buttons so user can see and use them
        val btnUp = overlay.findViewById<View>(R.id.button_pan_up)
        val btnDown = overlay.findViewById<View>(R.id.button_pan_down)
        val btnLeft = overlay.findViewById<View>(R.id.button_pan_left)
        val btnRight = overlay.findViewById<View>(R.id.button_pan_right)

        val panAmount = 1.0f // world units per tap; adjust as needed

        btnUp.setOnClickListener {
            panCameraBy(0.0f, -panAmount)
        }
        btnDown.setOnClickListener {
            panCameraBy(0.0f, panAmount)
        }
        btnLeft.setOnClickListener {
            panCameraBy(-panAmount, 0.0f)
        }
        btnRight.setOnClickListener {
            panCameraBy(panAmount, 0.0f)
        }

        // Forward touches on the overlay (outside of buttons) to native onTouch with raw screen coordinates
        overlay.setOnTouchListener { v, ev ->
            if (ev.action == android.view.MotionEvent.ACTION_DOWN) {
                val rawX = ev.rawX
                val rawY = ev.rawY

                // If the touch is inside any of the pan buttons, do not forward — let buttons handle it
                val buttons = listOf(btnUp, btnDown, btnLeft, btnRight)
                for (btn in buttons) {
                    val loc = IntArray(2)
                    btn.getLocationOnScreen(loc)
                    val bx = loc[0].toFloat()
                    val by = loc[1].toFloat()
                    val bw = btn.width.toFloat()
                    val bh = btn.height.toFloat()
                    if (rawX >= bx && rawX <= bx + bw && rawY >= by && rawY <= by + bh) {
                        // let the button receive the touch
                        return@setOnTouchListener false
                    }
                }

                try {
                    // Forward to native
                    onTouch(rawX, rawY)
                } catch (e: Throwable) {
                    // ignore
                }
                // consume so children don't also get it
                return@setOnTouchListener true
            }
            false
        }
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