cmake_minimum_required(VERSION 3.20)

project(wgfx LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile commands for editor tooling" FORCE)

find_package(Vulkan REQUIRED)

if(NOT Vulkan_GLSLC_EXECUTABLE)
    message(FATAL_ERROR "The Vulkan SDK was found, but glslc is unavailable.")
endif()

set(VULKAN_SHADER_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/vulkan")
file(GLOB_RECURSE VULKAN_SHADER_SOURCES CONFIGURE_DEPENDS
    "${VULKAN_SHADER_DIRECTORY}/*.vert"
    "${VULKAN_SHADER_DIRECTORY}/*.vert.glsl"
    "${VULKAN_SHADER_DIRECTORY}/*.frag"
    "${VULKAN_SHADER_DIRECTORY}/*.frag.glsl"
    "${VULKAN_SHADER_DIRECTORY}/*.comp"
    "${VULKAN_SHADER_DIRECTORY}/*.comp.glsl"
    "${VULKAN_SHADER_DIRECTORY}/*.geom"
    "${VULKAN_SHADER_DIRECTORY}/*.geom.glsl"
    "${VULKAN_SHADER_DIRECTORY}/*.tesc"
    "${VULKAN_SHADER_DIRECTORY}/*.tesc.glsl"
    "${VULKAN_SHADER_DIRECTORY}/*.tese"
    "${VULKAN_SHADER_DIRECTORY}/*.tese.glsl"
)

set(VULKAN_SHADER_BINARIES)
foreach(SHADER_SOURCE IN LISTS VULKAN_SHADER_SOURCES)
    get_filename_component(SHADER_FILENAME "${SHADER_SOURCE}" NAME)
    string(REGEX REPLACE "\\.glsl$" "" SHADER_OUTPUT_NAME "${SHADER_FILENAME}")
    set(SHADER_BINARY "${VULKAN_SHADER_DIRECTORY}/${SHADER_OUTPUT_NAME}.spv")

    if(SHADER_FILENAME MATCHES "\\.vert(\\.glsl)?$")
        set(SHADER_STAGE vertex)
    elseif(SHADER_FILENAME MATCHES "\\.frag(\\.glsl)?$")
        set(SHADER_STAGE fragment)
    elseif(SHADER_FILENAME MATCHES "\\.comp(\\.glsl)?$")
        set(SHADER_STAGE compute)
    elseif(SHADER_FILENAME MATCHES "\\.geom(\\.glsl)?$")
        set(SHADER_STAGE geometry)
    elseif(SHADER_FILENAME MATCHES "\\.tesc(\\.glsl)?$")
        set(SHADER_STAGE tesscontrol)
    elseif(SHADER_FILENAME MATCHES "\\.tese(\\.glsl)?$")
        set(SHADER_STAGE tesseval)
    else()
        message(FATAL_ERROR "Cannot determine Vulkan shader stage for ${SHADER_FILENAME}")
    endif()

    add_custom_command(
        OUTPUT "${SHADER_BINARY}"
        COMMAND "${Vulkan_GLSLC_EXECUTABLE}"
                --target-env=vulkan1.3
                -fshader-stage=${SHADER_STAGE}
                "${SHADER_SOURCE}"
                -o "${SHADER_BINARY}"
        DEPENDS "${SHADER_SOURCE}"
        COMMENT "Compiling ${SHADER_FILENAME} to SPIR-V"
        VERBATIM
    )
    list(APPEND VULKAN_SHADER_BINARIES "${SHADER_BINARY}")
endforeach()

add_custom_target(vulkan_shaders ALL DEPENDS ${VULKAN_SHADER_BINARIES})

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

add_library(imgui STATIC
    deps/imgui/imgui.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_widgets.cpp
    deps/imgui/backends/imgui_impl_glfw.cpp
    deps/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    deps/imgui
    deps/imgui/backends
)
target_link_libraries(imgui PUBLIC glfw)
target_compile_features(imgui PUBLIC cxx_std_17)

add_executable(wgfx
    main.cpp
    src/shader.cpp
    include/shader.h
    src/camera.cpp
    include/camera.h
    src/input.cpp
    include/input.h
    src/buffer.cpp
    include/buffer.h
    src/model.cpp
    include/model.h
    src/texture.cpp
    include/texture.h
    src/skybox.cpp
    include/skybox.h
    src/window.cpp
    include/window.h
    src/directional_shadow_map.cpp
    include/directional_shadow_map.h
    src/lighting.cpp
    include/lighting.h
    src/spot_shadow_map.cpp
    include/spot_shadow_map.h
    src/point_shadow_map.cpp
    include/point_shadow_map.h
    src/renderer.cpp
    include/renderer.h
    src/imgui_layer.cpp
    include/imgui_layer.h
)
add_dependencies(wgfx vulkan_shaders)
target_compile_features(wgfx PRIVATE cxx_std_20)
target_include_directories(wgfx PRIVATE include)
target_compile_definitions(wgfx PRIVATE
    SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/"
    VULKAN_SHADER_DIR="${VULKAN_SHADER_DIRECTORY}/"
    MODEL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res/models/"
    CUBEMAP_DIR="${CMAKE_CURRENT_SOURCE_DIR}/res/cubemaps/"
)
target_link_libraries(wgfx PRIVATE
    glad
    glfw
    imgui
    glm::glm
    tinyobjloader
    stb_image
    fastgltf::fastgltf
    Vulkan::Vulkan
    opengl32
    gdi32
    user32
    shell32
    winmm
)
