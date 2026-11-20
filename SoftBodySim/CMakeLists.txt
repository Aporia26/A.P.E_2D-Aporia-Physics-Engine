cmake_minimum_required(VERSION 3.20)

# 1. CUSTOM EXECUTABLE NAME (Change this anytime to rename your .exe)
set(APP_NAME "SoftBody")

project(${APP_NAME} VERSION 1.0.0 LANGUAGES CXX)

# 2. COMPILER & VS CODE INTELLISENSE SETTINGS
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generates compile_commands.json so VS Code IntelliSense never complains about headers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fast optimization flags (Debug vs Release)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
endif()

# 3. AUTOMATICALLY TRACK ALL SOURCE & HEADER FILES IN src/
# CONFIGURE_DEPENDS makes CMake automatically detect new files/folders without manual reconfiguration
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/AML/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/physics/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/*.hpp"
)
# 4. WINDOWS .EXE ICON SUPPORT (Explicit windres setup for MSYS2 / MinGW)
set(APP_ICON_RESOURCE "")
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/res/app.rc")
    enable_language(RC)
    set(CMAKE_RC_COMPILER_INIT windres)
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -i <SOURCE> -o <OBJECT>")
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/res/app.rc")
endif()


# 5. CREATE EXECUTABLE & PLACE IT DIRECTLY IN MAIN WORKSPACE ROOT
add_executable(${APP_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${APP_ICON_RESOURCE})

set_target_properties(${APP_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# 6. CLEAN INCLUDE PATHS (Works for both #include "AML/AML.hpp" and #include "AML.hpp")
target_include_directories(${APP_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/AML"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# Pass workspace root path as a C++ macro just in case you ever launch from a shortcut
target_compile_definitions(${APP_NAME} PRIVATE
    WORKSPACE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/"
)

# 7. FIND & LINK SFML (Works automatically with MSYS2 UCRT64 SFML 2.6 or SFML 3.0)
set(SFML_STATIC_LIBRARIES TRUE)
if(MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(${APP_NAME} PRIVATE -static -static-libgcc -static-libstdc++)
endif()

find_package(SFML 3 COMPONENTS Graphics Window System Audio Network QUIET)
if(SFML_FOUND)
    target_link_libraries(${APP_NAME} PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
    )
else()
    find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)
    target_link_libraries(${APP_NAME} PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
    )
endif()

# 8. IMGUI-SFML PROVISION (Toggle ON whenever you add ImGui-SFML later)
option(USE_IMGUI_SFML "Enable ImGui-SFML support" OFF)
if(USE_IMGUI_SFML)
    find_package(ImGui-SFML REQUIRED)
    target_link_libraries(${APP_NAME} PRIVATE ImGui-SFML::ImGui-SFML)
    target_compile_definitions(${APP_NAME} PRIVATE ENABLE_IMGUI)
endif()
