#pragma once

#include <glm/glm.hpp>

namespace wgfx {

struct SpotLight;

class SpotShadowMap {
public:
    explicit SpotShadowMap(int resolution);
    ~SpotShadowMap();

    SpotShadowMap(const SpotShadowMap&) = delete;
    SpotShadowMap& operator=(const SpotShadowMap&) = delete;

    void update(const SpotLight& light);
    void copyFrom(const SpotShadowMap& source);
    void bindForWriting() const;
    void bind(unsigned int slot) const;
    [[nodiscard]] const glm::mat4& getLightSpaceMatrix() const;

private:
    unsigned int framebuffer = 0;
    unsigned int depthTexture = 0;
    int resolution;
    glm::mat4 lightSpaceMatrix{1.0f};
};

} // namespace wgfx
