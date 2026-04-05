#include "Engine.h"
#include <iostream>
#include "../../frontend/src/editor/EditorUI.h"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <filesystem>
#include "../SceneSerializer.h"

Engine::Engine() : running(false) {}

extern void SetupEditorStyle();

namespace fs = std::filesystem;

bool Engine::init() {
    if (!windowManager.init("SDL3 Window Control Demo", 800, 600)) {
        return false;
    }

    if (!renderer2D.init(windowManager.getWindow())) {
        return false;
    }

    resourceManager.setRenderer(renderer2D.getRenderer());
    configureResourceSearchPaths();

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    SetupEditorStyle();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui_ImplSDL3_InitForSDLRenderer(windowManager.getWindow(), renderer2D.getRenderer());
    ImGui_ImplSDLRenderer3_Init(renderer2D.getRenderer());

    sceneState.objects.push_back({ 0, "Player", {100.0f, 100.0f}, {1.0f, 1.0f}, 0, "pillar.png" });
    sceneState.objects.push_back({ 1, "Enemy", {300.0f, 200.0f}, {1.0f, 1.0f}, 0, "pillar.png" });
    editorState.selectedObjectIndex = 0;
    editorState.assetStatus = "Create or open a project to import and persist project assets";
    editorState.sceneFilePath.clear();

    running = true;
    return true;
}
void Engine::handleEditorCommands()
{
    if (editorState.pendingCommand != EditorCommand::None) {
        std::cout << "[Engine] pendingCommand = "
            << static_cast<int>(editorState.pendingCommand) << std::endl;
    }

    switch (editorState.pendingCommand) {
    case EditorCommand::Play:
        SDL_Log("[Engine] Play command received");
        if (editorState.mode == EditorMode::Edit) {
            std::cout << "[Engine] Backup saved" << std::endl;
            playModeSceneBackup = sceneState;
            hasPlayModeBackup = true;
        }
        editorState.mode = EditorMode::Play;
        break;

    case EditorCommand::Pause:
        SDL_Log("[Engine] Pause command received");
        if (editorState.mode == EditorMode::Play) {
            editorState.mode = EditorMode::Pause;
        }
        else if (editorState.mode == EditorMode::Pause) {
            editorState.mode = EditorMode::Play;
        }
        break;

    case EditorCommand::Stop:
        SDL_Log("[Engine] Stop command received");
        if (hasPlayModeBackup) {
            SDL_Log("[Engine] Restoring backup");
            sceneState = playModeSceneBackup;
            hasPlayModeBackup = false;
        }
        editorState.mode = EditorMode::Edit;
        break;

    case EditorCommand::None:
    default:
        break;
    }

    editorState.pendingCommand = EditorCommand::None;
}

void Engine::configureResourceSearchPaths() {
    resourceManager.setSearchPaths({ ".", "asset", "asset/image", "asset/image/siheyuan" });
    if (!editorState.projectRootPath.empty() && !editorState.assetRegistry.getProjectAssetRoot().empty()) {
        resourceManager.addSearchPath(editorState.assetRegistry.getProjectAssetRoot());
    }
}

bool Engine::openProject(const ProjectDescriptor& project) {
    editorState.projectName = project.name;
    editorState.projectRootPath = project.rootPath;
    editorState.projectFilePath = project.projectFilePath;
    editorState.assetManifestPath = project.assetManifestPath;
    editorState.sceneFilePath = project.defaultScenePath;
    editorState.assetRegistry.clear();
    editorState.assetRegistry.setProjectAssetRoot(project.assetRootPath);

    const bool loadedManifest = editorState.assetRegistry.loadManifest(project.assetManifestPath);
    if (!loadedManifest) {
        editorState.assetRegistry.rebuildFromProjectAssets();
    }
    editorState.assetRegistry.saveManifest(project.assetManifestPath);

    configureResourceSearchPaths();
    resourceManager.releaseAllTextures();

    std::string loadedSceneName;
    if (!editorState.sceneFilePath.empty() && fs::exists(editorState.sceneFilePath)) {
        if (LoadSceneFromFile(sceneState, editorState, loadedSceneName, editorState.sceneFilePath)) {
            editorState.projectStatus = "Opened project: " + project.name;
        } else {
            editorState.projectStatus = "Opened project, but failed to load the default scene";
        }
    } else if (!editorState.sceneFilePath.empty()) {
        SaveSceneToFile(sceneState, project.name, editorState.sceneFilePath);
        editorState.projectStatus = "Created project: " + project.name;
    }

    editorState.assetStatus = editorState.assetRegistry.getAssetCount() > 0
        ? "Project assets loaded"
        : "Project opened with no imported assets yet";
    lastProjectSyncTick = SDL_GetTicks();
    return true;
}

void Engine::clearMissingAssetReferences(const std::vector<AssetRecord>& removedAssets) {
    if (removedAssets.empty()) {
        return;
    }

    std::size_t clearedBindings = 0;
    for (const AssetRecord& removedAsset : removedAssets) {
        resourceManager.releaseTexture(removedAsset.sourcePath);
        if (!removedAsset.relativePath.empty()) {
            resourceManager.releaseTexture(removedAsset.relativePath);
        }

        for (GameObject& object : sceneState.objects) {
            if (object.textureResourceId != removedAsset.id) {
                continue;
            }

            object.textureResourceId = 0;
            object.texturePath.clear();
            ++clearedBindings;
        }
    }

    if (clearedBindings > 0) {
        editorState.assetStatus =
            "Detected external asset removal. Cleared " + std::to_string(clearedBindings) + " invalid object binding(s)";
    }
}

void Engine::syncProjectAssets(bool force) {
    if (editorState.projectRootPath.empty()) {
        return;
    }

    const Uint64 now = SDL_GetTicks();
    if (!force && now - lastProjectSyncTick < 1000) {
        return;
    }
    lastProjectSyncTick = now;

    const AssetSyncSummary summary = editorState.assetRegistry.synchronizeProjectAssets();
    const bool projectMissing = !fs::exists(editorState.projectRootPath);
    if (summary.addedCount == 0 && summary.removedCount == 0 && !projectMissing) {
        return;
    }

    clearMissingAssetReferences(summary.removedAssets);
    if (!projectMissing) {
        editorState.assetRegistry.saveManifest(editorState.assetManifestPath);
    }
    configureResourceSearchPaths();
    resourceManager.releaseAllTextures();

    if (projectMissing) {
        editorState.projectStatus = "Current project folder was removed outside the editor";
        return;
    }

    editorState.projectStatus =
        "Detected external project changes: +" + std::to_string(summary.addedCount) +
        " / -" + std::to_string(summary.removedCount);
    if (summary.removedCount == 0) {
        editorState.assetStatus =
            "Detected " + std::to_string(summary.addedCount) + " new asset(s) from the project folder";
    }
}

void Engine::handleProjectCommands() {
    if (editorState.pendingProjectCommand == ProjectCommand::None) {
        return;
    }

    ProjectDescriptor project;
    std::string error;
    bool success = false;

    switch (editorState.pendingProjectCommand) {
    case ProjectCommand::Create:
        success = ProjectManager::CreateProject(
            editorState.pendingProjectName,
            editorState.pendingProjectDirectory,
            project,
            error
        );
        if (success) {
            openProject(project);
        }
        break;
    case ProjectCommand::Open:
        success = ProjectManager::LoadProject(editorState.pendingProjectDirectory, project, error);
        if (success) {
            openProject(project);
        }
        break;
    case ProjectCommand::Sync:
        syncProjectAssets(true);
        success = true;
        break;
    case ProjectCommand::None:
    default:
        break;
    }

    if (!success && !error.empty()) {
        editorState.projectStatus = error;
    }

    editorState.pendingProjectCommand = ProjectCommand::None;
    editorState.pendingProjectDirectory.clear();
    editorState.pendingProjectName.clear();
}

void Engine::run() {
    while (running) {
        inputManager.processEvents(windowManager);
        if (inputManager.shouldQuit()) {
            running = false;
        }

        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui::NewFrame();

        renderer2D.clear();
        DrawEditorUI(sceneState, editorState, renderer2D.getSceneRenderTarget());

        handleEditorCommands();
        handleProjectCommands();
        syncProjectAssets();
        gameLoop.update(sceneState, editorState);

        renderer2D.resizeSceneRenderTarget(
            static_cast<int>(editorState.sceneViewportWidth),
            static_cast<int>(editorState.sceneViewportHeight)
        );
        renderer2D.renderScene(sceneState, resourceManager);

        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer2D.getRenderer());
        renderer2D.present();
    }
}

void Engine::shutdown() {
    if (!editorState.assetManifestPath.empty()) {
        editorState.assetRegistry.saveManifest(editorState.assetManifestPath);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    resourceManager.destroy();
    renderer2D.destroy();
    windowManager.destroy();
}
