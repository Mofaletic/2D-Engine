#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct GameObject {
    int id = 0;
    std::string name;
    float position[2] = { 0.0f, 0.0f };
    float scale[2] = { 1.0f, 1.0f };
    float rotation = 0.0f;
    std::uint64_t textureResourceId = 0;
    std::string texturePath;
    std::uint64_t scriptResourceId = 0;
    std::string scriptPath;
};

struct SceneState {
    std::vector<GameObject> objects;
};
