#include "lighting.h"

#include "shader.h"

#include <cmath>
#include <stdexcept>
#include <string>

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

void Lighting::clearPointLights() {
    pointLights.clear();
}

void Lighting::clearSpotLights() {
    spotLights.clear();
}

void Lighting::upload(Shader& shader) const {
    shader.use();
    shader.setUniform("hasDirectionalLight", hasDirectionalLight ? 1 : 0);
    if (hasDirectionalLight) {
        shader.setUniform("directionalLight.direction", glm::normalize(directionalLight.direction));
        shader.setUniform("directionalLight.color", directionalLight.color);
        shader.setUniform("directionalLight.intensity", directionalLight.intensity);
    }

    shader.setUniform("pointLightCount", static_cast<int>(pointLights.size()));
    for (std::size_t index = 0; index < pointLights.size(); ++index) {
        const PointLight& light = pointLights[index];
        const std::string prefix = "pointLights[" + std::to_string(index) + "].";
        shader.setUniform(prefix + "position", light.position);
        shader.setUniform(prefix + "color", light.color);
        shader.setUniform(prefix + "intensity", light.intensity);
        shader.setUniform(prefix + "range", light.range);
    }

    shader.setUniform("spotLightCount", static_cast<int>(spotLights.size()));
    for (std::size_t index = 0; index < spotLights.size(); ++index) {
        const SpotLight& light = spotLights[index];
        const std::string prefix = "spotLights[" + std::to_string(index) + "].";
        shader.setUniform(prefix + "position", light.position);
        shader.setUniform(prefix + "direction", glm::normalize(light.direction));
        shader.setUniform(prefix + "color", light.color);
        shader.setUniform(prefix + "intensity", light.intensity);
        shader.setUniform(prefix + "range", light.range);
        shader.setUniform(prefix + "innerConeCos", std::cos(glm::radians(light.innerConeAngle)));
        shader.setUniform(prefix + "outerConeCos", std::cos(glm::radians(light.outerConeAngle)));
    }
}
