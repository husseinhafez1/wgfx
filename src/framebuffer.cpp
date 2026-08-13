#include "framebuffer.h"

#include <stdexcept>
#include <utility>

namespace wgfx {

Framebuffer::Framebuffer(int width, int height) {
    glGenFramebuffers(1, &id);
    bind();

    glGenTextures(1, &colorTextureId);
    glBindTexture(GL_TEXTURE_2D, colorTextureId);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        width,
        height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        colorTextureId,
        0
    );

    glGenRenderbuffers(1, &renderbufferId);
    bindRenderbuffer();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        renderbufferId
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Failed to create a complete OpenGL framebuffer.");
    }
    unbindRenderbuffer();
    unbind();
}

Framebuffer::~Framebuffer() {
    if (colorTextureId != 0) {
        glDeleteTextures(1, &colorTextureId);
    }
    if (renderbufferId != 0) {
        glDeleteRenderbuffers(1, &renderbufferId);
    }
    if (id != 0) {
        glDeleteFramebuffers(1, &id);
    }
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : id(std::exchange(other.id, 0)),
      renderbufferId(std::exchange(other.renderbufferId, 0)),
      colorTextureId(std::exchange(other.colorTextureId, 0)) {}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
    if (this != &other) {
        if (id != 0) {
            glDeleteFramebuffers(1, &id);
        }
        if (renderbufferId != 0) {
            glDeleteRenderbuffers(1, &renderbufferId);
        }
        if (colorTextureId != 0) {
            glDeleteTextures(1, &colorTextureId);
        }
        id = std::exchange(other.id, 0);
        renderbufferId = std::exchange(other.renderbufferId, 0);
        colorTextureId = std::exchange(other.colorTextureId, 0);
    }
    return *this;
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, id);
}

void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::bindColorTexture(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, colorTextureId);
}

void Framebuffer::bindRenderbuffer() const {
    glBindRenderbuffer(GL_RENDERBUFFER, renderbufferId);
}

void Framebuffer::unbindRenderbuffer() {
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

} // namespace wgfx
