#include "model.h"

#include "shader.h"
#include "texture.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <glm/glm.hpp>
#include <tiny_obj_loader.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace wgfx {

namespace {
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
    std::vector<float> normals;
    std::vector<float> uvs;
    std::vector<unsigned int> indices;
    std::vector<Material> materials;
    std::vector<Submesh> submeshes;
    bool hasCompleteNormals = true;
};

Model::Model(const std::string& path)
    : Model(loadModel(path)) {
}

Model::Model(MeshData&& meshData)
    : vbo(meshData.vertices.data(), meshData.vertices.size() * sizeof(float)),
      normalVbo(meshData.normals.data(), meshData.normals.size() * sizeof(float)),
      uvVbo(meshData.uvs.data(), meshData.uvs.size() * sizeof(float)),
      ebo(meshData.indices.data(), meshData.indices.size() * sizeof(unsigned int)),
      indexCount(meshData.indices.size()),
      materials(std::move(meshData.materials)),
      submeshes(std::move(meshData.submeshes)) {
    vao.linkVBO(vbo, 0);
    vao.linkVBO(normalVbo, 1);
    vao.linkVBO(uvVbo, 2, 2);
    vao.bind();
    ebo.bind();
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
    const std::string materialDirectory = fullPath.parent_path().string() + "/";
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> objMaterials;
    std::string warning;
    std::string error;

    if (!tinyobj::LoadObj(
            &attributes,
            &shapes,
            &objMaterials,
            &warning,
            &error,
            fullPath.string().c_str(),
            materialDirectory.c_str(),
            true
        )) {
        throw std::runtime_error("Failed to load model '" + fullPath.string() + "': " + warning + error);
    }

    if (!warning.empty()) {
        std::cerr << "Model warning for '" << fullPath.string() << "': " << warning << '\n';
    }

    MeshData meshData;
    meshData.materials.emplace_back();
    std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache;
    for (const tinyobj::material_t& objMaterial : objMaterials) {
        Material material;
        material.baseColor = glm::vec4(
            objMaterial.diffuse[0],
            objMaterial.diffuse[1],
            objMaterial.diffuse[2],
            objMaterial.dissolve
        );

        if (!objMaterial.diffuse_texname.empty()) {
            const std::filesystem::path texturePath =
                (fullPath.parent_path() / objMaterial.diffuse_texname).lexically_normal();
            const std::string textureKey = texturePath.string();
            const auto existing = textureCache.find(textureKey);
            if (existing != textureCache.end()) {
                material.baseColorTexture = existing->second;
            } else {
                try {
                    material.baseColorTexture = std::make_shared<Texture>(textureKey);
                    textureCache.emplace(textureKey, material.baseColorTexture);
                } catch (const std::exception& exception) {
                    std::cerr << exception.what() << '\n';
                }
            }
        }

        meshData.materials.push_back(std::move(material));
    }

    for (const tinyobj::shape_t& shape : shapes) {
        std::size_t shapeIndexOffset = 0;
        for (std::size_t face = 0; face < shape.mesh.num_face_vertices.size(); ++face) {
            const std::size_t faceVertexCount = shape.mesh.num_face_vertices[face];
            if (faceVertexCount != 3) {
                throw std::runtime_error("Model '" + fullPath.string() + "' contains a non-triangle face.");
            }

            const int objMaterialIndex = face < shape.mesh.material_ids.size()
                ? shape.mesh.material_ids[face]
                : -1;
            const std::size_t materialIndex = objMaterialIndex >= 0
                && static_cast<std::size_t>(objMaterialIndex) < objMaterials.size()
                ? static_cast<std::size_t>(objMaterialIndex) + 1
                : 0;

            if (meshData.submeshes.empty() || meshData.submeshes.back().materialIndex != materialIndex) {
                meshData.submeshes.push_back({meshData.indices.size(), 0, materialIndex});
            }

            for (std::size_t vertex = 0; vertex < faceVertexCount; ++vertex) {
                const tinyobj::index_t& index = shape.mesh.indices[shapeIndexOffset + vertex];
                if (index.vertex_index < 0) {
                    throw std::runtime_error("Model '" + fullPath.string() + "' contains an invalid vertex index.");
                }

                const std::size_t vertexOffset = static_cast<std::size_t>(index.vertex_index) * 3;
                meshData.vertices.push_back(attributes.vertices[vertexOffset]);
                meshData.vertices.push_back(attributes.vertices[vertexOffset + 1]);
                meshData.vertices.push_back(attributes.vertices[vertexOffset + 2]);

                if (index.normal_index >= 0) {
                    const std::size_t normalOffset = static_cast<std::size_t>(index.normal_index) * 3;
                    meshData.normals.push_back(attributes.normals[normalOffset]);
                    meshData.normals.push_back(attributes.normals[normalOffset + 1]);
                    meshData.normals.push_back(attributes.normals[normalOffset + 2]);
                } else {
                    meshData.normals.insert(meshData.normals.end(), {0.0f, 0.0f, 0.0f});
                    meshData.hasCompleteNormals = false;
                }

                if (index.texcoord_index >= 0) {
                    const std::size_t uvOffset = static_cast<std::size_t>(index.texcoord_index) * 2;
                    meshData.uvs.push_back(attributes.texcoords[uvOffset]);
                    meshData.uvs.push_back(attributes.texcoords[uvOffset + 1]);
                } else {
                    meshData.uvs.insert(meshData.uvs.end(), {0.0f, 0.0f});
                }

                meshData.indices.push_back(static_cast<unsigned int>(meshData.indices.size()));
                ++meshData.submeshes.back().indexCount;
            }

            shapeIndexOffset += faceVertexCount;
        }
    }

    if (!meshData.hasCompleteNormals) {
        calculateNormals(meshData);
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
        fastgltf::Options::LoadExternalImages |
        fastgltf::Options::GenerateMeshIndices;

    fastgltf::Parser parser(fastgltf::Extensions::KHR_texture_transform);
    auto loadedAsset = parser.loadGltf(gltfFile.get(), fullPath.parent_path(), options);
    if (loadedAsset.error() != fastgltf::Error::None) {
        throw std::runtime_error(
            "Failed to parse glTF model '" + fullPath.string() + "': "
            + std::string(fastgltf::getErrorMessage(loadedAsset.error()))
        );
    }

    fastgltf::Asset asset = std::move(loadedAsset.get());
    std::vector<std::shared_ptr<Texture>> srgbImageTextures(asset.images.size());
    std::vector<std::shared_ptr<Texture>> linearImageTextures(asset.images.size());
    const auto loadImageTexture = [&](std::size_t imageIndex, bool srgb) -> std::shared_ptr<Texture> {
        auto& cachedTexture = srgb
            ? srgbImageTextures[imageIndex]
            : linearImageTextures[imageIndex];
        if (cachedTexture) {
            return cachedTexture;
        }
        try {
            cachedTexture = std::visit(fastgltf::visitor {
                [&](const fastgltf::sources::URI& source) -> std::shared_ptr<Texture> {
                    if (source.fileByteOffset != 0 || !source.uri.isLocalPath()) {
                        return nullptr;
                    }
                    return std::make_shared<Texture>(
                        (fullPath.parent_path() / source.uri.fspath()).string(),
                        srgb
                    );
                },
                [&](const fastgltf::sources::Array& source) -> std::shared_ptr<Texture> {
                    return std::make_shared<Texture>(
                        reinterpret_cast<const unsigned char*>(source.bytes.data()),
                        source.bytes.size(),
                        srgb
                    );
                },
                [&](const fastgltf::sources::Vector& source) -> std::shared_ptr<Texture> {
                    return std::make_shared<Texture>(
                        reinterpret_cast<const unsigned char*>(source.bytes.data()),
                        source.bytes.size(),
                        srgb
                    );
                },
                [&](const fastgltf::sources::ByteView& source) -> std::shared_ptr<Texture> {
                    return std::make_shared<Texture>(
                        reinterpret_cast<const unsigned char*>(source.bytes.data()),
                        source.bytes.size(),
                        srgb
                    );
                },
                [&](const fastgltf::sources::BufferView& source) -> std::shared_ptr<Texture> {
                    const auto bytes = fastgltf::DefaultBufferDataAdapter{}(asset, source.bufferViewIndex);
                    return std::make_shared<Texture>(
                        reinterpret_cast<const unsigned char*>(bytes.data()),
                        bytes.size(),
                        srgb
                    );
                },
                [](const auto&) -> std::shared_ptr<Texture> {
                    return nullptr;
                }
            }, asset.images[imageIndex].data);
        } catch (const std::exception& exception) {
            std::cerr << "Failed to load image " << imageIndex << " from '" << fullPath.string()
                      << "': " << exception.what() << '\n';
        }
        return cachedTexture;
    };

    MeshData meshData;
    meshData.materials.emplace_back();
    for (const fastgltf::Material& gltfMaterial : asset.materials) {
        Material material;
        const auto& factor = gltfMaterial.pbrData.baseColorFactor;
        material.baseColor = glm::vec4(factor.x(), factor.y(), factor.z(), factor.w());
        material.metallic = gltfMaterial.pbrData.metallicFactor;
        material.roughness = gltfMaterial.pbrData.roughnessFactor;

        if (gltfMaterial.pbrData.baseColorTexture.has_value()) {
            const auto& textureInfo = gltfMaterial.pbrData.baseColorTexture.value();
            material.uvSet = textureInfo.texCoordIndex;
            if (textureInfo.transform) {
                material.uvOffset = glm::vec2(
                    textureInfo.transform->uvOffset.x(),
                    textureInfo.transform->uvOffset.y()
                );
                material.uvScale = glm::vec2(
                    textureInfo.transform->uvScale.x(),
                    textureInfo.transform->uvScale.y()
                );
                material.uvRotation = textureInfo.transform->rotation;
                if (textureInfo.transform->texCoordIndex.has_value()) {
                    material.uvSet = textureInfo.transform->texCoordIndex.value();
                }
            }
            if (textureInfo.textureIndex < asset.textures.size()) {
                const fastgltf::Texture& gltfTexture = asset.textures[textureInfo.textureIndex];
                if (gltfTexture.imageIndex.has_value()
                    && gltfTexture.imageIndex.value() < asset.images.size()) {
                    material.baseColorTexture = loadImageTexture(
                        gltfTexture.imageIndex.value(),
                        true
                    );
                }
            }
        }

        if (gltfMaterial.pbrData.metallicRoughnessTexture.has_value()) {
            const auto& textureInfo = gltfMaterial.pbrData.metallicRoughnessTexture.value();
            if (!gltfMaterial.pbrData.baseColorTexture.has_value()) {
                material.uvSet = textureInfo.texCoordIndex;
            }
            if (textureInfo.textureIndex < asset.textures.size()) {
                const fastgltf::Texture& gltfTexture = asset.textures[textureInfo.textureIndex];
                if (gltfTexture.imageIndex.has_value()
                    && gltfTexture.imageIndex.value() < asset.images.size()) {
                    material.metallicRoughnessTexture = loadImageTexture(
                        gltfTexture.imageIndex.value(),
                        false
                    );
                }
            }
        }

        meshData.materials.push_back(std::move(material));
    }

    const auto appendMesh = [&](const fastgltf::Mesh& mesh, const fastgltf::math::fmat4x4& transform) {
        for (const fastgltf::Primitive& primitive : mesh.primitives) {
            if (primitive.type != fastgltf::PrimitiveType::Triangles) {
                throw std::runtime_error("Model '" + fullPath.string() + "' contains a non-triangle primitive.");
            }

            const auto* positionAttribute = primitive.findAttribute("POSITION");
            if (positionAttribute == primitive.attributes.end()) {
                throw std::runtime_error("Model '" + fullPath.string() + "' contains a primitive without positions.");
            }

            const std::size_t materialIndex = primitive.materialIndex.has_value()
                && primitive.materialIndex.value() < asset.materials.size()
                ? primitive.materialIndex.value() + 1
                : 0;
            const std::size_t firstIndex = meshData.indices.size();
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

            const std::size_t vertexCount = positionAccessor.count;
            const auto* normalAttribute = primitive.findAttribute("NORMAL");
            if (normalAttribute != primitive.attributes.end()
                && asset.accessors[normalAttribute->accessorIndex].count == vertexCount) {
                const auto normalTransform = fastgltf::math::transpose(fastgltf::math::inverse(transform));
                const fastgltf::Accessor& normalAccessor = asset.accessors[normalAttribute->accessorIndex];
                fastgltf::iterateAccessor<fastgltf::math::fvec3>(
                    asset,
                    normalAccessor,
                    [&](const fastgltf::math::fvec3& normal) {
                        const auto transformed = normalTransform * fastgltf::math::fvec4(
                            normal.x(), normal.y(), normal.z(), 0.0f
                        );
                        const auto normalized = fastgltf::math::normalize(fastgltf::math::fvec3(
                            transformed.x(), transformed.y(), transformed.z()
                        ));
                        meshData.normals.push_back(normalized.x());
                        meshData.normals.push_back(normalized.y());
                        meshData.normals.push_back(normalized.z());
                    }
                );
            } else {
                meshData.normals.insert(meshData.normals.end(), vertexCount * 3, 0.0f);
                meshData.hasCompleteNormals = false;
            }

            const std::string uvName = "TEXCOORD_" + std::to_string(meshData.materials[materialIndex].uvSet);
            const auto* uvAttribute = primitive.findAttribute(uvName);
            if (uvAttribute != primitive.attributes.end()
                && asset.accessors[uvAttribute->accessorIndex].count == vertexCount) {
                const fastgltf::Accessor& uvAccessor = asset.accessors[uvAttribute->accessorIndex];
                fastgltf::iterateAccessor<fastgltf::math::fvec2>(
                    asset,
                    uvAccessor,
                    [&](const fastgltf::math::fvec2& uv) {
                        const Material& material = meshData.materials[materialIndex];
                        const float cosine = std::cos(material.uvRotation);
                        const float sine = std::sin(material.uvRotation);
                        const float scaledU = uv.x() * material.uvScale.x;
                        const float scaledV = uv.y() * material.uvScale.y;
                        meshData.uvs.push_back(material.uvOffset.x + cosine * scaledU - sine * scaledV);
                        meshData.uvs.push_back(material.uvOffset.y + sine * scaledU + cosine * scaledV);
                    }
                );
            } else {
                meshData.uvs.insert(meshData.uvs.end(), vertexCount * 2, 0.0f);
            }

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
            meshData.submeshes.push_back({firstIndex, meshData.indices.size() - firstIndex, materialIndex});
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
    if (!meshData.hasCompleteNormals) {
        calculateNormals(meshData);
    }
    return meshData;
}

void Model::calculateNormals(MeshData& meshData) {
    meshData.normals.assign(meshData.vertices.size(), 0.0f);
    const std::size_t vertexCount = meshData.vertices.size() / 3;

    for (std::size_t i = 0; i + 2 < meshData.indices.size(); i += 3) {
        const unsigned int index0 = meshData.indices[i];
        const unsigned int index1 = meshData.indices[i + 1];
        const unsigned int index2 = meshData.indices[i + 2];
        if (index0 >= vertexCount || index1 >= vertexCount || index2 >= vertexCount) {
            throw std::runtime_error("Cannot calculate normals for a mesh with invalid indices.");
        }

        const glm::vec3 position0(
            meshData.vertices[index0 * 3],
            meshData.vertices[index0 * 3 + 1],
            meshData.vertices[index0 * 3 + 2]
        );
        const glm::vec3 position1(
            meshData.vertices[index1 * 3],
            meshData.vertices[index1 * 3 + 1],
            meshData.vertices[index1 * 3 + 2]
        );
        const glm::vec3 position2(
            meshData.vertices[index2 * 3],
            meshData.vertices[index2 * 3 + 1],
            meshData.vertices[index2 * 3 + 2]
        );
        const glm::vec3 faceNormal = glm::cross(position1 - position0, position2 - position0);

        for (const unsigned int index : {index0, index1, index2}) {
            meshData.normals[index * 3] += faceNormal.x;
            meshData.normals[index * 3 + 1] += faceNormal.y;
            meshData.normals[index * 3 + 2] += faceNormal.z;
        }
    }

    for (std::size_t i = 0; i < meshData.normals.size(); i += 3) {
        glm::vec3 normal(
            meshData.normals[i],
            meshData.normals[i + 1],
            meshData.normals[i + 2]
        );
        normal = glm::dot(normal, normal) > 0.0f
            ? glm::normalize(normal)
            : glm::vec3(0.0f, 1.0f, 0.0f);
        meshData.normals[i] = normal.x;
        meshData.normals[i + 1] = normal.y;
        meshData.normals[i + 2] = normal.z;
    }
    meshData.hasCompleteNormals = true;
}

Model::~Model() = default;

void Model::draw(Shader& shader) const {
    shader.use();
    shader.setUniform("baseColorTexture", 0);
    shader.setUniform("metallicRoughnessTexture", 2);
    vao.bind();

    for (const Submesh& submesh : submeshes) {
        const Material& material = materials[submesh.materialIndex];
        shader.setUniform("baseColor", material.baseColor);
        shader.setUniform("metallic", material.metallic);
        shader.setUniform("roughness", material.roughness);
        shader.setUniform("hasBaseColorTexture", material.baseColorTexture ? 1 : 0);
        shader.setUniform("hasMetallicRoughnessTexture", material.metallicRoughnessTexture ? 1 : 0);
        if (material.baseColorTexture) {
            material.baseColorTexture->bind(0);
        } else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        if (material.metallicRoughnessTexture) {
            material.metallicRoughnessTexture->bind(2);
        } else {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(submesh.indexCount),
            GL_UNSIGNED_INT,
            reinterpret_cast<const void*>(submesh.indexOffset * sizeof(unsigned int))
        );
    }
}

void Model::drawDepth() const {
    vao.bind();
    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(indexCount),
        GL_UNSIGNED_INT,
        nullptr
    );
}

} // namespace wgfx
