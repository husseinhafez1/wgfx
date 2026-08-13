#pragma once

#include <glad/glad.h>

namespace wgfx {

class Framebuffer {
public:
    Framebuffer(int width, int height);
    ~Framebuffer();
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void bind(bool msaaEnabled) const;
    static void unbind();
    void resolve(bool msaaEnabled) const;
    void bindColorTexture(unsigned int slot = 0) const;
    void bindRenderbuffer() const;
    static void unbindRenderbuffer();

private:
    unsigned int id = 0;
    unsigned int postProcessingId = 0;
    unsigned int renderbufferId = 0;
    unsigned int postProcessingRenderbufferId = 0;
    unsigned int colorTextureId = 0;
    unsigned int postProcessingTextureId = 0;
    int width = 0;
    int height = 0;
};

} // namespace wgfx
