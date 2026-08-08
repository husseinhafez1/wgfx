#include "model.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <tiny_obj_loader.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
unsigned int createVertexArray() {
    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    return vao;
}

std::filesystem::path resolveModelPath(const std::string& path) {
    std::filesystem::path resolved(path);
    if (!resolved.is_absolute()) {
        resolved = std::filesystem::path(MODEL_DIR) / resolved;
    }
    return resolved;
}
}

struct Model::MeshData {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

Model::Model(const std::string& path)
    : Model(loadModel(path)) {
}

Model::Model(MeshData&& meshData)
    : vao(createVertexArray()),
      vbo(BufferType::VertexBuffer, meshData.vertices.data(), meshData.vertices.size() * sizeof(float)),
      ebo(BufferType::IndexBuffer, meshData.indices.data(), meshData.indices.size() * sizeof(unsigned int)),
      indexCount(meshData.indices.size()) {
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
}

Model::MeshData Model::loadModel(const std::string& path) {
    std::string extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });

    if (extension == ".obj") {
        return loadObj(path);
    }
    if (extension == ".gltf" || extension == ".glb") {
        return loadGltf(path);
    }

    throw std::runtime_error("Unsupported model format '" + extension + "' for '" + path + "'.");
}

Model::MeshData Model::loadObj(const std::string& path) {
    const std::filesystem::path fullPath = resolveModelPath(path);
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warning;
    std::string error;

    if (!tinyobj::LoadObj(
            &attributes,
            &shapes,
            &materials,
            &warning,
            &error,
            fullPath.string().c_str()
        )) {
        throw std::runtime_error("Failed to load model '" + fullPath.string() + "': " + warning + error);
    }

    if (!warning.empty()) {
        std::cerr << "Model warning for '" << fullPath.string() << "': " << warning << '\n';
    }

    MeshData meshData;
    for (const tinyobj::shape_t& shape : shapes) {
        for (const tinyobj::index_t& index : shape.mesh.indices) {
            if (index.vertex_index < 0) {
                throw std::runtime_error("Model '" + fullPath.string() + "' contains an invalid vertex index.");
            }

            const std::size_t vertexOffset = static_cast<std::size_t>(index.vertex_index) * 3;
            meshData.vertices.push_back(attributes.vertices[vertexOffset]);
            meshData.vertices.push_back(attributes.vertices[vertexOffset + 1]);
            meshData.vertices.push_back(attributes.vertices[vertexOffset + 2]);
            meshData.indices.push_back(static_cast<unsigned int>(meshData.indices.size()));
        }
    }

    return meshData;
}

Model::MeshData Model::loadGltf(const std::string& path) {
    const std::filesystem::path fullPath = resolveModelPath(path);
    auto gltfFile = fastgltf::MappedGltfFile::FromPath(fullPath);
    if (!gltfFile) {
        throw std::runtime_error(
            "Failed to open glTF model '" + fullPath.string() + "': "
            + std::string(fastgltf::getErrorMessage(gltfFile.error()))
        );
    }

    constexpr fastgltf::Options options =
        fastgltf::Options::LoadExternalBuffers |
        fastgltf::Options::GenerateMeshIndices;

    fastgltf::Parser parser;
    auto loadedAsset = parser.loadGltf(gltfFile.get(), fullPath.parent_path(), options);
    if (loadedAsset.error() != fastgltf::Error::None) {
        throw std::runtime_error(
            "Failed to parse glTF model '" + fullPath.string() + "': "
            + std::string(fastgltf::getErrorMessage(loadedAsset.error()))
        );
    }

    fastgltf::Asset asset = std::move(loadedAsset.get());
    MeshData meshData;

    const auto appendMesh = [&](const fastgltf::Mesh& mesh, const fastgltf::math::fmat4x4& transform) {
        for (const fastgltf::Primitive& primitive : mesh.primitives) {
            if (primitive.type != fastgltf::PrimitiveType::Triangles) {
                throw std::runtime_error("Model '" + fullPath.string() + "' contains a non-triangle primitive.");
            }

            const auto* positionAttribute = primitive.findAttribute("POSITION");
            if (positionAttribute == primitive.attributes.end()) {
                throw std::runtime_error("Model '" + fullPath.string() + "' contains a primitive without positions.");
            }

            const std::size_t baseVertex = meshData.vertices.size() / 3;
            const fastgltf::Accessor& positionAccessor = asset.accessors[positionAttribute->accessorIndex];
            fastgltf::iterateAccessor<fastgltf::math::fvec3>(
                asset,
                positionAccessor,
                [&](const fastgltf::math::fvec3& position) {
                    const auto transformed = transform * fastgltf::math::fvec4(
                        position.x(), position.y(), position.z(), 1.0f
                    );
                    meshData.vertices.push_back(transformed.x());
                    meshData.vertices.push_back(transformed.y());
                    meshData.vertices.push_back(transformed.z());
                }
            );

            if (!primitive.indicesAccessor.has_value()) {
                throw std::runtime_error("Model '" + fullPath.string() + "' contains a primitive without indices.");
            }

            const fastgltf::Accessor& indexAccessor = asset.accessors[primitive.indicesAccessor.value()];
            fastgltf::iterateAccessor<std::uint32_t>(
                asset,
                indexAccessor,
                [&](std::uint32_t index) {
                    meshData.indices.push_back(static_cast<unsigned int>(baseVertex + index));
                }
            );
        }
    };

    if (!asset.scenes.empty()) {
        const std::size_t sceneIndex = asset.defaultScene.value_or(0);
        fastgltf::iterateSceneNodes(
            asset,
            sceneIndex,
            fastgltf::math::fmat4x4(),
            [&](fastgltf::Node& node, const fastgltf::math::fmat4x4& transform) {
                if (node.meshIndex.has_value()) {
                    appendMesh(asset.meshes[node.meshIndex.value()], transform);
                }
            }
        );
    } else {
        for (const fastgltf::Mesh& mesh : asset.meshes) {
            appendMesh(mesh, fastgltf::math::fmat4x4());
        }
    }

    if (meshData.vertices.empty() || meshData.indices.empty()) {
        throw std::runtime_error("Model '" + fullPath.string() + "' contains no drawable triangle geometry.");
    }

    return meshData;
}

Model::~Model() {
    glDeleteVertexArrays(1, &vao);
}

void Model::draw() const {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, nullptr);
}
