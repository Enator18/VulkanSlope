#pragma once
#include <optional>
#include <vector>
#include <memory>
#include <filesystem>

struct GeoSurface
{
    uint32_t startIndex;
    uint32_t count;
};

struct MeshAsset
{
    std::string name;

    std::vector<GeoSurface> surfaces;
};

std::vector<char> readFile(const std::string& fileName);
std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadModel(std::filesystem::path filePath);