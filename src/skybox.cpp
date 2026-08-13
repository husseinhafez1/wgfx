#include "skybox.h"

#include <stb_image.h>

#include <array>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace wgfx {

namespace {
constexpr float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
};

std::filesystem::path resolveFacePath(const std::string& face) {
    std::filesystem::path path(face);
    if (!path.is_absolute()) {
        path = std::filesystem::path(CUBEMAP_DIR) / path;
    }
    return path;
}
}

Skybox::Skybox(const std::string& cubemapAtlas)
    : shader("GL/skybox.vert.glsl", "GL/skybox.frag.glsl") {
    loadAtlas(cubemapAtlas);
    createGeometry();
}

Skybox::Skybox(const std::array<std::string, 6>& faces)
    : shader("GL/skybox.vert.glsl", "GL/skybox.frag.glsl") {
    loadFaces(faces);
    createGeometry();
}

void Skybox::loadAtlas(const std::string& cubemapAtlas) {
    const std::filesystem::path path = resolveFacePath(cubemapAtlas);
    int width;
    int height;
    int channels;
    unsigned char* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error(
            "Failed to load cubemap atlas '" + path.string() + "': "
            + (reason != nullptr ? reason : "unknown error")
        );
    }

    if (width % 4 != 0 || height % 3 != 0 || width / 4 != height / 3) {
        stbi_image_free(pixels);
        throw std::runtime_error(
            "Cubemap atlas '" + path.string() + "' must use a 4x3 horizontal-cross layout."
        );
    }

    const int faceSize = width / 4;
    constexpr int channelCount = 4;
    // Atlas layout: +Y on top, then -X/+Z/+X/-Z, then -Y.
    constexpr std::array<std::array<int, 2>, 6> faceCells = {{
        {{2, 1}}, // +X
        {{0, 1}}, // -X
        {{1, 0}}, // +Y
        {{1, 2}}, // -Y
        {{1, 1}}, // +Z
        {{3, 1}}  // -Z
    }};

    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

    std::vector<unsigned char> facePixels(
        static_cast<std::size_t>(faceSize) * faceSize * channelCount
    );
    for (std::size_t face = 0; face < faceCells.size(); ++face) {
        const int sourceX = faceCells[face][0] * faceSize;
        const int sourceY = faceCells[face][1] * faceSize;
        for (int row = 0; row < faceSize; ++row) {
            const unsigned char* source = pixels
                + (static_cast<std::size_t>(sourceY + row) * width + sourceX) * channelCount;
            unsigned char* destination = facePixels.data()
                + static_cast<std::size_t>(row) * faceSize * channelCount;
            std::memcpy(destination, source, static_cast<std::size_t>(faceSize) * channelCount);
        }

        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<unsigned int>(face),
            0,
            GL_SRGB8_ALPHA8,
            faceSize,
            faceSize,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            facePixels.data()
        );
    }

    stbi_image_free(pixels);
    configureCubemap();
}

void Skybox::loadFaces(const std::array<std::string, 6>& faces) {
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

    for (std::size_t i = 0; i < faces.size(); ++i) {
        const std::filesystem::path path = resolveFacePath(faces[i]);
        int width;
        int height;
        int channels;
        unsigned char* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (pixels == nullptr) {
            const char* reason = stbi_failure_reason();
            glDeleteTextures(1, &cubemap);
            cubemap = 0;
            throw std::runtime_error(
                "Failed to load skybox face '" + path.string() + "': "
                + (reason != nullptr ? reason : "unknown error")
            );
        }

        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<unsigned int>(i),
            0,
            GL_SRGB8_ALPHA8,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixels
        );
        stbi_image_free(pixels);
    }

    configureCubemap();
}

void Skybox::configureCubemap() const {
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Skybox::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
}

void Skybox::createGeometry() {
    vbo.setData(skyboxVertices, sizeof(skyboxVertices));
    vao.linkVBO(vbo, 0);

    shader.use();
    shader.setUniform("skybox", 0);
}

Skybox::~Skybox() {
    glDeleteTextures(1, &cubemap);
}

void Skybox::draw(const glm::mat4& view, const glm::mat4& projection) {
    GLint previousDepthFunction;
    GLboolean previousDepthMask;
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunction);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
    const GLboolean cullingWasEnabled = glIsEnabled(GL_CULL_FACE);

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    shader.use();
    shader.setUniform("view", glm::mat4(glm::mat3(view)));
    shader.setUniform("projection", projection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);

    if (cullingWasEnabled) {
        glEnable(GL_CULL_FACE);
    }
    glDepthMask(previousDepthMask);
    glDepthFunc(previousDepthFunction);
}

} // namespace wgfx
