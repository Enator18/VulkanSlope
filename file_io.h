#pragma once
#include <optional>
#include <vector>
#include <memory>
#include <filesystem>

#include "mesh.h"
#include "entity.h"

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

std::string readFile(const std::string& filePath);
std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadModel(std::filesystem::path filePath);
TextureAsset loadImage(const char* filePath, std::string name);
std::vector<std::unique_ptr<Entity>> loadScene(std::string filePath);