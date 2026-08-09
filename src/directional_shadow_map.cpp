#include "directional_shadow_map.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
glm::vec3 transformPoint(const glm::mat4& matrix, const glm::vec3& point) {
    const glm::vec4 transformed = matrix * glm::vec4(point, 1.0f);
    return glm::vec3(transformed) / transformed.w;
}
}

DirectionalShadowMap::DirectionalShadowMap(int width, int height, int cascadeCount)
    : width(width),
      height(height),
      cascadeCount(cascadeCount),
      cascadeDistances(cascadeCount),
      cascadeDepthRanges(cascadeCount),
      lightSpaceMatrices(cascadeCount, glm::mat4(1.0f)) {
    if (width <= 0 || height <= 0 || cascadeCount <= 0 || cascadeCount > MaxCascades) {
        throw std::invalid_argument("Invalid cascaded shadow-map dimensions or cascade count.");
    }

    glGenFramebuffers(1, &framebuffer);
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, depthTexture);
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        GL_DEPTH_COMPONENT32F,
        width,
        height,
        cascadeCount,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    constexpr float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteTextures(1, &depthTexture);
        glDeleteFramebuffers(1, &framebuffer);
        depthTexture = 0;
        framebuffer = 0;
        throw std::runtime_error("Failed to create shadow-map framebuffer.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

DirectionalShadowMap::~DirectionalShadowMap() {
    glDeleteTextures(1, &depthTexture);
    glDeleteFramebuffers(1, &framebuffer);
}

void DirectionalShadowMap::updateCascades(
    const glm::mat4& view,
    const glm::mat4& projection,
    float cameraNearPlane,
    float cameraFarPlane,
    const glm::vec3& lightDirection
) {
    constexpr float splitBlend = 0.7f;
    for (int cascade = 0; cascade < cascadeCount; ++cascade) {
        const float fraction = static_cast<float>(cascade + 1) / static_cast<float>(cascadeCount);
        const float logarithmic = cameraNearPlane
                                * std::pow(cameraFarPlane / cameraNearPlane, fraction);
        const float uniform = cameraNearPlane + (cameraFarPlane - cameraNearPlane) * fraction;
        cascadeDistances[cascade] = glm::mix(uniform, logarithmic, splitBlend);
    }
    cascadeDistances.back() = cameraFarPlane;

    const glm::mat4 inverseViewProjection = glm::inverse(projection * view);
    std::array<glm::vec3, 4> cameraNearCorners;
    std::array<glm::vec3, 4> cameraFarCorners;
    int corner = 0;
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            cameraNearCorners[corner] = transformPoint(
                inverseViewProjection,
                glm::vec3(2.0f * x - 1.0f, 2.0f * y - 1.0f, -1.0f)
            );
            cameraFarCorners[corner] = transformPoint(
                inverseViewProjection,
                glm::vec3(2.0f * x - 1.0f, 2.0f * y - 1.0f, 1.0f)
            );
            ++corner;
        }
    }

    float previousDistance = cameraNearPlane;
    for (int cascade = 0; cascade < cascadeCount; ++cascade) {
        constexpr float transitionFraction = 0.1f;
        const float cascadeSpan = cascadeDistances[cascade] - previousDistance;
        const float previousCascadeNear = cascade > 1
            ? cascadeDistances[cascade - 2]
            : cameraNearPlane;
        const float fittedNearDistance = cascade == 0
            ? previousDistance
            : previousDistance - (previousDistance - previousCascadeNear)
                               * transitionFraction;
        const float fittedFarDistance = std::min(
            cameraFarPlane,
            cascadeDistances[cascade] + cascadeSpan * transitionFraction
        );
        const float nearRatio = (fittedNearDistance - cameraNearPlane)
                              / (cameraFarPlane - cameraNearPlane);
        const float farRatio = (fittedFarDistance - cameraNearPlane)
                             / (cameraFarPlane - cameraNearPlane);

        std::array<glm::vec3, 8> corners;
        for (int i = 0; i < 4; ++i) {
            const glm::vec3 ray = cameraFarCorners[i] - cameraNearCorners[i];
            corners[i] = cameraNearCorners[i] + ray * nearRatio;
            corners[i + 4] = cameraNearCorners[i] + ray * farRatio;
        }

        glm::vec3 center(0.0f);
        for (const glm::vec3& frustumCorner : corners) {
            center += frustumCorner;
        }
        center /= static_cast<float>(corners.size());

        float radius = 0.0f;
        for (const glm::vec3& frustumCorner : corners) {
            radius = std::max(radius, glm::length(frustumCorner - center));
        }
        // A fixed square footprint prevents the projection scale from changing as the
        // camera rotates relative to the light. Quantization absorbs floating-point jitter.
        radius = std::ceil(radius * 16.0f) / 16.0f;
        // A receiver can be inside a near cascade while its caster is much farther
        // along the light direction. Cover the camera range so those casters do not
        // appear and disappear as the receiver changes cascades.
        const float casterPadding = std::max(cameraFarPlane, radius * 0.5f);
        const glm::vec3 direction = glm::normalize(lightDirection);
        glm::vec3 lightUp(0.0f, 1.0f, 0.0f);
        if (std::abs(glm::dot(direction, lightUp)) > 0.99f) {
            lightUp = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        const glm::mat4 lightView = glm::lookAt(
            center - direction * (radius + casterPadding),
            center,
            lightUp
        );

        glm::vec3 minimum(std::numeric_limits<float>::max());
        glm::vec3 maximum(std::numeric_limits<float>::lowest());
        for (const glm::vec3& frustumCorner : corners) {
            const glm::vec3 lightSpaceCorner = glm::vec3(lightView * glm::vec4(frustumCorner, 1.0f));
            minimum = glm::min(minimum, lightSpaceCorner);
            maximum = glm::max(maximum, lightSpaceCorner);
        }
        minimum.x = -radius;
        maximum.x = radius;
        minimum.y = -radius;
        maximum.y = radius;

        const float lightNear = std::max(0.1f, -maximum.z - casterPadding);
        const float lightFar = std::max(lightNear + 1.0f, -minimum.z + casterPadding);
        cascadeDepthRanges[cascade] = lightFar - lightNear;
        glm::mat4 lightProjection = glm::ortho(
            minimum.x,
            maximum.x,
            minimum.y,
            maximum.y,
            lightNear,
            lightFar
        );

        // Snap the world origin in shadow-map space to reduce shimmering while moving.
        glm::vec4 shadowOrigin = lightProjection * lightView * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        shadowOrigin.x *= static_cast<float>(width) * 0.5f;
        shadowOrigin.y *= static_cast<float>(height) * 0.5f;
        const glm::vec2 roundedOrigin = glm::round(glm::vec2(shadowOrigin));
        lightProjection[3][0] += (roundedOrigin.x - shadowOrigin.x) * 2.0f / static_cast<float>(width);
        lightProjection[3][1] += (roundedOrigin.y - shadowOrigin.y) * 2.0f / static_cast<float>(height);

        lightSpaceMatrices[cascade] = lightProjection * lightView;
        previousDistance = cascadeDistances[cascade];
    }
}

void DirectionalShadowMap::bindLayerForWriting(int layer) const {
    if (layer < 0 || layer >= cascadeCount) {
        throw std::out_of_range("Shadow-map cascade layer is out of range.");
    }
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0, layer);
}

void DirectionalShadowMap::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D_ARRAY, depthTexture);
}

int DirectionalShadowMap::getWidth() const {
    return width;
}

int DirectionalShadowMap::getHeight() const {
    return height;
}

int DirectionalShadowMap::getCascadeCount() const {
    return cascadeCount;
}

const std::vector<float>& DirectionalShadowMap::getCascadeDistances() const {
    return cascadeDistances;
}

const std::vector<float>& DirectionalShadowMap::getCascadeDepthRanges() const {
    return cascadeDepthRanges;
}

const std::vector<glm::mat4>& DirectionalShadowMap::getLightSpaceMatrices() const {
    return lightSpaceMatrices;
}
