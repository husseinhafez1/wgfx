#pragma once

#include <glm/mat4x4.hpp>

#include <memory>
#include <unordered_map>

namespace wgfx {

class Camera;
class DirectionalShadowMap;
class EBO;
class Framebuffer;
class Input;
class ImGuiLayer;
class Lighting;
class Model;
class PointShadowMap;
class Shader;
class Skybox;
class SpotShadowMap;
class VAO;
class VBO;
class Window;

class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void init();
    void run();

private:
    enum class ShaderType {
        Pbr,
        Framebuffer,
        PointShadow,
        Shadow
    };

    enum class ModelType {
        Sponza,
        DamagedHelmet
    };

    Shader& getShader(ShaderType type);
    Model& getModel(ModelType type);
    void configurePbrShader();
    void updateShadowMaps();
    void processInput(float deltaTime);
    void renderFrame(const glm::mat4& view, const glm::mat4& projection);
    void renderFramebuffer();
    void renderGui();
    void renderStaticShadowMaps();
    void renderDirectionalShadowMap(const glm::mat4& view, const glm::mat4& projection);
    void renderSpotShadowMap();
    void renderPointShadowMap();
    void drawScene(Shader& shader);
    void drawDepthModel(Shader& shader, ModelType type, const glm::mat4& transform);

    // Declared first so the OpenGL context outlives every GPU resource.
    std::unique_ptr<Window> window;
    std::unique_ptr<ImGuiLayer> gui;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Input> input;
    std::unique_ptr<Lighting> lighting;
    std::unique_ptr<Framebuffer> framebuffer;
    std::unique_ptr<VAO> framebufferVao;
    std::unique_ptr<VBO> framebufferVbo;
    std::unique_ptr<EBO> framebufferEbo;
    std::unordered_map<ShaderType, std::unique_ptr<Shader>> shaders;
    std::unordered_map<ModelType, std::unique_ptr<Model>> models;
    std::unique_ptr<Skybox> skybox;
    std::unique_ptr<DirectionalShadowMap> directionalShadowMap;
    std::unique_ptr<SpotShadowMap> staticSpotShadowMap;
    std::unique_ptr<SpotShadowMap> spotShadowMap;
    std::unique_ptr<PointShadowMap> staticPointShadowMap;
    std::unique_ptr<PointShadowMap> pointShadowMap;
    glm::mat4 sponzaModel{1.0f};
    glm::mat4 helmetModel{1.0f};
    float lastFrameTime = 0.0f;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    bool rotating = false;
    bool msaaEnabled = true;
    bool initialized = false;
};

} // namespace wgfx
