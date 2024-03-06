#include <vector>
#include <fstream>
#include <filesystem>
#include <optional>
#include <vector>
#include <memory>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "file_io.h"
#include "mesh.h"

std::vector<char> readFile(const std::string& fileName)
{
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadModel(std::filesystem::path filePath)
{
    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf;
    fastgltf::Parser parser{};

    auto load = parser.loadBinaryGLTF(&data, filePath.parent_path());
    if (load)
    {
        gltf = std::move(load.get());
    }
    else
    {
        return {};
    }

    std::vector<std::shared_ptr<MeshAsset>> meshes;

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    for (fastgltf::Mesh& mesh : gltf.meshes)
    {
        MeshAsset asset;
        asset.name = mesh.name;

        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives)
        {
            GeoSurface surface;
            surface.startIndex = (uint32_t)indices.size();
            surface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            size_t initial_vtx = vertices.size();

            // load indexes
            {
                fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
                    [&](std::uint32_t idx)
                    {
                        indices.push_back(idx + initial_vtx);
                    });
            }

            // load vertex positions
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                    [&](glm::vec3 v, size_t index)
                    {
                        Vertex newvtx;
                        newvtx.pos = v;
                        newvtx.normal = { 1, 0, 0 };
                        newvtx.color = glm::vec4{ 1.f };
                        newvtx.texCoord.x = 0;
                        newvtx.texCoord.y = 0;
                        vertices[initial_vtx + index] = newvtx;
                    });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end())
            {

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                    [&](glm::vec3 v, size_t index)
                    {
                        vertices[initial_vtx + index].normal = v;
                    });
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
                    [&](glm::vec2 v, size_t index)
                    {
                        vertices[initial_vtx + index].texCoord.x = v.x;
                        vertices[initial_vtx + index].texCoord.y = v.y;
                    });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end())
            {

                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                    [&](glm::vec4 v, size_t index)
                    {
                        vertices[initial_vtx + index].color = v;
                    });
            }
            asset.surfaces.push_back(surface);
        }

        asset.mesh.vertices = vertices;
        asset.mesh.indices = indices;

        meshes.emplace_back(std::make_shared<MeshAsset>(std::move(asset)));
    }


    return meshes;
}

TextureAsset loadImage(const char* fileName, std::string name)
{
    int width, height, channels;

    stbi_uc* imageData = stbi_load(fileName, &width, &height, &channels, STBI_rgb_alpha);

    std::vector<uint32_t> pixels((width * height * channels) / sizeof(uint32_t));

    memcpy(pixels.data(), imageData, pixels.size() * sizeof(uint32_t));

    TextureAsset asset;

    asset.name = name;

    asset.width = width;
    asset.height = height;

    asset.data = pixels;

    return asset;
}

