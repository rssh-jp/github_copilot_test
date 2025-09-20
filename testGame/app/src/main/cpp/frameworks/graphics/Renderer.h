#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include <EGL/egl.h>
#include <memory>

#include "Model.h"
#include "Shader.h"
#include "entities/UnitEntity.h"
#include "UnitRenderer.h"
#include "../../usecases/CombatUseCase.h"
#include "../../usecases/MovementUseCase.h"
// MovementField is used by Renderer as a concrete type for the movement field instance
#include "../../domain/services/MovementField.h"

struct android_app;

class Renderer {
public:
    /*!
     * @param pApp the android_app this Renderer belongs to, needed to configure GL
     */
    inline Renderer(android_app *pApp) :
            app_(pApp),
            display_(EGL_NO_DISPLAY),
            surface_(EGL_NO_SURFACE),
            context_(EGL_NO_CONTEXT),
            width_(0),
            height_(0),
            shaderNeedsNewProjectionMatrix_(true) {
        initRenderer();
    }

    virtual ~Renderer();

    /*!
     * Handles input from the android_app.
     *
     * Note: this will clear the input queue
     */
    void handleInput();

    /*!
     * Renders all the models in the renderer
     */
    void render();

    /*!
     * UnitRendererへの参照を取得する
     * 
     * @return UnitRendererへのポインタ
     */
    UnitRenderer* getUnitRenderer() const;

private:
    /*!
     * Performs necessary OpenGL initialization. Customize this if you want to change your EGL
     * context or application-wide settings.
     */
    void initRenderer();

    /*!
     * @brief we have to check every frame to see if the framebuffer has changed in size. If it has,
     * update the viewport accordingly
     */
    void updateRenderArea();

    /*!
     * Creates the models for this sample. You'd likely load a scene configuration from a file or
     * use some other setup logic in your full game.
     */
    void createModels();
    
    /*!
     * 画面座標をゲーム内座標に変換する
     * 
     * @param screenX 画面X座標（ピクセル）
     * @param screenY 画面Y座標（ピクセル）
     * @param worldX 出力：ゲーム内X座標
     * @param worldY 出力：ゲーム内Y座標
     */
    void screenToWorldCoordinates(float screenX, float screenY, float& worldX, float& worldY) const;
    
    /*!
     * ユニットをタッチした位置に移動する処理
     * 
     * @param x ゲーム内X座標
     * @param y ゲーム内Y座標
     */
    void moveUnitToPosition(float x, float y);

private:
    android_app *app_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    bool shaderNeedsNewProjectionMatrix_;

    std::unique_ptr<Shader> shader_;
    std::vector<Model> models_;
    
    // ユニット管理
    std::unique_ptr<UnitRenderer> unitRenderer_;
    std::vector<std::shared_ptr<UnitEntity>> units_;
    
    // ユースケース
    std::unique_ptr<CombatUseCase> combatUseCase_;
    std::unique_ptr<MovementUseCase> movementUseCase_;
    // Movement field for walkability and obstacles
    std::unique_ptr<class MovementField> movementField_;
};

#endif //ANDROIDGLINVESTIGATIONS_RENDERER_H