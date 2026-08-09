#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
#include "skybox.h"
#include "window.h"
#include "lighting.h"
#include "spot_shadow_map.h"
#include "point_shadow_map.h"

Camera camera;
Input input;

void processInput(GLFWwindow* window, float deltaTime);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int main() {
    Window window("wgfx", 960, 720);

    glEnable(GL_DEPTH_TEST);

    try {
        Shader shader("pbr.vert.glsl", "pbr.frag.glsl");
        Shader shadowShader("shadow.vert.glsl", "shadow.frag.glsl");
        Shader pointShadowShader("point_shadow.vert.glsl", "point_shadow.frag.glsl");
        SpotShadowMap spotShadowMap(2048);
        PointShadowMap pointShadowMap(1024);
        Model sponza("sponza/sponza.glb");
        Model helmet("helmet/DamagedHelmet.glb");
        Skybox skybox({
            "skybox/right.jpg",
            "skybox/left.jpg",
            "skybox/top.jpg",
            "skybox/bottom.jpg",
            "skybox/front.jpg",
            "skybox/back.jpg",
        });

        float lastFrameTime = 0.0f;

        const glm::mat4 sponzaModel(1.0f);
        const glm::vec3 helmetPosition(0.0f, 3.0f, 0.0f);
        const glm::mat4 helmetModel = glm::rotate(
            glm::translate(glm::mat4(1.0f), helmetPosition),
            glm::radians(90.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix();
        Lighting lighting;
        const glm::vec3 spotlightPosition = helmetPosition + glm::vec3(-5.0f, 0.0f, 0.0f);
        const SpotLight helmetSpotlight{
            spotlightPosition,
            glm::normalize(helmetPosition - spotlightPosition),
            glm::vec3(1.0f),
            150.0f,
            15.0f,
            25.0f,
            35.0f
        };
        const PointLight helmetPointLight{
            glm::vec3(2.0f, 3.0f, 0.0f),
            glm::vec3(1.0f, 0.85f, 0.7f),
            120.0f,
            15.0f
        };
        lighting.addSpotLight(helmetSpotlight);
        lighting.addPointLight(helmetPointLight);
        spotShadowMap.update(helmetSpotlight);
        pointShadowMap.update(helmetPointLight);

        shader.use();
        shader.setUniform("view", view);
        shader.setUniform("projection", projection);
        shader.setUniform("environmentMap", 1);
        shader.setUniform("spotShadowMap", 3);
        shader.setUniform("spotShadowLightIndex", 0);
        shader.setUniform("spotLightSpaceMatrix", spotShadowMap.getLightSpaceMatrix());
        shader.setUniform("pointShadowMap", 4);
        shader.setUniform("pointShadowLightIndex", 0);
        shader.setUniform("pointShadowFarPlane", pointShadowMap.getFarPlane());
        lighting.upload(shader);

        while (!glfwWindowShouldClose(window.get())) {
            float currentFrameTime = static_cast<float>(glfwGetTime());
            float deltaTime = currentFrameTime - lastFrameTime;
            lastFrameTime = currentFrameTime;
            processInput(window.get(), deltaTime);
            // if (input.onKeyPress(GLFW_KEY_R)) {
            //     // Recompile shaders
            //     std::cout << "Recompiling shaders..." << std::endl;
            //     // shader.recompile();
            // }
            if (window.getHeight() > 0) {
                camera.setAspectRatio(
                    static_cast<float>(window.getWidth()) / static_cast<float>(window.getHeight())
                );
            }
            view = camera.getViewMatrix();
            projection = camera.getProjectionMatrix();

            spotShadowMap.bindForWriting();
            glClear(GL_DEPTH_BUFFER_BIT);
            shadowShader.use();
            shadowShader.setUniform("lightSpaceMatrix", spotShadowMap.getLightSpaceMatrix());
            shadowShader.setUniform("model", sponzaModel);
            sponza.draw(shadowShader);
            shadowShader.setUniform("model", helmetModel);
            helmet.draw(shadowShader);

            pointShadowShader.use();
            pointShadowShader.setUniform("lightPosition", pointShadowMap.getLightPosition());
            pointShadowShader.setUniform("farPlane", pointShadowMap.getFarPlane());
            for (int face = 0; face < 6; ++face) {
                pointShadowMap.bindFaceForWriting(face);
                glClear(GL_DEPTH_BUFFER_BIT);
                pointShadowShader.setUniform(
                    "lightSpaceMatrix",
                    pointShadowMap.getLightSpaceMatrix(face)
                );
                pointShadowShader.setUniform("model", sponzaModel);
                sponza.draw(pointShadowShader);
                pointShadowShader.setUniform("model", helmetModel);
                helmet.draw(pointShadowShader);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, window.getWidth(), window.getHeight());
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            shader.use();
            shader.setUniform("view", view);
            shader.setUniform("projection", projection);
            shader.setUniform("cameraPosition", camera.getPosition());
            skybox.bind(1);
            spotShadowMap.bind(3);
            pointShadowMap.bind(4);

            shader.setUniform("model", sponzaModel);
            sponza.draw(shader);

            shader.setUniform("model", helmetModel);
            helmet.draw(shader);
            skybox.draw(view, projection);

            window.pollEvents();
        }
    } catch (const std::exception& exception) {
        std::cerr << "Application error: " << exception.what() << '\n';
        return -1;
    }
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
