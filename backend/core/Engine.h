#pragma once
#include "../window/WindowManager.h"
#include "../render/Renderer2D.h"
#include "../input/InputManager.h"
#include "../resource/ResourceManager.h"
#include "../project/ProjectManager.h"
#include "../core/GameLoop.h"
#include "../core/SceneState.h"
#include "../../frontend/src/EditorState.h"

class Engine {
private:
    WindowManager windowManager;
    Renderer2D renderer2D;
    InputManager inputManager;
    ResourceManager resourceManager;
    SceneState sceneState;
    EditorState editorState;
    GameLoop gameLoop;
    bool running;
    SceneState playModeSceneBackup;
    bool hasPlayModeBackup = false;
    Uint64 lastProjectSyncTick = 0;

public:
    Engine();
    bool init();
    void run();
    void shutdown();
    void handleEditorCommands();
    void handleProjectCommands();
    bool openProject(const ProjectDescriptor& project);
    void configureResourceSearchPaths();
    void syncProjectAssets(bool force = false);
    void clearMissingAssetReferences(const std::vector<AssetRecord>& removedAssets);
};
