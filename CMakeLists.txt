cmake_minimum_required(VERSION 3.16)

project(oh_psp LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection
if(PSP OR DEFINED ENV{PSPDEV} OR CMAKE_SYSTEM_NAME STREQUAL "Generic")
    set(PLATFORM_PSP_DEFAULT ON)
else()
    set(PLATFORM_PSP_DEFAULT OFF)
endif()

option(PLATFORM_PSP "Build for PlayStation Portable" ${PLATFORM_PSP_DEFAULT})
option(BUILD_TESTS "Build host unit tests" ON)

if(PLATFORM_PSP)
    add_compile_options(-G0)
endif()

# SDL2 dependency
include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)

# Core puzzle math libraries (zero SDL/platform dependencies)
add_library(takuzu_core STATIC
    src/games/takuzu/core/takuzu_board.cpp
    src/games/takuzu/core/takuzu_solver.cpp
    src/games/takuzu/core/takuzu_generator.cpp
)
target_include_directories(takuzu_core PUBLIC
    src/games/takuzu/core
)

add_library(vision_core STATIC
    src/games/vision/core/vision_board.cpp
    src/games/vision/core/vision_solver.cpp
    src/games/vision/core/vision_generator.cpp
)
target_include_directories(vision_core PUBLIC
    src/games/vision/core
)

# Engine platform layer (SDL2, input, display, rendering, registry, audio)
add_library(engine STATIC
    src/engine/display.cpp
    src/engine/input.cpp
    src/engine/font.cpp
    src/engine/render_utils.cpp
    src/engine/registry.cpp
    src/engine/audio.cpp
    src/engine/embedded_music.S
)
target_include_directories(engine PUBLIC
    src
    src/engine
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
)
target_link_directories(engine PUBLIC ${SDL2_LIBRARY_DIRS})
target_link_libraries(engine PUBLIC ${SDL2_LIBRARIES})

# Game UI Modules
add_library(takuzu_module STATIC
    src/games/takuzu/ui/takuzu_module.cpp
)
target_include_directories(takuzu_module PUBLIC
    src
    src/games/takuzu/ui
)
target_link_libraries(takuzu_module PUBLIC takuzu_core engine)

add_library(vision_module STATIC
    src/games/vision/ui/vision_module.cpp
)
target_include_directories(vision_module PUBLIC
    src
    src/games/vision/ui
)
target_link_libraries(vision_module PUBLIC vision_core engine)

# Menu layer
add_library(menu STATIC
    src/menu/main_menu.cpp
)
target_include_directories(menu PUBLIC
    src
    src/menu
)
target_link_libraries(menu PUBLIC engine takuzu_module vision_module)

# Main Executable
add_executable(${PROJECT_NAME}
    src/main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    menu
    takuzu_module
    vision_module
    takuzu_core
    vision_core
    engine
    ${SDL2_LIBRARIES}
)
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ${SDL2_INCLUDE_DIRS}
)
target_link_directories(${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARY_DIRS}
)

# Copy assets to binary directory for runtime loading
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(PLATFORM_PSP)
    # PSP EBOOT generation
    create_pbp_file(
        TARGET ${PROJECT_NAME}
        ICON_PATH NULL
        BACKGROUND_PATH NULL
        PREVIEW_PATH NULL
        TITLE "Oh PSP"
        VERSION 01.00
    )
endif()

# Unit tests on host
if(BUILD_TESTS AND NOT PLATFORM_PSP)
    enable_testing()
    add_subdirectory(tests)
endif()
