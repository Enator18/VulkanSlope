#pragma once
#include <optional>
#include <vector>
#include <memory>
#include <filesystem>

#include "mesh.h"
#include "entity.h"
#include "engine_types.h"

struct GeoSurface
{
    uint32_t startIndex;
    uint32_t count;
};

struct MeshAsset
{
    std::string name;

    std::vector<GeoSurface> surfaces;

    Mesh mesh;
};

struct TextureAsset
{
    std::string name;
    int width, height;

    std::vector<uint32_t> data;
};

std::string readFile(std::filesystem::path filePath);
std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadModel(std::filesystem::path filePath);
TextureAsset loadImage(std::filesystem::path filePath, std::string name);
Scene loadScene(std::filesystem::path filePath, std::unordered_map<std::string, MeshAsset>& assets, std::unordered_map<std::string, TextureImage>& textures);