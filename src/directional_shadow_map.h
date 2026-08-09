#pragma once

#include <glm/glm.hpp>

#include <vector>

class DirectionalShadowMap {
public:
    static constexpr int MaxCascades = 8;

    DirectionalShadowMap(int width, int height, int cascadeCount = 4);
    ~DirectionalShadowMap();

    DirectionalShadowMap(const DirectionalShadowMap&) = delete;
    DirectionalShadowMap& operator=(const DirectionalShadowMap&) = delete;

    void updateCascades(
        const glm::mat4& view,
        const glm::mat4& projection,
        float cameraNearPlane,
        float cameraFarPlane,
        const glm::vec3& lightDirection
    );
    void bindLayerForWriting(int layer) const;
    void bind(unsigned int slot) const;
    [[nodiscard]] int getWidth() const;
    [[nodiscard]] int getHeight() const;
    [[nodiscard]] int getCascadeCount() const;
    [[nodiscard]] const std::vector<float>& getCascadeDistances() const;
    [[nodiscard]] const std::vector<float>& getCascadeDepthRanges() const;
    [[nodiscard]] const std::vector<glm::mat4>& getLightSpaceMatrices() const;

private:
    unsigned int framebuffer = 0;
    unsigned int depthTexture = 0;
    int width;
    int height;
    int cascadeCount;
    std::vector<float> cascadeDistances;
    std::vector<float> cascadeDepthRanges;
    std::vector<glm::mat4> lightSpaceMatrices;
};
