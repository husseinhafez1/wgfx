#include "buffer.h"

Buffer::Buffer(BufferType type, const void* data, std::size_t size)
    : size(0), bufferId(0), type(type) {
    glGenBuffers(1, &bufferId);
    setData(data, size);
}

Buffer::Buffer(BufferType type, std::size_t size)
    : Buffer(type, nullptr, size) {
}

Buffer::Buffer(Buffer&& other) noexcept
    : size(other.size), bufferId(other.bufferId), type(other.type) {
    other.size = 0;
    other.bufferId = 0;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        if (bufferId != 0) {
            glDeleteBuffers(1, &bufferId);
        }

        size = other.size;
        bufferId = other.bufferId;
        type = other.type;

        other.size = 0;
        other.bufferId = 0;
    }
    return *this;
}

Buffer::~Buffer() {
    if (bufferId != 0) {
        glDeleteBuffers(1, &bufferId);
    }
}

BufferType Buffer::getType() const {
    return type;
}

GLenum Buffer::getTarget() const {
    switch (type) {
        case BufferType::VertexBuffer:
            return GL_ARRAY_BUFFER;
        case BufferType::IndexBuffer:
            return GL_ELEMENT_ARRAY_BUFFER;
        case BufferType::UniformBuffer:
            return GL_UNIFORM_BUFFER;
    }

    throw std::logic_error("Unknown buffer type.");
}

void Buffer::bind() const {
    glBindBuffer(getTarget(), bufferId);
}

void Buffer::unbind() const {
    glBindBuffer(getTarget(), 0);
}

void Buffer::bindBase(unsigned int bindingPoint) const {
    if (type != BufferType::UniformBuffer) {
        throw std::logic_error("Only uniform buffers can be bound to a binding point.");
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, bufferId);
}

void Buffer::setData(const void* data, std::size_t size) {
    this->size = size;
    bind();
    glBufferData(getTarget(), static_cast<GLsizeiptr>(size), data, GL_STATIC_DRAW);
}

void Buffer::updateData(const void* data, std::size_t size, std::size_t offset) {
    if (offset > this->size || size > this->size - offset) {
        throw std::runtime_error("Buffer overflow: trying to update more data than the buffer can hold.");
    }

    bind();
    glBufferSubData(
        getTarget(),
        static_cast<GLintptr>(offset),
        static_cast<GLsizeiptr>(size),
        data
    );
}
