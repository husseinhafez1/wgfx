#include "renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "input.h"
#include "lighting.h"
#include "model.h"
#include "point_shadow_map.h"
#include "shader.h"
#include "skybox.h"
#include "spot_shadow_map.h"
#include "window.h"

#include <array>
#include <stdexcept>

Renderer::Renderer() = default;

Renderer::~Renderer() = default;

void Renderer::init() {
    if (initialized) {
        throw std::logic_error("Renderer is already initialized.");
    }

    window = std::make_unique<Window>("wgfx", 960, 720);
    if (!window->isOpen()) {
        throw std::runtime_error("Failed to initialize the renderer window.");
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    camera = std::make_unique<Camera>();
    input = std::make_unique<Input>();
    lighting = std::make_unique<Lighting>();
    shaders.emplace(
        ShaderType::Pbr,
        std::make_unique<Shader>("pbr.vert.glsl", "pbr.frag.glsl")
    );
    shaders.emplace(
        ShaderType::Shadow,
        std::make_unique<Shader>("shadow.vert.glsl", "shadow.frag.glsl")
    );
    shaders.emplace(
        ShaderType::PointShadow,
        std::make_unique<Shader>("point_shadow.vert.glsl", "point_shadow.frag.glsl")
    );

    staticSpotShadowMap = std::make_unique<SpotShadowMap>(1024);
    spotShadowMap = std::make_unique<SpotShadowMap>(1024);
    staticPointShadowMap = std::make_unique<PointShadowMap>(512);
    pointShadowMap = std::make_unique<PointShadowMap>(512);
    models.emplace(ModelType::Sponza, std::make_unique<Model>("sponza/sponza.glb"));
    models.emplace(
        ModelType::DamagedHelmet,
        std::make_unique<Model>("helmet/DamagedHelmet.glb")
    );
    skybox = std::make_unique<Skybox>(std::array<std::string, 6>{
        "skybox/right.jpg",
        "skybox/left.jpg",
        "skybox/top.jpg",
        "skybox/bottom.jpg",
        "skybox/front.jpg",
        "skybox/back.jpg"
    });

    const glm::vec3 helmetPosition(0.0f, 3.0f, 0.0f);
    helmetModel = glm::rotate(
        glm::translate(glm::mat4(1.0f), helmetPosition),
        glm::radians(90.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
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
    lighting->addSpotLight(helmetSpotlight);
    lighting->addPointLight(helmetPointLight);
    staticSpotShadowMap->update(helmetSpotlight);
    spotShadowMap->update(helmetSpotlight);
    staticPointShadowMap->update(helmetPointLight);
    pointShadowMap->update(helmetPointLight);

    configurePbrShader();
    renderStaticShadowMaps();
    initialized = true;
}

void Renderer::run() {
    if (!initialized) {
        throw std::logic_error("Renderer must be initialized before running.");
    }

    while (window->isOpen()) {
        const float currentFrameTime = static_cast<float>(glfwGetTime());
        const float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;
        processInput(deltaTime);

        if (window->getHeight() > 0) {
            camera->setAspectRatio(
                static_cast<float>(window->getWidth()) / static_cast<float>(window->getHeight())
            );
        }
        renderFrame(camera->getViewMatrix(), camera->getProjectionMatrix());
        window->pollEvents();
    }
}

Shader& Renderer::getShader(ShaderType type) {
    return *shaders.at(type);
}

Model& Renderer::getModel(ModelType type) {
    return *models.at(type);
}

void Renderer::configurePbrShader() {
    Shader& shader = getShader(ShaderType::Pbr);
    shader.use();
    shader.setUniform("view", camera->getViewMatrix());
    shader.setUniform("projection", camera->getProjectionMatrix());
    shader.setUniform("environmentMap", 1);
    shader.setUniform("spotShadowMap", 3);
    shader.setUniform("spotShadowLightIndex", 0);
    shader.setUniform("spotLightSpaceMatrix", spotShadowMap->getLightSpaceMatrix());
    shader.setUniform("pointShadowMap", 4);
    shader.setUniform("pointShadowLightIndex", 0);
    shader.setUniform("pointShadowFarPlane", pointShadowMap->getFarPlane());
    lighting->upload(shader);
}

void Renderer::processInput(float deltaTime) {
    GLFWwindow* nativeWindow = window->get();
    if (input->onKeyPress(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(nativeWindow, true);
    }
    if (input->onKeyRelease(GLFW_KEY_R)) {
        for (auto& [type, shader] : shaders) {
            shader->recompile();
        }
        configurePbrShader();
    }

    if (input->onKeyHold(GLFW_KEY_W)) {
        camera->processInput(deltaTime, CameraMovement::FORWARD);
    }
    if (input->onKeyHold(GLFW_KEY_S)) {
        camera->processInput(deltaTime, CameraMovement::BACKWARD);
    }
    if (input->onKeyHold(GLFW_KEY_A)) {
        camera->processInput(deltaTime, CameraMovement::LEFT);
    }
    if (input->onKeyHold(GLFW_KEY_D)) {
        camera->processInput(deltaTime, CameraMovement::RIGHT);
    }
    if (input->onKeyHold(GLFW_KEY_Q)) {
        camera->processInput(deltaTime, CameraMovement::UP);
    }
    if (input->onKeyHold(GLFW_KEY_E)) {
        camera->processInput(deltaTime, CameraMovement::DOWN);
    }

    if (input->onButtonHold(GLFW_MOUSE_BUTTON_LEFT)) {
        double mouseX;
        double mouseY;
        glfwGetCursorPos(nativeWindow, &mouseX, &mouseY);
        if (rotating) {
            camera->processMouseMovement(
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

void Renderer::renderFrame(const glm::mat4& view, const glm::mat4& projection) {
    spotShadowMap->copyFrom(*staticSpotShadowMap);
    pointShadowMap->copyFrom(*staticPointShadowMap);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);
    renderSpotShadowMap();
    renderPointShadowMap();
    glDisable(GL_POLYGON_OFFSET_FILL);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window->getWidth(), window->getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Shader& shader = getShader(ShaderType::Pbr);
    shader.use();
    shader.setUniform("view", view);
    shader.setUniform("projection", projection);
    shader.setUniform("cameraPosition", camera->getPosition());
    skybox->bind(1);
    spotShadowMap->bind(3);
    pointShadowMap->bind(4);
    drawScene(shader);
    skybox->draw(view, projection);
}

void Renderer::renderStaticShadowMaps() {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    staticSpotShadowMap->bindForWriting();
    glClear(GL_DEPTH_BUFFER_BIT);
    Shader& spotShader = getShader(ShaderType::Shadow);
    spotShader.use();
    spotShader.setUniform("lightSpaceMatrix", staticSpotShadowMap->getLightSpaceMatrix());
    drawDepthModel(spotShader, ModelType::Sponza, sponzaModel);

    Shader& pointShader = getShader(ShaderType::PointShadow);
    pointShader.use();
    pointShader.setUniform("lightPosition", staticPointShadowMap->getLightPosition());
    pointShader.setUniform("farPlane", staticPointShadowMap->getFarPlane());
    for (int face = 0; face < 6; ++face) {
        staticPointShadowMap->bindFaceForWriting(face);
        glClear(GL_DEPTH_BUFFER_BIT);
        pointShader.setUniform(
            "lightSpaceMatrix",
            staticPointShadowMap->getLightSpaceMatrix(face)
        );
        drawDepthModel(pointShader, ModelType::Sponza, sponzaModel);
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderSpotShadowMap() {
    spotShadowMap->bindForWriting();
    Shader& shader = getShader(ShaderType::Shadow);
    shader.use();
    shader.setUniform("lightSpaceMatrix", spotShadowMap->getLightSpaceMatrix());
    drawDepthModel(shader, ModelType::DamagedHelmet, helmetModel);
}

void Renderer::renderPointShadowMap() {
    Shader& shader = getShader(ShaderType::PointShadow);
    shader.use();
    shader.setUniform("lightPosition", pointShadowMap->getLightPosition());
    shader.setUniform("farPlane", pointShadowMap->getFarPlane());
    for (int face = 0; face < 6; ++face) {
        pointShadowMap->bindFaceForWriting(face);
        shader.setUniform("lightSpaceMatrix", pointShadowMap->getLightSpaceMatrix(face));
        drawDepthModel(shader, ModelType::DamagedHelmet, helmetModel);
    }
}

void Renderer::drawScene(Shader& shader) {
    shader.setUniform("model", sponzaModel);
    getModel(ModelType::Sponza).draw(shader);
    shader.setUniform("model", helmetModel);
    getModel(ModelType::DamagedHelmet).draw(shader);
}

void Renderer::drawDepthModel(Shader& shader, ModelType type, const glm::mat4& transform) {
    shader.setUniform("model", transform);
    getModel(type).drawDepth();
}
