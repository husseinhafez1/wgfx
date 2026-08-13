#pragma once

#include <glad/glad.h>

#include <cstddef>

namespace wgfx {

class VBO;

class VAO {
public:
    VAO();
    ~VAO();
    VAO(VAO&& other) noexcept;
    VAO& operator=(VAO&& other) noexcept;

    VAO(const VAO&) = delete;
    VAO& operator=(const VAO&) = delete;

    void bind() const;
    static void unbind();
    void linkVBO(VBO& vbo, unsigned int layout) const;
    void linkVBO(
        VBO& vbo,
        unsigned int layout,
        int componentCount,
        GLsizei stride = 0,
        std::size_t offset = 0
    ) const;

private:
    unsigned int id = 0;
};

class VBO {
public:
    VBO(const void* data, std::size_t size);
    explicit VBO(std::size_t size);
    ~VBO();
    VBO(VBO&& other) noexcept;
    VBO& operator=(VBO&& other) noexcept;

    VBO(const VBO&) = delete;
    VBO& operator=(const VBO&) = delete;

    void bind() const;
    static void unbind();
    void setData(const void* data, std::size_t size);
    void updateData(const void* data, std::size_t size, std::size_t offset = 0);

private:
    std::size_t size = 0;
    unsigned int id = 0;
};

class EBO {
public:
    EBO(const void* data, std::size_t size);
    explicit EBO(std::size_t size);
    ~EBO();
    EBO(EBO&& other) noexcept;
    EBO& operator=(EBO&& other) noexcept;

    EBO(const EBO&) = delete;
    EBO& operator=(const EBO&) = delete;

    void bind() const;
    static void unbind();
    void setData(const void* data, std::size_t size);
    void updateData(const void* data, std::size_t size, std::size_t offset = 0);

private:
    std::size_t size = 0;
    unsigned int id = 0;
};

} // namespace wgfx
