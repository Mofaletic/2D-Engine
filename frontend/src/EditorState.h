#pragma once

#include "../../backend/resource/AssetRegistry.h"

#include <string>
#include <vector>

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

enum class EditorLogLevel {
    Info,
    Warning,
    Error
};

struct EditorLogEntry {
    EditorLogLevel level = EditorLogLevel::Info;
    std::string message;
};

struct EditorState {
    int selectedObjectIndex = -1;
    EditorMode mode = EditorMode::Edit;
    EditorCommand pendingCommand = EditorCommand::None;
    ProjectCommand pendingProjectCommand = ProjectCommand::None;
    float sceneViewportWidth = 0.0f;
    float sceneViewportHeight = 0.0f;
    float sceneViewportScreenX = 0.0f;
    float sceneViewportScreenY = 0.0f;
    float sceneViewportScreenWidth = 0.0f;
    float sceneViewportScreenHeight = 0.0f;
    bool isDraggingSceneObject = false;
    int draggingObjectIndex = -1;
    float sceneDragOffsetX = 0.0f;
    float sceneDragOffsetY = 0.0f;
    AssetRegistry assetRegistry;
    std::string assetStatus = "No assets imported";
    std::string projectName;
    std::string projectRootPath;
    std::string projectFilePath;
    std::string assetManifestPath;
    std::string sceneFilePath;
    std::string projectStatus = "No project loaded";
    std::string scriptStatus;
    std::string pendingProjectName;
    std::string pendingProjectDirectory;
    bool showScene = true;
    bool showHierarchy = true;
    bool showInspector = true;
    bool showProject = true;
    bool showConsole = true;
    bool resetLayoutRequested = false;
    bool consoleShowInfo = true;
    bool consoleShowWarnings = true;
    bool consoleShowErrors = true;
    bool consoleAutoScroll = true;
    std::string hierarchySearch;
    std::string focusAssetPath;
    std::vector<EditorLogEntry> logs;
};

