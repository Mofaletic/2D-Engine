//modify at 28-03-2026 by Lancelot
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct GameObject {
    int id;
    std::string name;
    float position[2];
    float scale[2];
    std::uint64_t textureResourceId = 0;
    std::string texturePath;
};

struct SceneState {
    std::vector<GameObject> objects;
};
