#include "ProjectManager.h"

#include "../resource/ResourcePathUtils.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <vector>
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

std::string SanitizeName(std::string name) {
    for (char& ch : name) {
        const bool invalid =
            ch == '<' || ch == '>' || ch == ':' || ch == '"' || ch == '/' ||
            ch == '\\' || ch == '|' || ch == '?' || ch == '*';
        if (invalid) {
            ch = '_';
        }
    }

    while (!name.empty() && (name.back() == '.' || name.back() == ' ')) {
        name.pop_back();
    }

    return name;
}

fs::path BuildUniquePath(const fs::path& directory, const std::string& baseName, const std::string& extension) {
    fs::path target = directory / ResourcePathUtils::Utf8ToPath(baseName + extension);
    if (!fs::exists(target)) {
        return target;
    }

    int suffix = 1;
    while (true) {
        fs::path candidate = directory / ResourcePathUtils::Utf8ToPath(baseName + "_" + std::to_string(suffix) + extension);
        if (!fs::exists(candidate)) {
            return candidate;
        }
        ++suffix;
    }
}

bool WriteTextFile(const fs::path& targetPath, const std::string& content, std::string& outError) {
    std::ofstream file(targetPath);
    if (!file.is_open()) {
        outError = "Failed to create file: " + Normalize(targetPath);
        return false;
    }
    file << content;
    outError.clear();
    return true;
}

bool WriteBmpPlaceholder(const fs::path& targetPath, std::string& outError) {
    const std::vector<unsigned char> bytes = {
        0x42, 0x4D, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x13, 0x0B,
        0x00, 0x00, 0x13, 0x0B, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
        0xFF, 0x00
    };

    std::ofstream file(targetPath, std::ios::binary);
    if (!file.is_open()) {
        outError = "Failed to create image file: " + Normalize(targetPath);
        return false;
    }

    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    outError.clear();
    return true;
}

bool WriteSilentWav(const fs::path& targetPath, std::string& outError) {
    const std::vector<unsigned char> bytes = {
        'R','I','F','F', 0x24,0x00,0x00,0x00, 'W','A','V','E',
        'f','m','t',' ', 0x10,0x00,0x00,0x00, 0x01,0x00,0x01,0x00,
        0x44,0xAC,0x00,0x00, 0x88,0x58,0x01,0x00, 0x02,0x00,0x10,0x00,
        'd','a','t','a', 0x00,0x00,0x00,0x00
    };

    std::ofstream file(targetPath, std::ios::binary);
    if (!file.is_open()) {
        outError = "Failed to create audio file: " + Normalize(targetPath);
        return false;
    }

    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    outError.clear();
    return true;
}

bool EnsureDirectory(const fs::path& path, std::string& outError) {
    std::error_code ec;
    fs::create_directories(path, ec);
    if (ec) {
        outError = "Failed to create directory: " + Normalize(path);
        return false;
    }
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

bool CreateProjectItem(const ProjectDescriptor& project, ProjectItemType type, const std::string& itemName, std::string& outCreatedPath, std::string& outError) {
    const std::string sanitizedName = SanitizeName(itemName.empty() ? "NewItem" : itemName);
    if (sanitizedName.empty()) {
        outError = "Item name cannot be empty.";
        return false;
    }

    fs::path targetDirectory;
    std::string extension;
    bool wroteFile = false;

    switch (type) {
    case ProjectItemType::Audio:
        targetDirectory = ResourcePathUtils::Utf8ToPath(project.assetRootPath) / "Audio";
        extension = ".wav";
        break;
    case ProjectItemType::Image:
        targetDirectory = ResourcePathUtils::Utf8ToPath(project.assetRootPath) / "Images";
        extension = ".bmp";
        break;
    case ProjectItemType::Text:
        targetDirectory = ResourcePathUtils::Utf8ToPath(project.assetRootPath) / "Text";
        extension = ".txt";
        break;
    case ProjectItemType::Scene:
        targetDirectory = ResourcePathUtils::Utf8ToPath(project.assetRootPath) / "Scenes";
        extension = ".scene.json";
        break;
    }

    if (!EnsureDirectory(targetDirectory, outError)) {
        return false;
    }

    const fs::path targetPath = BuildUniquePath(targetDirectory, sanitizedName, extension);
    switch (type) {
    case ProjectItemType::Audio:
        wroteFile = WriteSilentWav(targetPath, outError);
        break;
    case ProjectItemType::Image:
        wroteFile = WriteBmpPlaceholder(targetPath, outError);
        break;
    case ProjectItemType::Text:
        wroteFile = WriteTextFile(targetPath, "New text asset\n", outError);
        break;
    case ProjectItemType::Scene:
        wroteFile = WriteTextFile(targetPath, "{\n    \"sceneName\": \"" + sanitizedName + "\",\n    \"objects\": []\n}\n", outError);
        break;
    }

    if (!wroteFile) {
        return false;
    }

    outCreatedPath = Normalize(targetPath);
    outError.clear();
    return true;
}

}
