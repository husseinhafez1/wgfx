#include "model.h"

namespace {
unsigned int createVertexArray() {
    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    return vao;
}
}

Model::Model(const std::string& path)
    : vao(createVertexArray()),
      vbo(BufferType::VertexBuffer, nullptr, 0),
      ebo(BufferType::IndexBuffer, nullptr, 0),
      indexCount(0) {
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warning;
    std::string error;

    const std::string fullPath = std::string(MODEL_DIR) + path;
    if (!tinyobj::LoadObj(
            &attributes,
            &shapes,
            &materials,
            &warning,
            &error,
            fullPath.c_str())) {
        throw std::runtime_error("Failed to load model '" + fullPath + "': " + warning + error);
    }

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            vertices.push_back(attributes.vertices[3 * index.vertex_index + 0]);
            vertices.push_back(attributes.vertices[3 * index.vertex_index + 1]);
            vertices.push_back(attributes.vertices[3 * index.vertex_index + 2]);

            indices.push_back(static_cast<unsigned int>(indices.size()));
        }
    }

    vbo.setData(vertices.data(), vertices.size() * sizeof(float));
    ebo.setData(indices.data(), indices.size() * sizeof(unsigned int));
    vbo.bind();
    ebo.bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    indexCount = indices.size();
}

Model::~Model() {
    glDeleteVertexArrays(1, &vao);
}

void Model::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
}
