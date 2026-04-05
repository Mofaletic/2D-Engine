#pragma once

#include "../../backend/resource/AssetRegistry.h"
#include <string>

enum class EditorMode {
    Edit,
    Play,
    Pause
};

enum class EditorCommand {
    None,
    Play,
    Pause,
    Stop
};

enum class ProjectCommand {
    None,
    Create,
    Open,
    Sync
};

struct EditorState {
    int selectedObjectIndex = -1;
    EditorMode mode = EditorMode::Edit;
    EditorCommand pendingCommand = EditorCommand::None;
    ProjectCommand pendingProjectCommand = ProjectCommand::None;
    float sceneViewportWidth = 0.0f;
    float sceneViewportHeight = 0.0f;
    AssetRegistry assetRegistry;
    std::string assetStatus = "No assets imported";
    std::string projectName;
    std::string projectRootPath;
    std::string projectFilePath;
    std::string assetManifestPath;
    std::string sceneFilePath;
    std::string projectStatus = "No project loaded";
    std::string pendingProjectName;
    std::string pendingProjectDirectory;
};
