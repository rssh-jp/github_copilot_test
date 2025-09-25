package com.example.testgame

import android.content.Context
import android.graphics.Color
import android.util.AttributeSet
import android.view.LayoutInflater
import android.view.View
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView

/**
 * StatusBoardView - lightweight HUD component used by the Android overlay.
 *
 * Responsibilities:
 * - Render short textual status and an optional row of buttons.
 * - Keep UI concerns only; do not implement domain logic here. Callers should supply callbacks.
 *
 * API expectations:
 * - setBoardSize expects dp units (converted to px internally).
 * - configureButtons accepts label/id/callback tuples; callbacks are invoked on the UI thread.
 */
class StatusBoardView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : LinearLayout(context, attrs, defStyleAttr) {

    private val root: LinearLayout
    private val textView: TextView
    private val buttonContainer: LinearLayout

    init {
        orientation = VERTICAL
        val v = LayoutInflater.from(context).inflate(R.layout.view_status_board, this, true)
        root = v.findViewById(R.id.status_board_root)
        textView = v.findViewById(R.id.status_board_text)
        buttonContainer = v.findViewById(R.id.status_board_buttons)
    }

    fun setBoardSize(widthDp: Int, heightDp: Int) {
        val scale = resources.displayMetrics.density
        val w = (widthDp * scale).toInt()
        val h = (heightDp * scale).toInt()
        root.layoutParams = LayoutParams(w, h)
    }

    fun setBackgroundColorHex(hex: String) {
        try {
            root.setBackgroundColor(Color.parseColor(hex))
        } catch (e: Exception) {
            // ignore parsing errors
        }
    }

    fun setTextColorHex(hex: String) {
        try {
            textView.setTextColor(Color.parseColor(hex))
        } catch (e: Exception) {
            // ignore
        }
    }

    fun setText(text: String) {
        textView.text = text
    }

    /**
     * ユニットステータス情報を表示します。
     */
    fun showUnitStatus(name: String, hp: Int, maxHp: Int, attack: String, defense: Int, 
                      posX: Float, posY: Float, targetX: Float, targetY: Float) {
        val statusText = buildString {
            appendLine("=== ユニット情報 ===")
            appendLine("名前: $name")
            appendLine("HP: $hp / $maxHp")
            appendLine("攻撃力: $attack")
            appendLine("防御力: $defense")
            appendLine("位置: (${String.format("%.1f", posX)}, ${String.format("%.1f", posY)})")
            appendLine("目標: (${String.format("%.1f", targetX)}, ${String.format("%.1f", targetY)})")
        }
        setText(statusText)
        visibility = View.VISIBLE
    }

    /**
     * ステータスボードを非表示にします。
     */
    fun hideStatus() {
        visibility = View.GONE
    }

    /**
     * Show or hide the button container. When hidden, existing buttons remain but are not visible.
     */
    fun setButtonsVisible(visible: Boolean) {
        buttonContainer.visibility = if (visible) View.VISIBLE else View.GONE
    }

    /**
     * Query whether buttons are currently visible.
     */
    fun areButtonsVisible(): Boolean {
        return buttonContainer.visibility == View.VISIBLE
    }

    /**
     * Configure buttons. Each element in labels is a Triple of (label, id, clickListener).
     * If clickListener is null, the button is not clickable.
     */
    fun configureButtons(vararg buttons: Triple<String, Int, (() -> Unit)?>) {
        buttonContainer.removeAllViews()
        for (b in buttons) {
            val btn = Button(context)
            btn.text = b.first
            btn.id = b.second
            val lp = LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT)
            lp.marginEnd = (6 * resources.displayMetrics.density).toInt()
            btn.layoutParams = lp
            btn.setBackgroundColor(Color.BLACK)
            btn.setTextColor(Color.WHITE)
            val cb = b.third
            if (cb != null) {
                btn.setOnClickListener { cb() }
            }

            // add a small press animation for visual feedback: scale down on press, restore on release/cancel
            btn.setOnTouchListener { v, ev ->
                when (ev.action) {
                    android.view.MotionEvent.ACTION_DOWN -> {
                        v.animate().scaleX(0.92f).scaleY(0.92f).alpha(0.95f).setDuration(80).start()
                    }
                    android.view.MotionEvent.ACTION_UP, android.view.MotionEvent.ACTION_CANCEL -> {
                        v.animate().scaleX(1f).scaleY(1f).alpha(1f).setDuration(100).start()
                    }
                }
                // Return false so that click events are still delivered to OnClickListener
                false
            }
            buttonContainer.addView(btn)
        }
    }
}
