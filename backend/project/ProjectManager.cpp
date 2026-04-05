#include "ProjectManager.h"

#include "../resource/ResourcePathUtils.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {

std::string Normalize(const fs::path& path) {
    return ResourcePathUtils::NormalizePathString(path);
}

bool BuildProjectDescriptor(const fs::path& projectRoot, ProjectDescriptor& outProject, std::string& outError) {
    const fs::path projectFilePath = projectRoot / "project.json";
    std::ifstream file(projectFilePath);
    if (!file.is_open()) {
        outError = "Project file not found: " + Normalize(projectFilePath);
        return false;
    }

    json data;
    file >> data;

    outProject.name = data.value("name", projectRoot.filename().string());
    outProject.rootPath = Normalize(projectRoot);
    outProject.projectFilePath = Normalize(projectFilePath);
    outProject.assetRootPath = Normalize(projectRoot / data.value("assetRoot", std::string("Assets")));
    outProject.assetManifestPath = Normalize(projectRoot / data.value("assetManifest", std::string("Settings/asset_registry.json")));
    outProject.defaultScenePath = Normalize(projectRoot / data.value("defaultScene", std::string("Scenes/Main.scene.json")));
    outError.clear();
    return true;
}

bool WriteProjectFile(const fs::path& projectRoot, const std::string& projectName, std::string& outError) {
    json data;
    data["name"] = projectName;
    data["assetRoot"] = "Assets";
    data["assetManifest"] = "Settings/asset_registry.json";
    data["defaultScene"] = "Scenes/Main.scene.json";

    std::ofstream file(projectRoot / "project.json");
    if (!file.is_open()) {
        outError = "Failed to write project.json";
        return false;
    }

    file << data.dump(4);
    outError.clear();
    return true;
}

}

namespace ProjectManager {

bool CreateProject(const std::string& projectName, const std::string& parentDirectory, ProjectDescriptor& outProject, std::string& outError) {
    if (projectName.empty()) {
        outError = "Project name cannot be empty.";
        return false;
    }

    if (parentDirectory.empty()) {
        outError = "Project directory cannot be empty.";
        return false;
    }

    std::error_code ec;
    const fs::path parentPath = ResourcePathUtils::Utf8ToPath(parentDirectory);
    if (!fs::exists(parentPath, ec) || !fs::is_directory(parentPath, ec)) {
        outError = "Parent directory does not exist: " + Normalize(parentPath);
        return false;
    }

    const fs::path projectRoot = parentPath / ResourcePathUtils::Utf8ToPath(projectName);
    if (fs::exists(projectRoot, ec)) {
        const bool hasEntries = fs::directory_iterator(projectRoot, ec) != fs::directory_iterator();
        if (!ec && hasEntries) {
            outError = "Project folder already exists and is not empty: " + Normalize(projectRoot);
            return false;
        }
    }

    fs::create_directories(projectRoot / "Assets", ec);
    if (ec) {
        outError = "Failed to create project asset folder: " + Normalize(projectRoot / "Assets");
        return false;
    }

    fs::create_directories(projectRoot / "Scenes", ec);
    if (ec) {
        outError = "Failed to create project scene folder: " + Normalize(projectRoot / "Scenes");
        return false;
    }

    fs::create_directories(projectRoot / "Settings", ec);
    if (ec) {
        outError = "Failed to create project settings folder: " + Normalize(projectRoot / "Settings");
        return false;
    }

    if (!WriteProjectFile(projectRoot, projectName, outError)) {
        return false;
    }

    return BuildProjectDescriptor(projectRoot, outProject, outError);
}

bool LoadProject(const std::string& projectDirectory, ProjectDescriptor& outProject, std::string& outError) {
    if (projectDirectory.empty()) {
        outError = "Project directory cannot be empty.";
        return false;
    }

    const fs::path projectRoot = ResourcePathUtils::Utf8ToPath(projectDirectory);
    std::error_code ec;
    if (!fs::exists(projectRoot, ec) || !fs::is_directory(projectRoot, ec)) {
        outError = "Project folder does not exist: " + Normalize(projectRoot);
        return false;
    }

    return BuildProjectDescriptor(projectRoot, outProject, outError);
}

}
