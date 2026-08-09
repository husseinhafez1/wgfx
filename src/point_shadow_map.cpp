#include "point_shadow_map.h"

#include "lighting.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>

PointShadowMap::PointShadowMap(int resolution) : resolution(resolution) {
    if (resolution <= 0) {
        throw std::invalid_argument("Point shadow-map resolution must be positive.");
    }

    glGenFramebuffers(1, &framebuffer);
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (int face = 0; face < 6; ++face) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            0,
            GL_DEPTH_COMPONENT24,
            resolution,
            resolution,
            0,
            GL_DEPTH_COMPONENT,
            GL_UNSIGNED_INT,
            nullptr
        );
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        depthCubemap,
        0
    );
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteTextures(1, &depthCubemap);
        glDeleteFramebuffers(1, &framebuffer);
        throw std::runtime_error("Failed to create point shadow-map framebuffer.");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

PointShadowMap::~PointShadowMap() {
    glDeleteTextures(1, &depthCubemap);
    glDeleteFramebuffers(1, &framebuffer);
}

void PointShadowMap::update(const PointLight& light) {
    if (light.range <= 0.1f) {
        throw std::invalid_argument("Point-light range must exceed the shadow near plane.");
    }

    lightPosition = light.position;
    farPlane = light.range;
    const glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, farPlane);
    const std::array<glm::vec3, 6> directions{
        glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)
    };
    const std::array<glm::vec3, 6> upVectors{
        glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)
    };
    for (int face = 0; face < 6; ++face) {
        lightSpaceMatrices[face] = projection * glm::lookAt(
            lightPosition,
            lightPosition + directions[face],
            upVectors[face]
        );
    }
}

void PointShadowMap::copyFrom(const PointShadowMap& source) {
    if (source.resolution != resolution) {
        throw std::invalid_argument("Point shadow maps must have matching resolutions.");
    }
    glCopyImageSubData(
        source.depthCubemap,
        GL_TEXTURE_CUBE_MAP,
        0,
        0,
        0,
        0,
        depthCubemap,
        GL_TEXTURE_CUBE_MAP,
        0,
        0,
        0,
        0,
        resolution,
        resolution,
        6
    );
}

void PointShadowMap::bindFaceForWriting(int face) const {
    if (face < 0 || face >= 6) {
        throw std::out_of_range("Point shadow-map face is out of range.");
    }
    glViewport(0, 0, resolution, resolution);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
        depthCubemap,
        0
    );
}

void PointShadowMap::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
}

const glm::mat4& PointShadowMap::getLightSpaceMatrix(int face) const {
    if (face < 0 || face >= 6) {
        throw std::out_of_range("Point shadow-map face is out of range.");
    }
    return lightSpaceMatrices[face];
}

const glm::vec3& PointShadowMap::getLightPosition() const {
    return lightPosition;
}

float PointShadowMap::getFarPlane() const {
    return farPlane;
}
