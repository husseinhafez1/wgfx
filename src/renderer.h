#pragma once

#include <glm/mat4x4.hpp>

#include <memory>
#include <unordered_map>

class Camera;
class Input;
class Lighting;
class Model;
class PointShadowMap;
class Shader;
class Skybox;
class SpotShadowMap;
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
    void processInput(float deltaTime);
    void renderFrame(const glm::mat4& view, const glm::mat4& projection);
    void renderStaticShadowMaps();
    void renderSpotShadowMap();
    void renderPointShadowMap();
    void drawScene(Shader& shader);
    void drawDepthModel(Shader& shader, ModelType type, const glm::mat4& transform);

    // Declared first so the OpenGL context outlives every GPU resource.
    std::unique_ptr<Window> window;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Input> input;
    std::unique_ptr<Lighting> lighting;
    std::unordered_map<ShaderType, std::unique_ptr<Shader>> shaders;
    std::unordered_map<ModelType, std::unique_ptr<Model>> models;
    std::unique_ptr<Skybox> skybox;
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
    bool initialized = false;
};
