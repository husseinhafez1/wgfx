// #include <renderer.h>
#include <vulkan/vulkan.hpp>
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/resource_limits_c.h>
#include <taskflow/taskflow.hpp>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <filesystem>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

GLFWwindow* initWindow(const char* title, uint32_t& width, uint32_t& height) {
    glfwSetErrorCallback([](int error, const char* description){
        printf("GLFW Error (%i): %s\n", error, description);
    });
    if (!glfwInit()) return nullptr;
    const bool wantsWholeArea = !width || !height;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, wantsWholeArea ? GLFW_FALSE : GLFW_TRUE);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor == nullptr) {
        glfwTerminate();
        return nullptr;
    }
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (mode == nullptr) {
        glfwTerminate();
        return nullptr;
    }
    int x(0), y(0);
    int w = mode->width;
    int h = mode->height;
    if (wantsWholeArea) {
        glfwGetMonitorWorkarea(monitor, &x, &y, &w, &h);
    } else {
        w = width;
        h = height;
    }
    GLFWwindow* window = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return nullptr;
    }
    if (wantsWholeArea) glfwSetWindowPos(window, x, y);
    glfwGetWindowSize(window, &w, &h);
    width = static_cast<uint32_t>(w);
    height = static_cast<uint32_t>(h);
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    });
    return window;
}

std::string readShaderFile(const std::filesystem::path& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open shader file '" + filename.string() + "'.");
    }
    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

glslang_stage_t shaderStageFromFilename(const std::filesystem::path& filename) {
    const std::string name = filename.filename().string();
    if (name.find(".vert") != std::string::npos) return GLSLANG_STAGE_VERTEX;
    if (name.find(".frag") != std::string::npos) return GLSLANG_STAGE_FRAGMENT;
    if (name.find(".comp") != std::string::npos) return GLSLANG_STAGE_COMPUTE;
    if (name.find(".geom") != std::string::npos) return GLSLANG_STAGE_GEOMETRY;
    if (name.find(".tesc") != std::string::npos) return GLSLANG_STAGE_TESSCONTROL;
    if (name.find(".tese") != std::string::npos) return GLSLANG_STAGE_TESSEVALUATION;
    if (name.find(".rgen") != std::string::npos) return GLSLANG_STAGE_RAYGEN;
    if (name.find(".rahit") != std::string::npos) return GLSLANG_STAGE_ANYHIT;
    if (name.find(".rchit") != std::string::npos) return GLSLANG_STAGE_CLOSESTHIT;
    if (name.find(".rmiss") != std::string::npos) return GLSLANG_STAGE_MISS;
    throw std::invalid_argument("Cannot infer shader stage from '" + filename.string() + "'.");
}

std::vector<uint32_t> compileShader(
    glslang_stage_t stage,
    const char* code,
    const glslang_resource_t* glslangResource = glslang_default_resource()
) {
    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_3,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_6,
        .code = code,
        .default_version = 100,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = glslangResource,
    };
    using ShaderPtr = std::unique_ptr<glslang_shader_t, decltype(&glslang_shader_delete)>;
    using ProgramPtr = std::unique_ptr<glslang_program_t, decltype(&glslang_program_delete)>;
    ShaderPtr shader(glslang_shader_create(&input), glslang_shader_delete);
    if (!shader) {
        throw std::runtime_error("Failed to create glslang shader.");
    }
    if (!glslang_shader_preprocess(shader.get(), &input)) {
        printf("shader preprocessing failed: \n");
        printf("    %s\n", glslang_shader_get_info_log(shader.get()));
        printf("    %s\n", glslang_shader_get_info_debug_log(shader.get()));
        std::cerr << code << '\n';
        throw std::runtime_error("Shader compilation failed");
    }

    glslang_shader_set_preprocessed_code(
        shader.get(),
        glslang_shader_get_preprocessed_code(shader.get())
    );
    if (!glslang_shader_parse(shader.get(), &input)) {
        printf("shader parsing failed: \n");
        printf("    %s\n", glslang_shader_get_info_log(shader.get()));
        printf("    %s\n", glslang_shader_get_info_debug_log(shader.get()));
        std::cerr << glslang_shader_get_preprocessed_code(shader.get()) << '\n';
        throw std::runtime_error("Shader compilation failed");
    }

    ProgramPtr program(glslang_program_create(), glslang_program_delete);
    if (!program) {
        throw std::runtime_error("Failed to create glslang program.");
    }
    glslang_program_add_shader(program.get(), shader.get());

    if (!glslang_program_link(program.get(), GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        printf("shader linking failed: \n");
        printf("    %s\n", glslang_program_get_info_log(program.get()));
        printf("    %s\n", glslang_program_get_info_debug_log(program.get()));
        throw std::runtime_error("Shader compilation failed");
    }

    glslang_spv_options_t options = {
        .generate_debug_info = true,
        .strip_debug_info = false,
        .disable_optimizer = false,
        .optimize_size = true,
        .disassemble = false,
        .validate = true,
        .emit_nonsemantic_shader_debug_info = false,
        .emit_nonsemantic_shader_debug_source = false
    };

    glslang_program_SPIRV_generate_with_options(program.get(), input.stage, &options);

    if (glslang_program_SPIRV_get_messages(program.get())) {
        printf("%s\n", glslang_program_SPIRV_get_messages(program.get()));
    }

    const std::size_t wordCount = glslang_program_SPIRV_get_size(program.get());
    const uint32_t* spirv = glslang_program_SPIRV_get_ptr(program.get());
    return std::vector<uint32_t>(spirv, spirv + wordCount);
}

void saveSPIRVBinaryFile(const std::filesystem::path& filename, const std::vector<uint32_t>& code) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to create SPIR-V file '" + filename.string() + "'.");
    }
    file.write(
        reinterpret_cast<const char*>(code.data()),
        static_cast<std::streamsize>(code.size() * sizeof(uint32_t))
    );
    if (!file) {
        throw std::runtime_error("Failed to write SPIR-V file '" + filename.string() + "'.");
    }
}

std::filesystem::path compileShaderFile(
    const std::filesystem::path& sourceFilename,
    const std::filesystem::path& cacheDirectory
) {
    std::filesystem::create_directories(cacheDirectory);
    const std::string shaderSource = readShaderFile(sourceFilename);
    const std::vector<uint32_t> spirv = compileShader(
        shaderStageFromFilename(sourceFilename),
        shaderSource.c_str()
    );
    const std::filesystem::path destination =
        cacheDirectory / (sourceFilename.stem().string() + ".bin");
    saveSPIRVBinaryFile(destination, spirv);
    return destination;
}

int main() {
    uint32_t width = 1280;
    uint32_t height = 800;
    GLFWwindow* window = initWindow("wgfx", width, height);
    if (window == nullptr) {
        std::cerr << "Failed to initialize GLFW window.\n";
        return -1;
    }
    if (!glslang_initialize_process()) {
        std::cerr << "Failed to initialize glslang.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    int exitCode = 0;
    try {
        const std::filesystem::path cacheDirectory = ".cache";
        const std::filesystem::path vertexBinary = compileShaderFile(
            "res/shaders/vulkan/test.vert.glsl",
            cacheDirectory
        );
        const std::filesystem::path fragmentBinary = compileShaderFile(
            "res/shaders/vulkan/test.frag.glsl",
            cacheDirectory
        );
        std::cout << "Compiled " << vertexBinary.string() << '\n';
        std::cout << "Compiled " << fragmentBinary.string() << '\n';

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    } catch (const std::exception& exception) {
        std::cerr << "Application error: " << exception.what() << '\n';
        exitCode = -1;
    }
    glslang_finalize_process();
    glfwDestroyWindow(window);
    glfwTerminate();
    // try {
    //     // wgfx::Renderer renderer;
    //     // renderer.init();
    //     // renderer.run();
    // } catch (const std::exception& exception) {
    //     std::cerr << "Application error: " << exception.what() << '\n';
    //     return -1;
    // }
    return exitCode;
}
