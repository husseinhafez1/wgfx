#pragma once

#include <glm/glm.hpp>

#include <array>

struct PointLight;

class PointShadowMap {
public:
    explicit PointShadowMap(int resolution);
    ~PointShadowMap();

    PointShadowMap(const PointShadowMap&) = delete;
    PointShadowMap& operator=(const PointShadowMap&) = delete;

    void update(const PointLight& light);
    void bindFaceForWriting(int face) const;
    void bind(unsigned int slot) const;
    [[nodiscard]] const glm::mat4& getLightSpaceMatrix(int face) const;
    [[nodiscard]] const glm::vec3& getLightPosition() const;
    [[nodiscard]] float getFarPlane() const;

private:
    unsigned int framebuffer = 0;
    unsigned int depthCubemap = 0;
    int resolution;
    glm::vec3 lightPosition{0.0f};
    float farPlane = 1.0f;
    std::array<glm::mat4, 6> lightSpaceMatrices{};
};
