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
    Scene,
    Script
};

namespace ProjectManager {

bool CreateProject(const std::string& projectName, const std::string& parentDirectory, ProjectDescriptor& outProject, std::string& outError);
bool LoadProject(const std::string& projectDirectory, ProjectDescriptor& outProject, std::string& outError);
bool CreateProjectItem(const ProjectDescriptor& project, ProjectItemType type, const std::string& itemName, std::string& outCreatedPath, std::string& outError);
bool CreateProjectItemInDirectory(const ProjectDescriptor& project, const std::string& targetDirectory, ProjectItemType type, const std::string& itemName, std::string& outCreatedPath, std::string& outError);
bool DeleteProjectEntry(const ProjectDescriptor& project, const std::string& targetPath, std::string& outError);
bool RenameProjectEntry(const ProjectDescriptor& project, const std::string& targetPath, const std::string& newName, std::string& outRenamedPath, std::string& outError);
bool MoveProjectEntry(const ProjectDescriptor& project, const std::string& sourcePath, const std::string& targetDirectory, std::string& outMovedPath, std::string& outError);
bool IsPathInsideProject(const ProjectDescriptor& project, const std::string& targetPath);

}
