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

namespace ProjectManager {

bool CreateProject(const std::string& projectName, const std::string& parentDirectory, ProjectDescriptor& outProject, std::string& outError);
bool LoadProject(const std::string& projectDirectory, ProjectDescriptor& outProject, std::string& outError);

}
