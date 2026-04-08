#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct GameObject {
    int id;
    std::string name;
    float position[2];
    float scale[2];
    float rotation = 0.0f;
    std::uint64_t textureResourceId = 0;
    std::string texturePath;
    std::uint64_t scriptResourceId = 0;
    std::string scriptPath;
};

struct SceneState {
    std::vector<GameObject> objects;
};
