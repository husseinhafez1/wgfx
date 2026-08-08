#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <tiny_obj_loader.h>
#include <stb_image.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>
#include <string>

#include "shader.h"
#include "camera.h"
#include "input.h"
#include "buffer.h"
#include "model.h"

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
        Shader shader("basic.vert.glsl", "basic.frag.glsl");
        Model cow("helmet/DamagedHelmet.glb");

        float lastFrameTime = 0.0f;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
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

            model = glm::rotate(model, glm::radians(20.0f * deltaTime), glm::vec3(0.0f, 1.0f, 0.0f));

            view = camera.getViewMatrix();
            projection = camera.getProjectionMatrix();
            
            shader.use();
            shader.setUniform("model", model);
            shader.setUniform("view", view);
            shader.setUniform("projection", projection);
            cow.draw();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
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
