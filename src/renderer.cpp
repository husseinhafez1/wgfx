#include "renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "buffer.h"
#include "directional_shadow_map.h"
#include "framebuffer.h"
#include "input.h"
#include "imgui_layer.h"
#include "lighting.h"
#include "model.h"
#include "point_shadow_map.h"
#include "shader.h"
#include "skybox.h"
#include "spot_shadow_map.h"
#include "window.h"

#include <array>
#include <stdexcept>
#include <string>

namespace wgfx {
namespace {
constexpr float FramebufferVertices[] = {
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f
};

constexpr unsigned int FramebufferIndices[] = {
    0, 1, 2,
    2, 3, 0
};
}

Renderer::Renderer() = default;

Renderer::~Renderer() = default;

void Renderer::init() {
    if (initialized) {
        throw std::logic_error("Renderer is already initialized.");
    }

    window = std::make_unique<Window>("wgfx", 1200, 900, GraphicsBackend::OpenGL);
    if (!window->isOpen()) {
        throw std::runtime_error("Failed to initialize the renderer window.");
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    gui = std::make_unique<ImGuiLayer>(window->get());
    camera = std::make_unique<Camera>();
    input = std::make_unique<Input>();
    lighting = std::make_unique<Lighting>();
    framebuffer = std::make_unique<Framebuffer>(window->getWidth(), window->getHeight());
    framebufferVao = std::make_unique<VAO>();
    framebufferVbo = std::make_unique<VBO>(FramebufferVertices, sizeof(FramebufferVertices));
    framebufferEbo = std::make_unique<EBO>(FramebufferIndices, sizeof(FramebufferIndices));
    framebufferVao->linkVBO(*framebufferVbo, 0, 3, 5 * sizeof(float));
    framebufferVao->linkVBO(*framebufferVbo, 1, 2, 5 * sizeof(float), 3 * sizeof(float));
    framebufferVao->bind();
    framebufferEbo->bind();
    shaders.emplace(
        ShaderType::Framebuffer,
        std::make_unique<Shader>("GL/framebuffer.vert.glsl", "GL/framebuffer.frag.glsl")
    );
    shaders.emplace(
        ShaderType::BloomExtract,
        std::make_unique<Shader>("GL/framebuffer.vert.glsl", "GL/bloom_extract.frag.glsl")
    );
    shaders.emplace(
        ShaderType::BloomBlur,
        std::make_unique<Shader>("GL/framebuffer.vert.glsl", "GL/bloom_blur.frag.glsl")
    );
    shaders.emplace(
        ShaderType::Pbr,
        std::make_unique<Shader>("GL/pbr.vert.glsl", "GL/pbr.frag.glsl")
    );
    shaders.emplace(
        ShaderType::Shadow,
        std::make_unique<Shader>("GL/shadow.vert.glsl", "GL/shadow.frag.glsl")
    );
    shaders.emplace(
        ShaderType::PointShadow,
        std::make_unique<Shader>("GL/point_shadow.vert.glsl", "GL/point_shadow.frag.glsl")
    );

    staticSpotShadowMap = std::make_unique<SpotShadowMap>(1024);
    spotShadowMap = std::make_unique<SpotShadowMap>(1024);
    staticPointShadowMap = std::make_unique<PointShadowMap>(512);
    pointShadowMap = std::make_unique<PointShadowMap>(512);
    models.emplace(ModelType::Sponza, std::make_unique<Model>("sponza/Sponza.gltf"));
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
    directionalShadowMap = std::make_unique<DirectionalShadowMap>(1024, 1024, 3);

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
    getShader(ShaderType::Framebuffer).use();
    getShader(ShaderType::Framebuffer).setUniform("screenTexture", 0);
    getShader(ShaderType::Framebuffer).setUniform("bloomTexture", 1);
    getShader(ShaderType::BloomExtract).use();
    getShader(ShaderType::BloomExtract).setUniform("scene", 0);
    getShader(ShaderType::BloomBlur).use();
    getShader(ShaderType::BloomBlur).setUniform("image", 0);
    renderStaticShadowMaps();
    initialized = true;
}

void Renderer::run() {
    if (!initialized) {
        throw std::logic_error("Renderer must be initialized before running.");
    }

    while (window->isOpen()) {
        gui->beginFrame();
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
        renderFramebuffer();
        renderGui();
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
    shader.setUniform("directionalShadowEnabled", 0);
    shader.setUniform("shadowMap", 5);
    shader.setUniform("spotShadowMap", 3);
    shader.setUniform(
        "spotShadowLightIndex",
        lighting->getSpotLights().empty() ? -1 : 0
    );
    shader.setUniform("spotLightSpaceMatrix", spotShadowMap->getLightSpaceMatrix());
    shader.setUniform("pointShadowMap", 4);
    shader.setUniform(
        "pointShadowLightIndex",
        lighting->getPointLights().empty() ? -1 : 0
    );
    shader.setUniform("pointShadowFarPlane", pointShadowMap->getFarPlane());
    lighting->upload(shader);
}

void Renderer::updateShadowMaps() {
    if (!lighting->getSpotLights().empty()) {
        const SpotLight& light = lighting->getSpotLights().front();
        staticSpotShadowMap->update(light);
        spotShadowMap->update(light);
    }
    if (!lighting->getPointLights().empty()) {
        const PointLight& light = lighting->getPointLights().front();
        staticPointShadowMap->update(light);
        pointShadowMap->update(light);
    }
    renderStaticShadowMaps();
}

void Renderer::processInput(float deltaTime) {
    GLFWwindow* nativeWindow = window->get();
    if (input->onKeyPress(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(nativeWindow, true);
    }
    if (!gui->wantsKeyboardCapture() && input->onKeyRelease(GLFW_KEY_R)) {
        for (auto& [type, shader] : shaders) {
            shader->recompile();
        }
        configurePbrShader();
    }

    if (!gui->wantsKeyboardCapture() && input->onKeyHold(GLFW_KEY_W)) {
        camera->processInput(deltaTime, CameraMovement::FORWARD);
    }
    if (!gui->wantsKeyboardCapture() && input->onKeyHold(GLFW_KEY_S)) {
        camera->processInput(deltaTime, CameraMovement::BACKWARD);
    }
    if (!gui->wantsKeyboardCapture() && input->onKeyHold(GLFW_KEY_A)) {
        camera->processInput(deltaTime, CameraMovement::LEFT);
    }
    if (!gui->wantsKeyboardCapture() && input->onKeyHold(GLFW_KEY_D)) {
        camera->processInput(deltaTime, CameraMovement::RIGHT);
    }
    if (!gui->wantsKeyboardCapture() && input->onKeyHold(GLFW_KEY_Q)) {
        camera->processInput(deltaTime, CameraMovement::UP);
    }
    if (!gui->wantsKeyboardCapture() && input->onKeyHold(GLFW_KEY_E)) {
        camera->processInput(deltaTime, CameraMovement::DOWN);
    }

    if (!gui->wantsMouseCapture() && input->onButtonHold(GLFW_MOUSE_BUTTON_LEFT)) {
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
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);
    if (lighting->hasDirectional()) {
        renderDirectionalShadowMap(view, projection);
    }
    if (!lighting->getSpotLights().empty()) {
        spotShadowMap->copyFrom(*staticSpotShadowMap);
        renderSpotShadowMap();
    }
    if (!lighting->getPointLights().empty()) {
        pointShadowMap->copyFrom(*staticPointShadowMap);
        renderPointShadowMap();
    }
    glDisable(GL_POLYGON_OFFSET_FILL);

    framebuffer->bind(msaaEnabled);
    if (msaaEnabled) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }
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
    directionalShadowMap->bind(5);
    drawScene(shader);
    skybox->draw(view, projection);
}

void Renderer::renderFramebuffer() {
    framebuffer->resolve(msaaEnabled);
    glDisable(GL_DEPTH_TEST);
    const unsigned int bloomTextureIndex = bloomEnabled ? renderBloom() : 0;
    Framebuffer::unbind();
    glViewport(0, 0, window->getWidth(), window->getHeight());
    glClear(GL_COLOR_BUFFER_BIT);
    Shader& shader = getShader(ShaderType::Framebuffer);
    shader.use();
    shader.setUniform("bloomEnabled", bloomEnabled ? 1 : 0);
    shader.setUniform("exposure", exposure);
    shader.setUniform("gamma", gamma);
    framebuffer->bindColorTexture(0);
    if (bloomEnabled) {
        framebuffer->bindBloomTexture(bloomTextureIndex, 1);
    }
    framebufferVao->bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glEnable(GL_DEPTH_TEST);
}

unsigned int Renderer::renderBloom() {
    framebuffer->bindBloomExtraction();
    glClear(GL_COLOR_BUFFER_BIT);
    Shader& extractShader = getShader(ShaderType::BloomExtract);
    extractShader.use();
    extractShader.setUniform("threshold", bloomThreshold);
    framebuffer->bindColorTexture(0);
    framebufferVao->bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    constexpr int BlurPassCount = 10;
    bool horizontal = true;
    bool firstPass = true;
    Shader& blurShader = getShader(ShaderType::BloomBlur);
    blurShader.use();
    for (int pass = 0; pass < BlurPassCount; ++pass) {
        const unsigned int targetIndex = horizontal ? 1u : 0u;
        framebuffer->bindBloomBlur(targetIndex);
        blurShader.setUniform("horizontal", horizontal ? 1 : 0);
        if (firstPass) {
            framebuffer->bindBrightTexture(0);
            firstPass = false;
        } else {
            framebuffer->bindBloomTexture(1u - targetIndex, 0);
        }
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        horizontal = !horizontal;
    }
    return horizontal ? 0u : 1u;
}

void Renderer::renderDirectionalShadowMap(
    const glm::mat4& view,
    const glm::mat4& projection
) {
    directionalShadowMap->updateCascades(
        view,
        projection,
        camera->getNearPlane(),
        camera->getFarPlane(),
        lighting->getDirectionalLight().direction
    );

    Shader& depthShader = getShader(ShaderType::Shadow);
    depthShader.use();
    const auto& lightSpaceMatrices = directionalShadowMap->getLightSpaceMatrices();
    for (int cascade = 0; cascade < directionalShadowMap->getCascadeCount(); ++cascade) {
        directionalShadowMap->bindLayerForWriting(cascade);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthShader.setUniform("lightSpaceMatrix", lightSpaceMatrices[cascade]);
        drawDepthModel(depthShader, ModelType::Sponza, sponzaModel);
        drawDepthModel(depthShader, ModelType::DamagedHelmet, helmetModel);
    }

    Shader& pbrShader = getShader(ShaderType::Pbr);
    pbrShader.use();
    pbrShader.setUniform("directionalShadowEnabled", 1);
    pbrShader.setUniform("cascadeCount", directionalShadowMap->getCascadeCount());
    const auto& cascadeDistances = directionalShadowMap->getCascadeDistances();
    const auto& cascadeDepthRanges = directionalShadowMap->getCascadeDepthRanges();
    for (int cascade = 0; cascade < directionalShadowMap->getCascadeCount(); ++cascade) {
        const std::string index = std::to_string(cascade);
        pbrShader.setUniform("lightSpaceMatrices[" + index + "]", lightSpaceMatrices[cascade]);
        pbrShader.setUniform("cascadePlaneDistances[" + index + "]", cascadeDistances[cascade]);
        pbrShader.setUniform("cascadeDepthRanges[" + index + "]", cascadeDepthRanges[cascade]);
    }
}

void Renderer::renderGui() {
    bool vsyncEnabled = window->isVSyncEnabled();
    const ImGuiLayerChanges changes = gui->drawRendererPanel(
        vsyncEnabled,
        msaaEnabled,
        bloomEnabled,
        exposure,
        bloomThreshold,
        *lighting
    );
    if (changes.vsyncChanged) {
        window->setVSync(vsyncEnabled);
    }
    if (changes.shadowConfigurationChanged) {
        updateShadowMaps();
    }
    if (changes.lightingChanged) {
        configurePbrShader();
    }
    gui->endFrame();
}

void Renderer::renderStaticShadowMaps() {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    if (!lighting->getSpotLights().empty()) {
        staticSpotShadowMap->bindForWriting();
        glClear(GL_DEPTH_BUFFER_BIT);
        Shader& spotShader = getShader(ShaderType::Shadow);
        spotShader.use();
        spotShader.setUniform("lightSpaceMatrix", staticSpotShadowMap->getLightSpaceMatrix());
        drawDepthModel(spotShader, ModelType::Sponza, sponzaModel);
    }

    if (!lighting->getPointLights().empty()) {
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

} // namespace wgfx
