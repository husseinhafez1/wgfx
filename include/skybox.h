#pragma once

#include <glm/glm.hpp>

#include <array>
#include <string>

#include "shader.h"

namespace wgfx {

class Skybox {
public:
    // Loads a single 4x3 horizontal-cross cubemap image.
    explicit Skybox(const std::string& cubemapAtlas);
    // Face order: +X, -X, +Y, -Y, +Z, -Z.
    explicit Skybox(const std::array<std::string, 6>& faces);
    ~Skybox();

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    void draw(const glm::mat4& view, const glm::mat4& projection);
    void bind(unsigned int slot) const;

private:
    void loadAtlas(const std::string& cubemapAtlas);
    void loadFaces(const std::array<std::string, 6>& faces);
    void configureCubemap() const;
    void createGeometry();

    Shader shader;
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int cubemap = 0;
};

} // namespace wgfx
