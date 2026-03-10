cmake_minimum_required(VERSION 3.14)
project(COD_FPS LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch Raylib
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

# Source files
set(SOURCES
    src/main.cpp
    src/player.cpp
    src/weapon.cpp
    src/enemy.cpp
    src/level.cpp
    src/hud.cpp
    src/physics.cpp
    src/particles.cpp
    src/audio_manager.cpp
    src/python_bridge.cpp
    src/grenade.cpp
    src/wave_system.cpp
    src/killstreak.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Python disabled to avoid runtime DLL dependency
message(STATUS "Building without Python scripting (disabled)")
target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_PYTHON=0)

# Copy scripts folder to build directory
file(COPY ${CMAKE_SOURCE_DIR}/scripts DESTINATION ${CMAKE_BINARY_DIR})

# Set working directory for debugging
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm)
endif()
