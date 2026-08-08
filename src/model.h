#pragma once

#include <string>

#include "buffer.h"

class Model {
public:
    Model(const std::string& path);
    ~Model();

    void draw() const;
private:
    struct MeshData;

    explicit Model(MeshData&& meshData);
    static MeshData loadModel(const std::string& path);
    static MeshData loadObj(const std::string& path);
    static MeshData loadGltf(const std::string& path);

    unsigned int vao;
    Buffer vbo;
    Buffer ebo;
    std::size_t indexCount;
};
