#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

#include "buffer.h"

class Shader;
class Texture;

class Model {
public:
    Model(const std::string& path);
    ~Model();

    void draw(Shader& shader) const;
private:
    struct Material {
        glm::vec4 baseColor{1.0f};
        std::shared_ptr<Texture> baseColorTexture;
        std::size_t uvSet = 0;
        glm::vec2 uvOffset{0.0f};
        glm::vec2 uvScale{1.0f};
        float uvRotation = 0.0f;
    };

    struct Submesh {
        std::size_t indexOffset;
        std::size_t indexCount;
        std::size_t materialIndex;
    };

    struct MeshData;

    explicit Model(MeshData&& meshData);
    static MeshData loadModel(const std::string& path);
    static MeshData loadObj(const std::string& path);
    static MeshData loadGltf(const std::string& path);
    static void calculateNormals(MeshData& meshData);

    unsigned int vao;
    Buffer vbo;
    Buffer normalVbo;
    Buffer uvVbo;
    Buffer ebo;
    std::size_t indexCount;
    std::vector<Material> materials;
    std::vector<Submesh> submeshes;
};
