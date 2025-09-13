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
    
    // 簡単なタッチ判定 - ゲーム座標系での判定
    // スクリーン座標からゲーム座標への変換が必要だが、
    // 今回は簡単にスクリーン中央付近のタッチでユニットが選択されたとする
    float screenCenterX = 540.0f; // 仮のスクリーン中央X（1080pの半分）
    float screenCenterY = 960.0f; // 仮のスクリーン中央Y（1920pの半分）
    
    float dx = x - screenCenterX;
    float dy = y - screenCenterY;
    float distance = sqrt(dx * dx + dy * dy);
    
    // スクリーン中央から一定範囲内のタッチでユニットが選択されたとする
    if (distance < 200.0f) {
        LOGI("Unit touched at screen (%f, %f), distance from center: %f", x, y, distance);
        return JNI_TRUE;
    }
    
    LOGI("Touch outside unit range at (%f, %f), distance: %f", x, y, distance);
    return JNI_FALSE;
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
    return unit->getStats().getAttackPower();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_testgame_MainActivity_getMaxAttack(JNIEnv *env, jobject /* this */) {
    auto unit = getPlayerUnit();
    if (!unit) {
        return 0;
    }
    return unit->getStats().getAttackPower();
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
