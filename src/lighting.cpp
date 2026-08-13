#include "lighting.h"

#include "shader.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace wgfx {
namespace {
glm::vec3 srgbToLinear(const glm::vec3& color) {
    return glm::pow(color, glm::vec3(2.2f));
}
}

void Lighting::setDirectionalLight(const DirectionalLight& light) {
    directionalLight = light;
    hasDirectionalLight = true;
}

void Lighting::clearDirectionalLight() {
    hasDirectionalLight = false;
}

void Lighting::addPointLight(const PointLight& light) {
    if (pointLights.size() >= MaxPointLights) {
        throw std::length_error("Maximum point light count exceeded.");
    }
    pointLights.push_back(light);
}

void Lighting::addSpotLight(const SpotLight& light) {
    if (spotLights.size() >= MaxSpotLights) {
        throw std::length_error("Maximum spot light count exceeded.");
    }
    spotLights.push_back(light);
}

void Lighting::removePointLight(std::size_t index) {
    if (index >= pointLights.size()) {
        throw std::out_of_range("Point light index is out of range.");
    }
    pointLights.erase(pointLights.begin() + static_cast<std::ptrdiff_t>(index));
}

void Lighting::removeSpotLight(std::size_t index) {
    if (index >= spotLights.size()) {
        throw std::out_of_range("Spot light index is out of range.");
    }
    spotLights.erase(spotLights.begin() + static_cast<std::ptrdiff_t>(index));
}

void Lighting::clearPointLights() {
    pointLights.clear();
}

void Lighting::clearSpotLights() {
    spotLights.clear();
}

bool Lighting::hasDirectional() const {
    return hasDirectionalLight;
}

DirectionalLight& Lighting::getDirectionalLight() {
    if (!hasDirectionalLight) {
        throw std::logic_error("No directional light is configured.");
    }
    return directionalLight;
}

std::vector<PointLight>& Lighting::getPointLights() {
    return pointLights;
}

std::vector<SpotLight>& Lighting::getSpotLights() {
    return spotLights;
}

const std::vector<PointLight>& Lighting::getPointLights() const {
    return pointLights;
}

const std::vector<SpotLight>& Lighting::getSpotLights() const {
    return spotLights;
}

void Lighting::upload(Shader& shader) const {
    shader.use();
    shader.setUniform("hasDirectionalLight", hasDirectionalLight ? 1 : 0);
    if (hasDirectionalLight) {
        shader.setUniform("directionalLight.direction", glm::normalize(directionalLight.direction));
        shader.setUniform("directionalLight.color", srgbToLinear(directionalLight.color));
        shader.setUniform("directionalLight.intensity", directionalLight.intensity);
    }

    shader.setUniform("pointLightCount", static_cast<int>(pointLights.size()));
    for (std::size_t index = 0; index < pointLights.size(); ++index) {
        const PointLight& light = pointLights[index];
        const std::string prefix = "pointLights[" + std::to_string(index) + "].";
        shader.setUniform(prefix + "position", light.position);
        shader.setUniform(prefix + "color", srgbToLinear(light.color));
        shader.setUniform(prefix + "intensity", light.intensity);
        shader.setUniform(prefix + "range", light.range);
    }

    shader.setUniform("spotLightCount", static_cast<int>(spotLights.size()));
    for (std::size_t index = 0; index < spotLights.size(); ++index) {
        const SpotLight& light = spotLights[index];
        const std::string prefix = "spotLights[" + std::to_string(index) + "].";
        shader.setUniform(prefix + "position", light.position);
        shader.setUniform(prefix + "direction", glm::normalize(light.direction));
        shader.setUniform(prefix + "color", srgbToLinear(light.color));
        shader.setUniform(prefix + "intensity", light.intensity);
        shader.setUniform(prefix + "range", light.range);
        shader.setUniform(prefix + "innerConeCos", std::cos(glm::radians(light.innerConeAngle)));
        shader.setUniform(prefix + "outerConeCos", std::cos(glm::radians(light.outerConeAngle)));
    }
}

} // namespace wgfx
