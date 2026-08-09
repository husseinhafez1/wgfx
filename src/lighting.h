#pragma once

#include <glm/glm.hpp>

#include <vector>

class Shader;

struct DirectionalLight {
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
};

struct PointLight {
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
};

struct SpotLight {
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float innerConeAngle = 12.5f;
    float outerConeAngle = 17.5f;
};

class Lighting {
public:
    static constexpr int MaxPointLights = 8;
    static constexpr int MaxSpotLights = 8;

    void setDirectionalLight(const DirectionalLight& light);
    void clearDirectionalLight();
    void addPointLight(const PointLight& light);
    void addSpotLight(const SpotLight& light);
    void clearPointLights();
    void clearSpotLights();
    void upload(Shader& shader) const;

private:
    DirectionalLight directionalLight;
    bool hasDirectionalLight = false;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
};
