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

struct EditorState {
    int selectedObjectIndex = -1;
    EditorMode mode = EditorMode::Edit;
    EditorCommand pendingCommand = EditorCommand::None;
    float sceneViewportWidth = 0.0f;
    float sceneViewportHeight = 0.0f;
    AssetRegistry assetRegistry;
    std::string assetStatus = "No assets imported";
};
