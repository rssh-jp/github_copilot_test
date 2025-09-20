#include "Shader.h"

#include "android/AndroidOut.h"
#include "utils/Utility.h"
#include "Model.h"

Shader *Shader::loadShader(
        const std::string &vertexSource,
        const std::string &fragmentSource,
        const std::string &positionAttributeName,
        const std::string &uvAttributeName,
        const std::string &projectionMatrixUniformName,
        const std::string &modelMatrixUniformName) {
    Shader *shader = nullptr;

    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexShader) {
        return nullptr;
    }

    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        return nullptr;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);

        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint logLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

            // If we fail to link the shader program, log the result for debugging
            if (logLength) {
                GLchar *log = new GLchar[logLength];
                glGetProgramInfoLog(program, logLength, nullptr, log);
                aout << "Failed to link program with:\n" << log << std::endl;
                delete[] log;
            }

            glDeleteProgram(program);
        } else {
            // シェーダープログラムの属性ロケーションを明示的にバインド
            // これはlayout(location = X)の代わりに使用できる
            glBindAttribLocation(program, 0, positionAttributeName.c_str());
            glBindAttribLocation(program, 1, uvAttributeName.c_str());
            
            // バインド後に再度リンクする必要がある
            glLinkProgram(program);
            glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
            if (linkStatus != GL_TRUE) {
                aout << "Failed to relink program after attribute binding" << std::endl;
                glDeleteProgram(program);
                return nullptr;
            }
            
            // 属性ロケーションを取得 - layout指定があるので固定値になるはず
            GLint positionAttribute = 0; // layout(location = 0)に対応
            GLint uvAttribute = 1;       // layout(location = 1)に対応
            
            // uniform locationを取得
            GLint projectionMatrixUniform = glGetUniformLocation(
                    program,
                    projectionMatrixUniformName.c_str());
            GLint modelMatrixUniform = glGetUniformLocation(
                    program,
                    modelMatrixUniformName.c_str());
                    
            // デバッグ情報を出力
            aout << "Shader attribute/uniform locations:" << std::endl;
            aout << "  " << positionAttributeName << ": " << positionAttribute << std::endl;
            aout << "  " << uvAttributeName << ": " << uvAttribute << std::endl;
            aout << "  " << projectionMatrixUniformName << ": " << projectionMatrixUniform << std::endl;
            aout << "  " << modelMatrixUniformName << ": " << modelMatrixUniform << std::endl;

            // Only create a new shader if all the attributes are found.
            if (positionAttribute != -1
                && uvAttribute != -1
                && projectionMatrixUniform != -1
                && modelMatrixUniform != -1) {

                shader = new Shader(
                        program,
                        positionAttribute,
                        uvAttribute,
                        projectionMatrixUniform,
                        modelMatrixUniform);
            } else {
                glDeleteProgram(program);
            }
        }
    }

    // The shaders are no longer needed once the program is linked. Release their memory.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shader;
}

GLuint Shader::loadShader(GLenum shaderType, const std::string &shaderSource) {
    Utility::assertGlError();
    
    // シェーダタイプのログ出力
    const char* shaderTypeStr = (shaderType == GL_VERTEX_SHADER) ? "VERTEX" : 
                               ((shaderType == GL_FRAGMENT_SHADER) ? "FRAGMENT" : "UNKNOWN");
    aout << "Compiling " << shaderTypeStr << " shader, source:\n" << shaderSource << std::endl;
    
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        auto *shaderRawString = (GLchar *) shaderSource.c_str();
        GLint shaderLength = shaderSource.length();
        glShaderSource(shader, 1, &shaderRawString, &shaderLength);
        glCompileShader(shader);

        GLint shaderCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);

        // If the shader doesn't compile, log the result to the terminal for debugging
        if (!shaderCompiled) {
            GLint infoLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);

            if (infoLength) {
                auto *infoLog = new GLchar[infoLength];
                glGetShaderInfoLog(shader, infoLength, nullptr, infoLog);
                aout << "Failed to compile with:\n" << infoLog << std::endl;
                delete[] infoLog;
            }

            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

void Shader::activate() const {
    glUseProgram(program_);
}

void Shader::deactivate() const {
    glUseProgram(0);
}

void Shader::drawModel(const Model &model) const {
    // OpenGLのエラーをチェック
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        aout << "OpenGL error before drawModel: " << err << std::endl;
    }

    // 位置属性の設定 - layout(location = 0)
    glVertexAttribPointer(
            0, // layout(location = 0)にマッピング
            3, // elements
            GL_FLOAT, // of type float
            GL_FALSE, // don't normalize
            sizeof(Vertex), // stride is Vertex bytes
            model.getVertexData() // pull from the start of the vertex data
    );
    glEnableVertexAttribArray(0);

    // テクスチャ座標属性の設定 - layout(location = 1)
    glVertexAttribPointer(
            1, // layout(location = 1)にマッピング
            2, // elements
            GL_FLOAT, // of type float
            GL_FALSE, // don't normalize
            sizeof(Vertex), // stride is Vertex bytes
            ((uint8_t *) model.getVertexData()) + sizeof(Vector3) // offset Vector3 from the start
    );
    glEnableVertexAttribArray(1);

    // テクスチャの設定
    glActiveTexture(GL_TEXTURE0);
    GLuint textureID = model.getTexture().getTextureID();
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // テクスチャuniformの設定を確認
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    GLint texLoc = glGetUniformLocation(currentProgram, "uTexture");
    if (texLoc != -1) {
        glUniform1i(texLoc, 0); // サンプラーをテクスチャユニット0に関連付け
    }
    
    aout << "Drawing with texture ID: " << textureID << std::endl;

    // Draw as indexed triangles
    glDrawElements(GL_TRIANGLES, model.getIndexCount(), GL_UNSIGNED_SHORT, model.getIndexData());

    glDisableVertexAttribArray(uv_);
    glDisableVertexAttribArray(position_);
}

void Shader::drawModelWithMode(const Model &model, GLenum mode) const {
    // Similar setup to drawModel but allows specifying primitive mode
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        aout << "OpenGL error before drawModelWithMode: " << err << std::endl;
    }

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), model.getVertexData());
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((uint8_t *) model.getVertexData()) + sizeof(Vector3));
    glEnableVertexAttribArray(1);

    glActiveTexture(GL_TEXTURE0);
    GLuint textureID = model.getTexture().getTextureID();
    glBindTexture(GL_TEXTURE_2D, textureID);

    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    GLint texLoc = glGetUniformLocation(currentProgram, "uTexture");
    if (texLoc != -1) {
        glUniform1i(texLoc, 0);
    }

    aout << "Drawing with texture ID (mode): " << textureID << " mode: " << mode << std::endl;

    // If caller requests a line primitive (GL_LINE_LOOP / GL_LINE_STRIP / GL_LINES), draw using
    // the provided index buffer directly. Some callers (like UnitRenderer) prepare models where
    // indices are already ordered for line drawing (e.g. 0..N-1). Converting triangle indices to
    // lines here breaks those models, so prefer direct draw with the requested primitive mode.
    if (mode == GL_LINE_LOOP || mode == GL_LINES || mode == GL_LINE_STRIP) {
        glDrawElements(mode, static_cast<GLsizei>(model.getIndexCount()), GL_UNSIGNED_SHORT, model.getIndexData());
    } else {
        glDrawElements(mode, static_cast<GLsizei>(model.getIndexCount()), GL_UNSIGNED_SHORT, model.getIndexData());
    }

    glDisableVertexAttribArray(uv_);
    glDisableVertexAttribArray(position_);
}

void Shader::setProjectionMatrix(float *projectionMatrix) const {
    glUniformMatrix4fv(projectionMatrix_, 1, false, projectionMatrix);
}

void Shader::setModelMatrix(float *modelMatrix) const {
    glUniformMatrix4fv(modelMatrix_, 1, false, modelMatrix);
}