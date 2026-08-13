#include "buffer.h"

#include <stdexcept>
#include <utility>

namespace wgfx {

VAO::VAO() {
    glGenVertexArrays(1, &id);
}

VAO::~VAO() {
    if (id != 0) {
        glDeleteVertexArrays(1, &id);
    }
}

VAO::VAO(VAO&& other) noexcept : id(std::exchange(other.id, 0)) {}

VAO& VAO::operator=(VAO&& other) noexcept {
    if (this != &other) {
        if (id != 0) {
            glDeleteVertexArrays(1, &id);
        }
        id = std::exchange(other.id, 0);
    }
    return *this;
}

void VAO::bind() const {
    glBindVertexArray(id);
}

void VAO::unbind() {
    glBindVertexArray(0);
}

void VAO::linkVBO(VBO& vbo, unsigned int layout) const {
    linkVBO(vbo, layout, 3);
}

void VAO::linkVBO(
    VBO& vbo,
    unsigned int layout,
    int componentCount,
    GLsizei stride,
    std::size_t offset
) const {
    bind();
    vbo.bind();
    glVertexAttribPointer(
        layout,
        componentCount,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<const void*>(offset)
    );
    glEnableVertexAttribArray(layout);
    VBO::unbind();
}

VBO::VBO(const void* data, std::size_t size) {
    glGenBuffers(1, &id);
    setData(data, size);
}

VBO::VBO(std::size_t size) : VBO(nullptr, size) {}

VBO::~VBO() {
    if (id != 0) {
        glDeleteBuffers(1, &id);
    }
}

VBO::VBO(VBO&& other) noexcept
    : size(std::exchange(other.size, 0)), id(std::exchange(other.id, 0)) {}

VBO& VBO::operator=(VBO&& other) noexcept {
    if (this != &other) {
        if (id != 0) {
            glDeleteBuffers(1, &id);
        }
        size = std::exchange(other.size, 0);
        id = std::exchange(other.id, 0);
    }
    return *this;
}

void VBO::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, id);
}

void VBO::unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VBO::setData(const void* data, std::size_t size) {
    this->size = size;
    GLint previousBuffer = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);
    bind();
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<unsigned int>(previousBuffer));
}

void VBO::updateData(const void* data, std::size_t size, std::size_t offset) {
    if (offset > this->size || size > this->size - offset) {
        throw std::runtime_error("VBO update exceeds allocated storage.");
    }
    GLint previousBuffer = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);
    bind();
    glBufferSubData(
        GL_ARRAY_BUFFER,
        static_cast<GLintptr>(offset),
        static_cast<GLsizeiptr>(size),
        data
    );
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<unsigned int>(previousBuffer));
}

EBO::EBO(const void* data, std::size_t size) {
    glGenBuffers(1, &id);
    setData(data, size);
}

EBO::EBO(std::size_t size) : EBO(nullptr, size) {}

EBO::~EBO() {
    if (id != 0) {
        glDeleteBuffers(1, &id);
    }
}

EBO::EBO(EBO&& other) noexcept
    : size(std::exchange(other.size, 0)), id(std::exchange(other.id, 0)) {}

EBO& EBO::operator=(EBO&& other) noexcept {
    if (this != &other) {
        if (id != 0) {
            glDeleteBuffers(1, &id);
        }
        size = std::exchange(other.size, 0);
        id = std::exchange(other.id, 0);
    }
    return *this;
}

void EBO::bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
}

void EBO::unbind() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void EBO::setData(const void* data, std::size_t size) {
    this->size = size;
    GLint previousVertexArray = 0;
    GLint previousBuffer = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &previousBuffer);
    glBindVertexArray(0);
    bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(static_cast<unsigned int>(previousVertexArray));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<unsigned int>(previousBuffer));
}

void EBO::updateData(const void* data, std::size_t size, std::size_t offset) {
    if (offset > this->size || size > this->size - offset) {
        throw std::runtime_error("EBO update exceeds allocated storage.");
    }
    GLint previousVertexArray = 0;
    GLint previousBuffer = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &previousBuffer);
    glBindVertexArray(0);
    bind();
    glBufferSubData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLintptr>(offset),
        static_cast<GLsizeiptr>(size),
        data
    );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(static_cast<unsigned int>(previousVertexArray));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<unsigned int>(previousBuffer));
}

} // namespace wgfx
