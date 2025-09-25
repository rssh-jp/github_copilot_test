# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("testgame")

# C++17 standard for modern features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories for clean architecture
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/domain
    ${CMAKE_CURRENT_SOURCE_DIR}/usecases
    ${CMAKE_CURRENT_SOURCE_DIR}/adapters
    ${CMAKE_CURRENT_SOURCE_DIR}/frameworks
    ${CMAKE_CURRENT_SOURCE_DIR}/frameworks/android
    ${CMAKE_CURRENT_SOURCE_DIR}/frameworks/graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/frameworks/input
    ${CMAKE_CURRENT_SOURCE_DIR}/frameworks/utils
)

# Source files organized by layer
set(DOMAIN_SOURCES
    # Value Objects (header-only, no .cpp files needed)
    
    # Entities (header-only for now, can add .cpp if needed)
    
    # Domain Services
    domain/services/CombatDomainService.cpp
    domain/services/CollisionDomainService.cpp
)

set(USECASE_SOURCES
    # Usecases
    usecases/CombatUseCase.cpp
    usecases/MovementUseCase.cpp
    usecases/CameraControlUseCase.cpp
)

set(FRAMEWORK_SOURCES
    frameworks/android/AndroidOut.cpp
    frameworks/android/UnitStatusJNI.cpp
    frameworks/android/TouchInputHandler.cpp
    frameworks/graphics/Renderer.cpp
    frameworks/graphics/Shader.cpp
    frameworks/graphics/TextureAsset.cpp
    frameworks/graphics/UnitRenderer.cpp
    frameworks/utils/Utility.cpp
)

set(MAIN_SOURCES
    main.cpp
)

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(testgame SHARED
        ${FRAMEWORK_SOURCES}
        ${MAIN_SOURCES}
        ${DOMAIN_SOURCES}
        ${USECASE_SOURCES}
)

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)

# Configure libraries CMake uses to link your target library.
target_link_libraries(testgame
        # The game activity
        game-activity::game-activity

        # EGL and other dependent libraries required for drawing
        # and interacting with Android system
        EGL
        GLESv3
        jnigraphics
        android
        log)