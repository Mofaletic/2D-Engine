#include "SceneSerializer.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

using json = nlohmann::json;

bool SaveSceneToFile(const SceneState& sceneState, const std::string& sceneName, const std::string& path)
{
    json j;
    j["sceneName"] = sceneName;
    j["objects"] = json::array();

    for (const auto& obj : sceneState.objects) {
        json objJson;
        objJson["id"] = obj.id;
        objJson["name"] = obj.name;
        objJson["position"] = { obj.position[0], obj.position[1] };
        objJson["scale"] = { obj.scale[0], obj.scale[1] };
        objJson["textureResourceId"] = obj.textureResourceId;
        objJson["texturePath"] = obj.texturePath;

        j["objects"].push_back(objJson);
    }

    std::filesystem::path outputPath(path);
    std::error_code ec;
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path(), ec);
    }

    std::ofstream file(outputPath);
    if (!file.is_open()) {
        return false;
    }

    file << j.dump(4);
    return true;
}

bool LoadSceneFromFile(SceneState& sceneState, EditorState& editorState, std::string& sceneName, const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    json j;
    file >> j;

    sceneState.objects.clear();
    sceneName = j.value("sceneName", "UntitledScene");

    if (j.contains("objects") && j["objects"].is_array()) {
        for (const auto& objJson : j["objects"]) {
            GameObject obj;
            obj.id = objJson.value("id", 0);
            obj.name = objJson.value("name", "Unnamed");

            obj.position[0] = 0.0f;
            obj.position[1] = 0.0f;
            if (objJson.contains("position") && objJson["position"].is_array() && objJson["position"].size() >= 2) {
                obj.position[0] = objJson["position"][0].get<float>();
                obj.position[1] = objJson["position"][1].get<float>();
            }

            obj.scale[0] = 1.0f;
            obj.scale[1] = 1.0f;
            if (objJson.contains("scale") && objJson["scale"].is_array() && objJson["scale"].size() >= 2) {
                obj.scale[0] = objJson["scale"][0].get<float>();
                obj.scale[1] = objJson["scale"][1].get<float>();
            }

            obj.textureResourceId = objJson.value("textureResourceId", static_cast<std::uint64_t>(0));
            obj.texturePath = objJson.value("texturePath", "pillar.png");

            sceneState.objects.push_back(obj);
        }
    }

    editorState.selectedObjectIndex = sceneState.objects.empty() ? -1 : 0;
    return true;
}
