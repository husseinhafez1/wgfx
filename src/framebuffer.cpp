#include "framebuffer.h"

#include <stdexcept>
#include <utility>

namespace wgfx {

Framebuffer::Framebuffer(int width, int height) : width(width), height(height) {
    glGenFramebuffers(1, &id);
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    glGenTextures(1, &colorTextureId);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorTextureId);
    glTexImage2DMultisample(
        GL_TEXTURE_2D_MULTISAMPLE,
        8,
        GL_RGBA16F,
        width,
        height,
        GL_TRUE
    );
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D_MULTISAMPLE,
        colorTextureId,
        0
    );

    glGenRenderbuffers(1, &renderbufferId);
    bindRenderbuffer();
    glRenderbufferStorageMultisample(
        GL_RENDERBUFFER,
        8,
        GL_DEPTH24_STENCIL8,
        width,
        height
    );
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        renderbufferId
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Failed to create a complete OpenGL framebuffer.");
    }

    glGenFramebuffers(1, &postProcessingId);
    glBindFramebuffer(GL_FRAMEBUFFER, postProcessingId);
    glGenTextures(1, &postProcessingTextureId);
    glBindTexture(GL_TEXTURE_2D, postProcessingTextureId);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA16F,
        width,
        height,
        0,
        GL_RGBA,
        GL_FLOAT,
        nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        postProcessingTextureId,
        0
    );
    glGenRenderbuffers(1, &postProcessingRenderbufferId);
    glBindRenderbuffer(GL_RENDERBUFFER, postProcessingRenderbufferId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        postProcessingRenderbufferId
    );
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Failed to create a complete OpenGL post-processing framebuffer.");
    }

    glGenFramebuffers(1, &brightId);
    glBindFramebuffer(GL_FRAMEBUFFER, brightId);
    glGenTextures(1, &brightTextureId);
    glBindTexture(GL_TEXTURE_2D, brightTextureId);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brightTextureId, 0
    );
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Failed to create the bloom extraction framebuffer.");
    }

    glGenFramebuffers(2, bloomIds);
    glGenTextures(2, bloomTextureIds);
    for (unsigned int index = 0; index < 2; ++index) {
        glBindFramebuffer(GL_FRAMEBUFFER, bloomIds[index]);
        glBindTexture(GL_TEXTURE_2D, bloomTextureIds[index]);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomTextureIds[index], 0
        );
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("Failed to create a bloom blur framebuffer.");
        }
    }
    unbindRenderbuffer();
    unbind();
}

Framebuffer::~Framebuffer() {
    glDeleteTextures(2, bloomTextureIds);
    if (brightTextureId != 0) {
        glDeleteTextures(1, &brightTextureId);
    }
    if (postProcessingTextureId != 0) {
        glDeleteTextures(1, &postProcessingTextureId);
    }
    if (colorTextureId != 0) {
        glDeleteTextures(1, &colorTextureId);
    }
    if (renderbufferId != 0) {
        glDeleteRenderbuffers(1, &renderbufferId);
    }
    if (postProcessingRenderbufferId != 0) {
        glDeleteRenderbuffers(1, &postProcessingRenderbufferId);
    }
    if (id != 0) {
        glDeleteFramebuffers(1, &id);
    }
    if (postProcessingId != 0) {
        glDeleteFramebuffers(1, &postProcessingId);
    }
    glDeleteFramebuffers(2, bloomIds);
    if (brightId != 0) {
        glDeleteFramebuffers(1, &brightId);
    }
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : id(std::exchange(other.id, 0)),
      postProcessingId(std::exchange(other.postProcessingId, 0)),
      renderbufferId(std::exchange(other.renderbufferId, 0)),
      postProcessingRenderbufferId(std::exchange(other.postProcessingRenderbufferId, 0)),
      colorTextureId(std::exchange(other.colorTextureId, 0)),
      postProcessingTextureId(std::exchange(other.postProcessingTextureId, 0)),
      brightId(std::exchange(other.brightId, 0)),
      brightTextureId(std::exchange(other.brightTextureId, 0)),
      width(std::exchange(other.width, 0)),
      height(std::exchange(other.height, 0)) {
    for (unsigned int index = 0; index < 2; ++index) {
        bloomIds[index] = std::exchange(other.bloomIds[index], 0);
        bloomTextureIds[index] = std::exchange(other.bloomTextureIds[index], 0);
    }
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
    if (this != &other) {
        if (id != 0) {
            glDeleteFramebuffers(1, &id);
        }
        if (postProcessingId != 0) {
            glDeleteFramebuffers(1, &postProcessingId);
        }
        if (renderbufferId != 0) {
            glDeleteRenderbuffers(1, &renderbufferId);
        }
        if (postProcessingRenderbufferId != 0) {
            glDeleteRenderbuffers(1, &postProcessingRenderbufferId);
        }
        if (colorTextureId != 0) {
            glDeleteTextures(1, &colorTextureId);
        }
        if (postProcessingTextureId != 0) {
            glDeleteTextures(1, &postProcessingTextureId);
        }
        glDeleteTextures(2, bloomTextureIds);
        if (brightTextureId != 0) {
            glDeleteTextures(1, &brightTextureId);
        }
        glDeleteFramebuffers(2, bloomIds);
        if (brightId != 0) {
            glDeleteFramebuffers(1, &brightId);
        }
        id = std::exchange(other.id, 0);
        postProcessingId = std::exchange(other.postProcessingId, 0);
        renderbufferId = std::exchange(other.renderbufferId, 0);
        postProcessingRenderbufferId = std::exchange(other.postProcessingRenderbufferId, 0);
        colorTextureId = std::exchange(other.colorTextureId, 0);
        postProcessingTextureId = std::exchange(other.postProcessingTextureId, 0);
        brightId = std::exchange(other.brightId, 0);
        brightTextureId = std::exchange(other.brightTextureId, 0);
        for (unsigned int index = 0; index < 2; ++index) {
            bloomIds[index] = std::exchange(other.bloomIds[index], 0);
            bloomTextureIds[index] = std::exchange(other.bloomTextureIds[index], 0);
        }
        width = std::exchange(other.width, 0);
        height = std::exchange(other.height, 0);
    }
    return *this;
}

void Framebuffer::bind(bool msaaEnabled) const {
    glBindFramebuffer(GL_FRAMEBUFFER, msaaEnabled ? id : postProcessingId);
}

void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resolve(bool msaaEnabled) const {
    if (!msaaEnabled) {
        return;
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, postProcessingId);
    glBlitFramebuffer(
        0,
        0,
        width,
        height,
        0,
        0,
        width,
        height,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );
}

void Framebuffer::bindColorTexture(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, postProcessingTextureId);
}

void Framebuffer::bindBloomExtraction() const {
    glBindFramebuffer(GL_FRAMEBUFFER, brightId);
}

void Framebuffer::bindBloomBlur(unsigned int index) const {
    glBindFramebuffer(GL_FRAMEBUFFER, bloomIds[index]);
}

void Framebuffer::bindBrightTexture(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, brightTextureId);
}

void Framebuffer::bindBloomTexture(unsigned int index, unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, bloomTextureIds[index]);
}

void Framebuffer::bindRenderbuffer() const {
    glBindRenderbuffer(GL_RENDERBUFFER, renderbufferId);
}

void Framebuffer::unbindRenderbuffer() {
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

} // namespace wgfx
