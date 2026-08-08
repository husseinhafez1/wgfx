cmake_minimum_required(VERSION 3.20)

project(wgfx LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile commands for editor tooling" FORCE)

add_library(glad STATIC deps/glad/src/glad.c)
target_include_directories(glad PUBLIC deps/glad/include)

add_library(glm INTERFACE)
add_library(glm::glm ALIAS glm)
target_include_directories(glm INTERFACE deps/glm)

add_library(tinyobjloader STATIC deps/tinyobjloader/tiny_obj_loader.cc)
target_include_directories(tinyobjloader PUBLIC deps/tinyobjloader)

add_library(stb_image STATIC deps/stb/stb_image.cpp)
target_include_directories(stb_image PUBLIC deps/stb)

set(FASTGLTF_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/fastgltf EXCLUDE_FROM_ALL)

add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw/lib-vc2022/glfw3.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw/include"
)

add_executable(wgfx
    src/main.cpp
    src/shader.cpp
    src/shader.h
    src/camera.cpp
    src/camera.h
    src/input.cpp
    src/input.h
    src/buffer.cpp
    src/buffer.h
    src/model.cpp
    src/model.h
    src/texture.cpp
    src/texture.h
    src/skybox.cpp
    src/skybox.h
    src/window.cpp
    src/window.h
    src/shadow_map.cpp
    src/shadow_map.h
)
target_compile_features(wgfx PRIVATE cxx_std_17)
target_include_directories(wgfx PRIVATE src)
target_compile_definitions(wgfx PRIVATE
    SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/"
    MODEL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res/models/"
    CUBEMAP_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res/cubemaps/"
)
target_link_libraries(wgfx PRIVATE glad glfw glm::glm tinyobjloader stb_image fastgltf::fastgltf opengl32 gdi32 user32 shell32 winmm)
