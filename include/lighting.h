#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

namespace wgfx {

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
    void removePointLight(std::size_t index);
    void removeSpotLight(std::size_t index);
    void clearPointLights();
    void clearSpotLights();
    [[nodiscard]] bool hasDirectional() const;
    [[nodiscard]] DirectionalLight& getDirectionalLight();
    [[nodiscard]] std::vector<PointLight>& getPointLights();
    [[nodiscard]] std::vector<SpotLight>& getSpotLights();
    [[nodiscard]] const std::vector<PointLight>& getPointLights() const;
    [[nodiscard]] const std::vector<SpotLight>& getSpotLights() const;
    void upload(Shader& shader) const;

private:
    DirectionalLight directionalLight;
    bool hasDirectionalLight = false;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
};

} // namespace wgfx
