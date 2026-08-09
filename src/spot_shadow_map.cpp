#include "spot_shadow_map.h"

#include "lighting.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <stdexcept>

namespace wgfx {

SpotShadowMap::SpotShadowMap(int resolution) : resolution(resolution) {
    if (resolution <= 0) {
        throw std::invalid_argument("Spot shadow-map resolution must be positive.");
    }

    glGenFramebuffers(1, &framebuffer);
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT24,
        resolution,
        resolution,
        0,
        GL_DEPTH_COMPONENT,
        GL_UNSIGNED_INT,
        nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    constexpr float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteTextures(1, &depthTexture);
        glDeleteFramebuffers(1, &framebuffer);
        throw std::runtime_error("Failed to create spotlight shadow-map framebuffer.");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

SpotShadowMap::~SpotShadowMap() {
    glDeleteTextures(1, &depthTexture);
    glDeleteFramebuffers(1, &framebuffer);
}

void SpotShadowMap::update(const SpotLight& light) {
    if (light.range <= 0.1f || light.outerConeAngle <= 0.0f || light.outerConeAngle >= 89.0f) {
        throw std::invalid_argument("Invalid spotlight range or outer cone angle for shadow mapping.");
    }

    const glm::vec3 direction = glm::normalize(light.direction);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(direction, up)) > 0.99f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    const glm::mat4 projection = glm::perspective(
        glm::radians(light.outerConeAngle * 2.0f),
        1.0f,
        0.1f,
        light.range
    );
    const glm::mat4 view = glm::lookAt(light.position, light.position + direction, up);
    lightSpaceMatrix = projection * view;
}

void SpotShadowMap::copyFrom(const SpotShadowMap& source) {
    if (source.resolution != resolution) {
        throw std::invalid_argument("Spot shadow maps must have matching resolutions.");
    }
    glCopyImageSubData(
        source.depthTexture,
        GL_TEXTURE_2D,
        0,
        0,
        0,
        0,
        depthTexture,
        GL_TEXTURE_2D,
        0,
        0,
        0,
        0,
        resolution,
        resolution,
        1
    );
}

void SpotShadowMap::bindForWriting() const {
    glViewport(0, 0, resolution, resolution);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
}

void SpotShadowMap::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
}

const glm::mat4& SpotShadowMap::getLightSpaceMatrix() const {
    return lightSpaceMatrix;
}

} // namespace wgfx
