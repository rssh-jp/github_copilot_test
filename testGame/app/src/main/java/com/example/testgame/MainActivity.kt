package com.example.testgame

import android.os.Bundle
import android.view.View
import com.google.androidgamesdk.GameActivity

/**
 * MainActivity overview:
 * - Hosts the native GL surface provided by GameActivity and adds an Android overlay for HUD and
 *   controls via addContentView. The native renderer keeps ownership of the GL surface.
 * - UI periodically polls native state over JNI (camera offsets, unit info). Polling is intentionally
 *   lightweight (250ms) to avoid excessive JNI crossing.
 * - UI dispatches user commands (move/stop/pan) via simple JNI entrypoints. UI should remain thin and
 *   avoid embedding game logic; domain rules live in the native layers.
 */

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
    private external fun getTargetPositionX(): Float
    private external fun getTargetPositionY(): Float
    private external fun getUnitStatusString(): String
    
    // Clear persisted selected unit in native side
    private external fun clearPersistSelectUnit()
    
    // JNI Native functions - ユニット選択関連
    private external fun getSelectedUnitId(): Int
    private external fun clearUnitSelection()
    
    // JNI Native functions - Renderer status (camera / HUD)
    private external fun getCameraOffsetX(): Float
    private external fun getCameraOffsetY(): Float
    private external fun getElapsedTime(): Float
    private external fun getFactionCountsPacked(): Int
    private external fun getUnit1EffectiveMoveSpeed(): Float
    
    // JNI Native functions - ユニットコマンド用
    private external fun moveUnit()
    private external fun stopUnit()
    // Move persisted selected unit to a random point inside current visible view (screen px)
    private external fun moveSelectedUnitToRandomInView(screenW: Int, screenH: Int): Boolean
    // Debug: move all units to random positions in view
    private external fun moveAllUnitsToRandomInView(screenW: Int, screenH: Int): Boolean
    // Debug: reset all units to initial positions (teleport)
    private external fun resetAllUnitsToInitialPositions(): Boolean
    // JNI Native functions - Camera pan commands (delta in world units)
    private external fun panCameraBy(dx: Float, dy: Float)
    // JNI: toggle native attack-range visualization
    private external fun setShowAttackRanges(show: Boolean)

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
    // Ensure status board is visually on top of other overlay controls so its buttons are tappable
    try {
        statusBoard.bringToFront()
        statusBoard.elevation = 200f
        statusBoard.invalidate()
    } catch (e: Throwable) {
        // ignore if not yet attached
    }
    // a centered dialog to show selected unit info (created lazily)
    var centerUnitDialog: com.example.testgame.StatusBoardView? = null
    // show the center dialog only if the user explicitly touched a unit
    var centerDialogShowRequested: Boolean = false
        val handler = android.os.Handler(mainLooper)
        val updateTask = object : Runnable {
            override fun run() {
                try {
                    // Keep the top-left status board showing world info always
                    val camX = getCameraOffsetX()
                    val camY = getCameraOffsetY()
                    val elapsed = getElapsedTime()
                    val packed = getFactionCountsPacked()
                    val unit1Speed = getUnit1EffectiveMoveSpeed()
                    val f1 = packed and 0xFF
                    val f2 = (packed shr 8) and 0xFF
                    val f3 = (packed shr 16) and 0xFF
                    val f4 = (packed shr 24) and 0xFF
                    val worldText = String.format(
                        "Center: %.2f, %.2f\nF1: %d F2: %d F3: %d F4: %d\nTime: %.1fs\nUnit1 Speed: %.2f",
                        camX, camY, f1, f2, f3, f4, elapsed, unit1Speed
                    )
                    // 左上のステータスボードは常にワールド情報を表示
                    statusBoard.setText(worldText)
                    statusBoard.setButtonsVisible(false)
                    
                    // ユニット選択チェック - 中央ダイアログで表示
                    val selectedUnitId = getSelectedUnitId()
                    if (selectedUnitId >= 0) {
                        // ユニットが選択されている場合：中央ダイアログでステータス表示をリクエスト
                        centerDialogShowRequested = true
                    }

                    // Now check for a selected unit; if present, show a centered dialog with details
                    val unitName = getUnitName()
                    // Show dialog only when user requested it via touch. Once shown it stays visible
                    // until the user presses Close (centerDialogShowRequested is cleared there).
                    if (centerDialogShowRequested) {
                        if (unitName.isNotEmpty()) {
                            // create center dialog lazily when we actually have a selected unit
                            if (centerUnitDialog == null) {
                                val dialog = com.example.testgame.StatusBoardView(this@MainActivity)
                                dialog.setBoardSize(390, 320)
                                dialog.setBackgroundColorHex("#CC000000")
                                dialog.setTextColorHex("#FFFFFFFF")
                                // layout params to center it
                                val lp = android.widget.FrameLayout.LayoutParams(
                                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT,
                                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT
                                )
                                lp.gravity = android.view.Gravity.CENTER
                                (overlay as? android.view.ViewGroup)?.addView(dialog, lp)
                                // Ensure dialog is on top so its buttons receive touches
                                try {
                                    dialog.bringToFront()
                                    dialog.elevation = 300f
                                } catch (e: Throwable) {
                                    // ignore
                                }
                                centerUnitDialog = dialog
                            }

                            // populate dialog with unit status using the new showUnitStatus method
                            val dialog = centerUnitDialog!!
                            val name = unitName
                            val curHp = getCurrentHp()
                            val maxHp = getMaxHp()
                            val minAtk = getMinAttack()
                            val maxAtk = getMaxAttack()
                            val def = getDefense()
                            val px = getPositionX()
                            val py = getPositionY()
                            val tx = getTargetPositionX()
                            val ty = getTargetPositionY()
                            
                            val attackText = if (minAtk == maxAtk) minAtk.toString() else "$minAtk-$maxAtk"
                            dialog.showUnitStatus(name, curHp, maxHp, attackText, def, px, py, tx, ty)
                            dialog.configureButtons(
                                Triple("Move", View.generateViewId()) {
                                    // Move the currently persisted selected unit to a random visible location
                                    try {
                                        val metrics = resources.displayMetrics
                                        val screenW = metrics.widthPixels
                                        val screenH = metrics.heightPixels
                                        val moved = moveSelectedUnitToRandomInView(screenW, screenH)
                                        if (!moved) {
                                            // No persisted selection; inform user instead of falling back to unit 1
                                            android.widget.Toast.makeText(this@MainActivity, "No unit selected", android.widget.Toast.LENGTH_SHORT).show()
                                        }
                                    } catch (e: Throwable) {
                                        // If JNI call fails for any reason, inform user
                                        android.widget.Toast.makeText(this@MainActivity, "Move failed", android.widget.Toast.LENGTH_SHORT).show()
                                    }
                                },
                                Triple("Stop", View.generateViewId()) { stopUnit() },
                                Triple("Close", View.generateViewId()) {
                                    dialog.visibility = View.GONE
                                    centerDialogShowRequested = false
                                    try {
                                        clearUnitSelection() // 新しいユニット選択システムをクリア
                                        clearPersistSelectUnit() // 従来のシステムもクリア
                                    } catch (e: Throwable) {
                                        // ignore
                                    }
                                }
                            )
                            dialog.visibility = View.VISIBLE
                        } else {
                            // Selected unit might have been cleared in native side; keep showing last known
                            // dialog contents until Close is pressed. Do NOT create or overwrite the dialog
                            // when no unit is currently selected on the native side.
                            // If there's no existing dialog (shouldn't happen because we only set
                            // centerDialogShowRequested when a unit was selected) then do nothing.
                        }
                    }
                } catch (e: Throwable) {
                    // ignore
                }
                handler.postDelayed(this, 250)
            }
        }
        handler.postDelayed(updateTask, 250)

        // Configure the status board appearance and buttons. Caller can change these at runtime.
    statusBoard.setBoardSize(300, 270) // dp units (increased height)
        statusBoard.setBackgroundColorHex("#66000000")
        statusBoard.setTextColorHex("#FFFFFFFF")

        // Example: dynamic buttons - only Close, OK, NG
        statusBoard.configureButtons(
            Triple("閉じる", View.generateViewId()) { /* close */ },
            Triple("OK", View.generateViewId()) { /* ok */ },
            Triple("NG", View.generateViewId()) { /* ng */ }
        )

        // For the top-left HUD we don't want to show buttons by default; hide them.
        statusBoard.setButtonsVisible(false)

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

        // Wire debug buttons
        try {
            val btnMoveAll = overlay.findViewById<android.widget.Button>(R.id.button_move_all)
            val btnResetAll = overlay.findViewById<android.widget.Button>(R.id.button_reset_all)
            btnMoveAll?.setOnClickListener {
                try {
                    val metrics = resources.displayMetrics
                    moveAllUnitsToRandomInView(metrics.widthPixels, metrics.heightPixels)
                } catch (e: Throwable) {
                    android.widget.Toast.makeText(this@MainActivity, "Move all failed", android.widget.Toast.LENGTH_SHORT).show()
                }
            }
            btnResetAll?.setOnClickListener {
                try {
                    val ok = resetAllUnitsToInitialPositions()
                    if (!ok) android.widget.Toast.makeText(this@MainActivity, "Reset failed", android.widget.Toast.LENGTH_SHORT).show()
                } catch (e: Throwable) {
                    android.widget.Toast.makeText(this@MainActivity, "Reset failed", android.widget.Toast.LENGTH_SHORT).show()
                }
            }
        } catch (e: Throwable) {
            // ignore if layout not available
        }

        // Wire the attack-range toggle switch in the status board (if present)
        try {
            val rangeSwitch = overlay.findViewById<android.widget.Switch>(R.id.switch_attack_range)
            rangeSwitch?.setOnCheckedChangeListener { _, isChecked ->
                try {
                    setShowAttackRanges(isChecked)
                } catch (e: Throwable) {
                    // ignore JNI failures
                }
            }
        } catch (e: Throwable) {
            // ignore if switch not found or APIs missing
        }

        // Remove touch listener - let all touch events go to native GameActivity
        // This allows TouchInputHandler to receive raw touch events for pinch processing
        // UI button handling will be done through click listeners instead
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