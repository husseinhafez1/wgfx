cmake_minimum_required(VERSION 3.20)

project(wgfx LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile commands for editor tooling" FORCE)

add_library(glad STATIC deps/glad/src/glad.c)
target_include_directories(glad PUBLIC deps/glad/include)

add_library(glm INTERFACE)
add_library(glm::glm ALIAS glm)
target_include_directories(glm INTERFACE deps/glm)

add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw/lib-vc2022/glfw3.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw/include"
)

add_executable(wgfx
    src/main.cpp
    src/shader.cpp
    src/shader.h
)
target_compile_features(wgfx PRIVATE cxx_std_17)
target_include_directories(wgfx PRIVATE src)
target_compile_definitions(wgfx PRIVATE
    SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/"
)
target_link_libraries(wgfx PRIVATE glad glfw glm::glm opengl32 gdi32 user32 shell32 winmm)
