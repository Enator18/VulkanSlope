#pragma once
std::vector<char> readFile(const std::string& filename);
bool loadModel(tinygltf::Model& model, const char* filename);