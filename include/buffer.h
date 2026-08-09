#pragma once

#include <glad/glad.h>

#include <cstddef>
#include <stdexcept>

namespace wgfx {

enum class BufferType {
    VertexBuffer,
    IndexBuffer,
    UniformBuffer
};

class Buffer {
public:
    Buffer(BufferType type, const void* data, std::size_t size);
    Buffer(BufferType type, std::size_t size);
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;
    ~Buffer();

    [[nodiscard]] BufferType getType() const;
    void bind() const;
    void unbind() const;
    void bindBase(unsigned int bindingPoint) const;
    void setData(const void* data, std::size_t size);
    void updateData(const void* data, std::size_t size, std::size_t offset = 0);

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
private:
    GLenum getTarget() const;

    std::size_t size;
    unsigned int bufferId;
    BufferType type;
};

} // namespace wgfx
