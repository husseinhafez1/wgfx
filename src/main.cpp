#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <tiny_obj_loader.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>
#include <string>

#include "shader.h"
#include "camera.h"
#include "input.h"
#include "buffer.h"

Camera camera;
Input input;

void processInput(GLFWwindow* window, float deltaTime);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "wgfx", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    {
        tinyobj::attrib_t attributes;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warning;
        std::string error;

        if (!tinyobj::LoadObj(
                &attributes,
                &shapes,
                &materials,
                &warning,
                &error,
                MODEL_DIR "cow/cow.obj"
            )) {
            std::cerr << warning << error << '\n';
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }

        if (!warning.empty()) {
            std::cerr << warning << '\n';
        }

        std::vector<float> vertices = attributes.vertices;
        std::vector<unsigned int> indices;
        for (const tinyobj::shape_t& shape : shapes) {
            for (const tinyobj::index_t& index : shape.mesh.indices) {
                if (index.vertex_index < 0) {
                    std::cerr << "Cow model contains an invalid vertex index.\n";
                    glfwDestroyWindow(window);
                    glfwTerminate();
                    return -1;
                }
                indices.push_back(static_cast<unsigned int>(index.vertex_index));
            }
        }

        glm::vec3 minimum(std::numeric_limits<float>::max());
        glm::vec3 maximum(std::numeric_limits<float>::lowest());
        for (std::size_t i = 0; i < vertices.size(); i += 3) {
            const glm::vec3 position(vertices[i], vertices[i + 1], vertices[i + 2]);
            minimum = glm::min(minimum, position);
            maximum = glm::max(maximum, position);
        }

        const glm::vec3 center = (minimum + maximum) * 0.5f;
        const glm::vec3 extent = maximum - minimum;
        const float scale = 2.0f / std::max({extent.x, extent.y, extent.z});
        for (std::size_t i = 0; i < vertices.size(); i += 3) {
            vertices[i] = (vertices[i] - center.x) * scale;
            vertices[i + 1] = (vertices[i + 1] - center.y) * scale;
            vertices[i + 2] = (vertices[i + 2] - center.z) * scale;
        }


        Shader shader("basic.vert.glsl", "basic.frag.glsl");

        unsigned int vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        Buffer vertexBuffer(BufferType::VertexBuffer, vertices.data(), vertices.size() * sizeof(float));
        Buffer indexBuffer(BufferType::IndexBuffer, indices.data(), indices.size() * sizeof(unsigned int));
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        float lastFrameTime = 0.0f;

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix();

        shader.use();
        shader.setUniform("model", model);
        shader.setUniform("view", view);
        shader.setUniform("projection", projection);

        while (!glfwWindowShouldClose(window)) {
            float currentFrameTime = static_cast<float>(glfwGetTime());
            float deltaTime = currentFrameTime - lastFrameTime;
            lastFrameTime = currentFrameTime;
            processInput(window, deltaTime);
            // if (input.onKeyPress(GLFW_KEY_R)) {
            //     // Recompile shaders
            //     std::cout << "Recompiling shaders..." << std::endl;
            //     // shader.recompile();
            // }
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            view = camera.getViewMatrix();
            projection = camera.getProjectionMatrix();

            shader.use();
            shader.setUniform("view", view);
            shader.setUniform("projection", projection);
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glDeleteVertexArrays(1, &vao);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window, float deltaTime) {
    static bool rotating = false;
    static double lastMouseX = 0.0;
    static double lastMouseY = 0.0;

    if (input.onKeyPress(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(window, true);
    }

    if (input.onKeyRelease(GLFW_KEY_R)) {
        // Recompile shaders
        std::cout << "Recompiling shaders..." << std::endl;
        // shader.recompile();
    }

    if (input.onKeyHold(GLFW_KEY_W))
        camera.processInput(deltaTime, CameraMovement::FORWARD);

    if (input.onKeyHold(GLFW_KEY_S))
        camera.processInput(deltaTime, CameraMovement::BACKWARD);

    if (input.onKeyHold(GLFW_KEY_A))
        camera.processInput(deltaTime, CameraMovement::LEFT);

    if (input.onKeyHold(GLFW_KEY_D))
        camera.processInput(deltaTime, CameraMovement::RIGHT);

    if (input.onKeyHold(GLFW_KEY_Q))
        camera.processInput(deltaTime, CameraMovement::UP);

    if (input.onKeyHold(GLFW_KEY_E))
        camera.processInput(deltaTime, CameraMovement::DOWN);

    if (input.onButtonHold(GLFW_MOUSE_BUTTON_LEFT)) {
        double mouseX;
        double mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        if (rotating) {
            camera.processMouseMovement(
                static_cast<float>(mouseX - lastMouseX),
                static_cast<float>(lastMouseY - mouseY)
            );
        }

        lastMouseX = mouseX;
        lastMouseY = mouseY;
        rotating = true;
    } else {
        rotating = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
