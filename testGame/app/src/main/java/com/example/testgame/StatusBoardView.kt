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
 * Reusable status board view. Allows configuring size, background color, text color and dynamic buttons.
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
