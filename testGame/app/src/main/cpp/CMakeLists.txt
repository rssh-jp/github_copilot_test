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
)

# Source files organized by layer
set(DOMAIN_SOURCES
    # Value Objects (header-only, no .cpp files needed)
    
    # Entities (header-only for now, can add .cpp if needed)
)

set(USECASE_SOURCES
    # Usecases (header-only for now)
)

set(LEGACY_SOURCES
    main.cpp
    AndroidOut.cpp
    Renderer.cpp
    Shader.cpp
    TextureAsset.cpp
    Utility.cpp
    Unit.cpp
    UnitRenderer.cpp
    UnitStatusJNI.cpp
)

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(testgame SHARED
        ${LEGACY_SOURCES}
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