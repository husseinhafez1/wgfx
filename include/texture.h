#pragma once

#include <glad/glad.h>

#include <cstddef>
#include <string>

namespace wgfx {

class Texture {
public:
    Texture(const std::string& path, bool srgb = true);
    Texture(const unsigned char* encodedData, std::size_t size, bool srgb = true);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void bind(unsigned int slot = 0) const;
    void unbind() const;

private:
    void upload(const unsigned char* pixels, int width, int height, bool srgb);

    unsigned int m_textureID = 0;
};

} // namespace wgfx
