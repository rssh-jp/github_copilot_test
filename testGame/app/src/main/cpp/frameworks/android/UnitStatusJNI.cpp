#include <jni.h>
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

// Rendererの参照を設定する関数（他のファイルから呼ばれる）
extern "C" void setRendererReference(Renderer* renderer) {
    g_renderer = renderer;
}

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
    
    auto unit = unitRenderer->getUnit(1); // プレイヤーユニット
    if (!unit) {
        LOGE("Player unit not found for touch processing");
        return JNI_FALSE;
    }
    
    // Convert screen coordinates (pixels) to world/game coordinates using Renderer helper
    float worldX = 0.0f;
    float worldY = 0.0f;
    g_renderer->screenToWorld(x, y, worldX, worldY);

    LOGI("Touch at screen (%f, %f) -> world (%.3f, %.3f)", x, y, worldX, worldY);

    // Set player's target position to the converted world coordinates
    unit->setTargetPosition(Position(worldX, worldY));
    unit->setState(UnitState::MOVING);
    return JNI_TRUE;
}

// MainActivityで使用されている関数名に合わせた関数定義
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_testgame_MainActivity_getUnitName(JNIEnv *env, jobject /* this */) {
    auto unit = getPlayerUnit();
    if (!unit) {
        return env->NewStringUTF("Unknown");
    }
    return env->NewStringUTF(unit->getName().c_str());
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_testgame_MainActivity_getCurrentHp(JNIEnv *env, jobject /* this */) {
    auto unit = getPlayerUnit();
    if (!unit) {
        return 0;
    }
    return unit->getStats().getCurrentHp();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_testgame_MainActivity_getMaxHp(JNIEnv *env, jobject /* this */) {
    auto unit = getPlayerUnit();
    if (!unit) {
        return 100;
    }
    return unit->getStats().getMaxHp();
}


extern "C" JNIEXPORT jint JNICALL
Java_com_example_testgame_MainActivity_getMinAttack(JNIEnv *env, jobject /* this */) {
    auto unit = getPlayerUnit();
    if (!unit) {
        return 0;
    }
    return unit->getStats().getMinAttackPower();
}


extern "C" JNIEXPORT jint JNICALL
Java_com_example_testgame_MainActivity_getMaxAttack(JNIEnv *env, jobject /* this */) {
    auto unit = getPlayerUnit();
    if (!unit) {
        return 0;
    }
    return unit->getStats().getMaxAttackPower();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_testgame_MainActivity_getDefense(JNIEnv *env, jobject /* this */) {
    auto unit = getPlayerUnit();
    if (!unit) {
        return 0;
    }
    // UnitStatsに防御力がないため、固定値を返す（将来的にUnitStatsに追加予定）
    return 5;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_example_testgame_MainActivity_getPositionX(JNIEnv *env, jobject /* this */) {
    auto unit = getPlayerUnit();
    if (!unit) {
        return 0.0f;
    }
    return unit->getPosition().getX();
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_example_testgame_MainActivity_getPositionY(JNIEnv *env, jobject /* this */) {
    auto unit = getPlayerUnit();
    if (!unit) {
        return 0.0f;
    }
    return unit->getPosition().getY();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_testgame_MainActivity_getUnitStatusString(JNIEnv *env, jobject /* this */) {
    auto unit = getPlayerUnit();
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
    LOGI("Unit moving to (%f, %f)", x, y);
}

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
