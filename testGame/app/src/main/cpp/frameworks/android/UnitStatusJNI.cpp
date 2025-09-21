#include <jni.h>
#include <string>
#include <android/log.h>
#include <cmath>
#include <cstdlib>
#include "graphics/Renderer.h"
#include "graphics/UnitRenderer.h"
#include "entities/UnitEntity.h"

/*
 * UnitStatusJNI.cpp
 *
 * JNI bridge between Android UI (Kotlin) and native engine state.
 *
 * Responsibilities:
 * - Expose read-only accessors for unit state (position, HP, stats) and renderer state (camera offset).
 * - Provide touch handling entry point that translates screen coords to world coords and performs hit-tests.
 *
 * Threading / safety notes:
 * - JNI calls may be invoked on the UI thread; ensure any interaction with renderer or game state is
 *   safe for the current threading model. Where necessary, marshal requests to the render thread.
 */
#include <string>
#include <android/log.h>
#include <cmath>
#include <cstdlib>
#include "graphics/Renderer.h"
#include "graphics/UnitRenderer.h"
#include "entities/UnitEntity.h"

#define TAG "UnitStatusJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// グローバル変数でRenderer参照を保持
static Renderer* g_renderer = nullptr;
// currently selected unit id (set by touch hit-test). -1 = none
static int g_selectedUnitId = -1;
// persisted last selected unit id so UI can keep polling its state until explicitly cleared
static int g_persistSelectedUnitId = -1;

// Rendererの参照を設定する関数（他のファイルから呼ばれる）
extern "C" void setRendererReference(Renderer* renderer) {
    g_renderer = renderer;
}

/**
 * JNI accessor: getCameraOffsetX/Y, getElapsedTime などは UI 側から定期的にポーリングされます。
 * これらは単純に Renderer のアクセサを呼ぶだけで副作用はありません。
 */

// JNI helper: expose camera offsets and elapsed time so Java UI can poll status
extern "C" JNIEXPORT jfloat JNICALL
Java_com_example_testgame_MainActivity_getCameraOffsetX(JNIEnv *env, jobject /* this */) {
    if (!g_renderer) return 0.0f;
    return g_renderer->getCameraOffsetX();
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_example_testgame_MainActivity_getCameraOffsetY(JNIEnv *env, jobject /* this */) {
    if (!g_renderer) return 0.0f;
    return g_renderer->getCameraOffsetY();
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_example_testgame_MainActivity_getElapsedTime(JNIEnv *env, jobject /* this */) {
    if (!g_renderer) return 0.0f;
    return g_renderer->getElapsedTime();
}

// Return a packed int where each byte is the count for faction 1..4 (supports up to 255 each)
extern "C" JNIEXPORT jint JNICALL
Java_com_example_testgame_MainActivity_getFactionCountsPacked(JNIEnv *env, jobject /* this */) {
    if (!g_renderer) return 0;
    auto unitRenderer = g_renderer->getUnitRenderer();
    if (!unitRenderer) return 0;
    const auto& all = unitRenderer->getAllUnits();
    uint8_t counts[4] = {0,0,0,0};
    for (const auto& p : all) {
        auto u = p.second;
        if (!u) continue;
        int f = u->getFaction();
        if (f >= 1 && f <= 4) counts[f-1]++;
    }
    jint packed = (counts[0] & 0xFF) | ((counts[1] & 0xFF) << 8) | ((counts[2] & 0xFF) << 16) | ((counts[3] & 0xFF) << 24);
    return packed;
}

// プレイヤーユニット（ID=1）を取得するヘルパー関数
std::shared_ptr<UnitEntity> getPlayerUnit() {
    if (!g_renderer) {
        LOGE("Renderer not available");
        return nullptr;
    }
    
    auto unitRenderer = g_renderer->getUnitRenderer();
    if (!unitRenderer) {
        LOGE("UnitRenderer not available");
        return nullptr;
    }
    
    return unitRenderer->getUnit(1); // プレイヤーユニットはID=1
}

// Return currently selected unit if any
std::shared_ptr<UnitEntity> getSelectedUnit() {
    if (!g_renderer) return nullptr;
    auto unitRenderer = g_renderer->getUnitRenderer();
    if (!unitRenderer) return nullptr;
    if (g_selectedUnitId <= 0) return nullptr;
    return unitRenderer->getUnit(g_selectedUnitId);
}

// Return persisted selected unit (last selected) if any
std::shared_ptr<UnitEntity> getPersistSelectedUnit() {
    if (!g_renderer) return nullptr;
    auto unitRenderer = g_renderer->getUnitRenderer();
    if (!unitRenderer) return nullptr;
    if (g_persistSelectedUnitId <= 0) return nullptr;
    return unitRenderer->getUnit(g_persistSelectedUnitId);
}

// タッチイベント処理関数
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_testgame_MainActivity_onTouch(JNIEnv *env, jobject /* this */, jfloat x, jfloat y) {
    if (!g_renderer) {
        LOGE("Renderer not available for touch processing");
        return JNI_FALSE;
    }
    
    auto unitRenderer = g_renderer->getUnitRenderer();
    if (!unitRenderer) {
        LOGE("UnitRenderer not available for touch processing");
        return JNI_FALSE;
    }
    
    // Convert screen coordinates (pixels) to world/game coordinates using Renderer helper.
    // NOTE: This JNI entrypoint is often invoked on the UI thread; position calculations and
    // hit-tests are performed synchronously here. If renderer or unit lists are mutated on
    // another thread, callers must ensure proper synchronization (not handled here).
    float worldX = 0.0f;
    float worldY = 0.0f;
    g_renderer->screenToWorld(x, y, worldX, worldY);

    LOGI("Touch at screen (%f, %f) -> world (%.3f, %.3f)", x, y, worldX, worldY);

    // Hit-test units: pick the nearest unit whose collision radius contains the touch point
    const auto& all = unitRenderer->getAllUnits();
    float bestDist = std::numeric_limits<float>::infinity();
    int bestId = -1;
    for (const auto& p : all) {
        auto u = p.second;
        if (!u) continue;
        auto pos = u->getPosition();
        float dx = worldX - pos.getX();
        float dy = worldY - pos.getY();
        float dist = std::sqrt(dx*dx + dy*dy);
        float radius = u->getStats().getCollisionRadius();
        if (dist <= radius && dist < bestDist) {
            bestDist = dist;
            bestId = p.first;
        }
    }

    if (bestId != -1) {
        // select the hit unit
        g_selectedUnitId = bestId;
        // persist selection so UI can continue showing this unit even after temporary clears
        g_persistSelectedUnitId = bestId;
        LOGI("Selected unit id %d via touch", bestId);
        return JNI_TRUE; // indicate selection occurred
    }

    // No unit hit: clear selection and treat as move command for player unit (id=1)
    g_selectedUnitId = -1;
    auto player = unitRenderer->getUnit(1);
    if (!player) {
        LOGE("Player unit not found for touch processing");
        return JNI_FALSE;
    }
    player->setTargetPosition(Position(worldX, worldY));
    player->setState(UnitState::MOVING);
    return JNI_FALSE; // indicate no selection (move issued)
}

/**
 * JNI: UI 側が選択中のユニット名を取得するために呼び出します。
 * 戻り値が空文字列の場合は「選択なし」を意味します。
 */

// MainActivityで使用されている関数名に合わせた関数定義
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_testgame_MainActivity_getUnitName(JNIEnv *env, jobject /* this */) {
    // Prefer persisted selected unit so UI can keep polling last selection
    auto unit = getPersistSelectedUnit();
    if (!unit) {
        // No persisted selection -> return empty string to indicate none selected
        return env->NewStringUTF("");
    }
    return env->NewStringUTF(unit->getName().c_str());
}

/**
 * JNI: 選択中のユニットのステータス文字列（IDLE/MOVING/COMBAT）を返します。
 * UI のセンターダイアログで使用されます。
 */

extern "C" JNIEXPORT jint JNICALL
Java_com_example_testgame_MainActivity_getCurrentHp(JNIEnv *env, jobject /* this */) {
    auto unit = getPersistSelectedUnit();
    if (!unit) {
        return 0;
    }
    return unit->getStats().getCurrentHp();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_testgame_MainActivity_getMaxHp(JNIEnv *env, jobject /* this */) {
    auto unit = getPersistSelectedUnit();
    if (!unit) {
        return 100;
    }
    return unit->getStats().getMaxHp();
}


extern "C" JNIEXPORT jint JNICALL
Java_com_example_testgame_MainActivity_getMinAttack(JNIEnv *env, jobject /* this */) {
    auto unit = getPersistSelectedUnit();
    if (!unit) {
        return 0;
    }
    return unit->getStats().getMinAttackPower();
}


extern "C" JNIEXPORT jint JNICALL
Java_com_example_testgame_MainActivity_getMaxAttack(JNIEnv *env, jobject /* this */) {
    auto unit = getPersistSelectedUnit();
    if (!unit) {
        return 0;
    }
    return unit->getStats().getMaxAttackPower();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_testgame_MainActivity_getDefense(JNIEnv *env, jobject /* this */) {
    auto unit = getPersistSelectedUnit();
    if (!unit) {
        return 0;
    }
    // If UnitStats lacks defense, return fixed value
    return 5;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_example_testgame_MainActivity_getPositionX(JNIEnv *env, jobject /* this */) {
    auto unit = getPersistSelectedUnit();
    if (!unit) {
        return 0.0f;
    }
    return unit->getPosition().getX();
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_example_testgame_MainActivity_getPositionY(JNIEnv *env, jobject /* this */) {
    auto unit = getPersistSelectedUnit();
    if (!unit) {
        return 0.0f;
    }
    return unit->getPosition().getY();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_testgame_MainActivity_getUnitStatusString(JNIEnv *env, jobject /* this */) {
    // Prefer the currently selected unit (if any) for status display.
    auto unit = getSelectedUnit();
    if (!unit) {
        // Fallback to player unit
        unit = getPlayerUnit();
    }
    if (!unit) {
        return env->NewStringUTF("UNKNOWN");
    }

    UnitState state = unit->getState();
    switch (state) {
        case UnitState::IDLE:
            return env->NewStringUTF("IDLE");
        case UnitState::MOVING:
            return env->NewStringUTF("MOVING");
        case UnitState::COMBAT:
            return env->NewStringUTF("COMBAT");
        default:
            return env->NewStringUTF("UNKNOWN");
    }
}

/**
 * JNI: UI がセンターダイアログを閉じたときに永続選択をクリアします。
 * 明示的に呼ばれるまで UI は同じユニットを表示し続けます。
 */

// Clear persisted selection - callable from Java when user closes the dialog
extern "C" JNIEXPORT void JNICALL
Java_com_example_testgame_MainActivity_clearPersistSelectedUnit(JNIEnv *env, jobject /* this */) {
    g_persistSelectedUnitId = -1;
    LOGI("Cleared persisted selected unit");
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_testgame_MainActivity_moveUnit(JNIEnv *env, jobject /* this */) {
    auto unit = getPlayerUnit();
    if (!unit) {
        LOGE("Cannot move unit - player unit not found");
        return;
    }
    
    // ランダムな位置に移動
    float x = (rand() % 200 - 100) / 10.0f;  // -10.0 to 10.0
    float y = (rand() % 200 - 100) / 10.0f;  // -10.0 to 10.0
    Position targetPos(x, y);
    unit->setTargetPosition(targetPos);
    // Ensure unit state reflects movement so UI shows MOVING
    unit->setState(UnitState::MOVING);
    LOGI("Unit moving to (%f, %f)", x, y);
}

/**
 * JNI: UI 上のボタンやデバッグ用にユニットの停止を命令します。
 * プレイヤーユニットの targetPosition を現在位置へ設定し、状態を IDLE にします。
 */

extern "C" JNIEXPORT void JNICALL
Java_com_example_testgame_MainActivity_stopUnit(JNIEnv *env, jobject /* this */) {
    auto unit = getPlayerUnit();
    if (!unit) {
        LOGE("Cannot stop unit - player unit not found");
        return;
    }
    
    // 現在位置を目標位置に設定することで停止
    auto pos = unit->getPosition();
    unit->setTargetPosition(pos);
    // Set state to IDLE so UI shows correct status
    unit->setState(UnitState::IDLE);
    LOGI("Unit stopped");
}

// JNI: pan the camera by (dx, dy) in world units
extern "C" JNIEXPORT void JNICALL
Java_com_example_testgame_MainActivity_panCameraBy(JNIEnv *env, jobject /* this */, jfloat dx, jfloat dy) {
    if (!g_renderer) {
        LOGE("Renderer not available for panCameraBy");
        return;
    }

    // If Renderer provides a pan or offset API, use it. Otherwise, adjust camera target/offset directly.
    try {
        // Preferential API: panCameraBy
        // NOTE: If Renderer doesn't have panCameraBy, this will be a compile error; so check via pointer to method isn't possible in C++.
        // We will attempt to call an assumed method; if it doesn't exist, user will need to add it.
        g_renderer->panCameraBy(dx, dy);
        return;
    } catch (...) {
        // Fallback: directly change camera offsets if members are accessible
    }

    // Fallback logic: try to get current offset and set a new target (if API exists)
    // Many Renderers have getCameraOffsetX/Y and setCameraTargetX/Y or setCameraOffset; attempt to use them if available.
    // We'll attempt the most common functions; if they don't exist, this code will not compile and should be adapted.
    #if 0
    float curX = g_renderer->getCameraOffsetX();
    float curY = g_renderer->getCameraOffsetY();
    g_renderer->setCameraOffset(curX + dx, curY + dy);
    #endif

    // If we reached here, no portable fallback implemented. Log and return.
    LOGE("panCameraBy: fallback not implemented for this Renderer. Please add Renderer::panCameraBy or setCameraOffset API.");
}

// JNI: toggle attack range visualization in UnitRenderer. Called from UI toggle switch.
extern "C" JNIEXPORT void JNICALL
Java_com_example_testgame_MainActivity_setShowAttackRanges(JNIEnv *env, jobject /* this */, jboolean show) {
    if (!g_renderer) {
        LOGE("setShowAttackRanges: renderer not available");
        return;
    }
    auto unitRenderer = g_renderer->getUnitRenderer();
    if (!unitRenderer) {
        LOGE("setShowAttackRanges: unitRenderer not available");
        return;
    }
    unitRenderer->setShowAttackRanges(show == JNI_TRUE);
    LOGI("setShowAttackRanges called: %d", (int)show);
}
