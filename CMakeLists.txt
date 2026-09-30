cmake_minimum_required(VERSION 3.20)
project(VoxelEngine CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ── GLFW (windowing & input) ──
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ── GLM (math library) ──
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

# ── stb_image (texture loading) ──
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

# ── FastNoise Lite (terrain generation) ──
FetchContent_Declare(
    fastnoise
    GIT_REPOSITORY https://github.com/Auburn/FastNoiseLite.git
    GIT_TAG        v1.1.1
)
FetchContent_MakeAvailable(fastnoise)

# ── Source files ──
set(ENGINE_SOURCES
    src/main.cpp
    src/glad.c
    src/window.cpp
    src/shader.cpp
    src/camera.cpp
    src/chunk.cpp
    src/world.cpp
    src/renderer.cpp
    src/input.cpp
    src/player.cpp
    src/ui.cpp
)

add_executable(${PROJECT_NAME} ${ENGINE_SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_INCLUDE_NONE)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    ${stb_SOURCE_DIR}
    ${fastnoise_SOURCE_DIR}/Cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glm::glm
)

# ── Link platform OpenGL ──
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework OpenGL")
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
endif()

# ── Copy shaders to build directory ──
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)