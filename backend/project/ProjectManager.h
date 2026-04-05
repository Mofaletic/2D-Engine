#pragma once

#include <string>

struct ProjectDescriptor {
    std::string name;
    std::string rootPath;
    std::string assetRootPath;
    std::string assetManifestPath;
    std::string defaultScenePath;
    std::string projectFilePath;
};

enum class ProjectItemType {
    Audio,
    Image,
    Text,
    Scene
};

namespace ProjectManager {

bool CreateProject(const std::string& projectName, const std::string& parentDirectory, ProjectDescriptor& outProject, std::string& outError);
bool LoadProject(const std::string& projectDirectory, ProjectDescriptor& outProject, std::string& outError);
bool CreateProjectItem(const ProjectDescriptor& project, ProjectItemType type, const std::string& itemName, std::string& outCreatedPath, std::string& outError);

}
