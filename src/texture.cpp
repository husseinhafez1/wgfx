#include "texture.h"

#include <stb_image.h>

#include <limits>
#include <stdexcept>

namespace wgfx {

Texture::Texture(const std::string& path) {
    int width;
    int height;
    int channels;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error(
            "Failed to load texture '" + path + "': " + (reason != nullptr ? reason : "unknown error")
        );
    }

    upload(pixels, width, height);
    stbi_image_free(pixels);
}

Texture::Texture(const unsigned char* encodedData, std::size_t size) {
    if (size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::runtime_error("Encoded texture is too large for stb_image.");
    }

    int width;
    int height;
    int channels;
    unsigned char* pixels = stbi_load_from_memory(
        encodedData,
        static_cast<int>(size),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );
    if (pixels == nullptr) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error(
            std::string("Failed to decode embedded texture: ") + (reason != nullptr ? reason : "unknown error")
        );
    }

    upload(pixels, width, height);
    stbi_image_free(pixels);
}

void Texture::upload(const unsigned char* pixels, int width, int height) {
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::~Texture() {
    glDeleteTextures(1, &m_textureID);
}

void Texture::bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void Texture::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace wgfx
