#include "Engine.h"

#include "../../frontend/src/editor/EditorActions.h"
#include "../../frontend/src/editor/EditorUI.h"
#include "../SceneSerializer.h"
#include "../core/Instrumentor.h"
#include "../render/Renderer.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <chrono>
#include <exception>
#include <filesystem>

namespace fs = std::filesystem;

Engine::Engine() : running(false) {}

extern void SetupEditorStyle();

bool Engine::init() {
    PROFILE_FUNCTION();

    try {
        PROFILE_BEGIN_SESSION("Lancelot Editor", "out/profiles/lancelot-editor-trace.json");

        WindowSpecification windowSpecification;
        windowSpecification.Title = "Lancelot Editor";
        windowSpecification.Width = 1280;
        windowSpecification.Height = 720;
        windowSpecification.Fullscreen = false;
        windowSpecification.Maximized = true;
        windowSpecification.API = GraphicsAPI::OpenGL;

        if (!windowManager.Init(windowSpecification)) {
            return false;
        }

        Renderer::Init(windowManager.GetGraphicsAPI());
        Renderer::OnWindowResize(windowManager.GetFramebufferWidth(), windowManager.GetFramebufferHeight());

        if (!renderer2D.init(windowManager.GetNativeWindow())) {
            return false;
        }

        configureResourceSearchPaths();

        if (!imguiLayer.Init(windowManager.GetNativeWindow())) {
            return false;
        }
        ImGui::StyleColorsDark();
        SetupEditorStyle();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        imguiLayer.SetExternalScrollCallback([this](float, float yOffset) {
            if (!editorState.showScene) {
                return;
            }

            const ImVec2 mousePos = ImGui::GetIO().MousePos;
            const bool insideViewport =
                mousePos.x >= editorState.sceneViewportScreenX &&
                mousePos.y >= editorState.sceneViewportScreenY &&
                mousePos.x <= editorState.sceneViewportScreenX + editorState.sceneViewportScreenWidth &&
                mousePos.y <= editorState.sceneViewportScreenY + editorState.sceneViewportScreenHeight;

            if (insideViewport) {
                renderer2D.OnMouseScrolled(0.0f, yOffset);
            }
        });

        const AssetRecord* defaultTexture = editorState.assetRegistry.findByPath("pillar.png");
        const std::uint64_t defaultTextureId = defaultTexture ? defaultTexture->id : 0;
        const std::string defaultTexturePath =
            defaultTexture && !defaultTexture->relativePath.empty() ? defaultTexture->relativePath : "pillar.png";
        editorState.sceneViewportWidth = static_cast<float>(windowManager.GetFramebufferWidth());
        editorState.sceneViewportHeight = static_cast<float>(windowManager.GetFramebufferHeight());
        sceneState.objects.push_back({ 0, "Player", {100.0f, 100.0f}, {1.0f, 1.0f}, 0.0f, defaultTextureId, defaultTexturePath, 0, "" });
        sceneState.objects.push_back({ 1, "Enemy", {300.0f, 200.0f}, {1.0f, 1.0f}, 0.0f, defaultTextureId, defaultTexturePath, 0, "" });
        sceneState.objects.push_back({ 2, "DebugColorQuad", {540.0f, 180.0f}, {1.25f, 1.25f}, 0.0f, 0, "", 0, "" });
        editorState.selectedObjectIndex = 0;
        editorState.assetStatus = "Create or open a project to import and persist project assets";
        editorState.sceneFilePath.clear();
        AddEditorLog(editorState, EditorLogLevel::Info, "Engine initialized.");

        running = true;
        lastFrameTime = std::chrono::steady_clock::now();
        return true;
    } catch (const std::exception&) {
        PROFILE_END_SESSION();
        return false;
    } catch (...) {
        PROFILE_END_SESSION();
        return false;
    }
}

void Engine::handleEditorCommands()
{
    switch (editorState.pendingCommand) {
    case EditorCommand::Play:
        if (editorState.mode == EditorMode::Edit) {
            playModeSceneBackup = sceneState;
            hasPlayModeBackup = true;
        }
        editorState.mode = EditorMode::Play;
        AddEditorLog(editorState, EditorLogLevel::Info, "Entered Play mode.");
        break;

    case EditorCommand::Pause:
        if (editorState.mode == EditorMode::Play) {
            editorState.mode = EditorMode::Pause;
            AddEditorLog(editorState, EditorLogLevel::Info, "Paused Play mode.");
        }
        else if (editorState.mode == EditorMode::Pause) {
            editorState.mode = EditorMode::Play;
            AddEditorLog(editorState, EditorLogLevel::Info, "Resumed Play mode.");
        }
        break;

    case EditorCommand::Stop:
        if (hasPlayModeBackup) {
            sceneState = playModeSceneBackup;
            hasPlayModeBackup = false;
        }
        editorState.mode = EditorMode::Edit;
        AddEditorLog(editorState, EditorLogLevel::Info, "Returned to Edit mode.");
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
    editorState.assetRegistry.setProjectRoot(project.rootPath);
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
    lastProjectSyncTick = static_cast<unsigned long long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    AddEditorLog(editorState, EditorLogLevel::Info, editorState.projectStatus);
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
            if (object.textureResourceId == removedAsset.id) {
                object.textureResourceId = 0;
                object.texturePath.clear();
                ++clearedBindings;
            }

            if (object.scriptResourceId == removedAsset.id) {
                object.scriptResourceId = 0;
                object.scriptPath.clear();
                ++clearedBindings;
            }
        }
    }

    if (clearedBindings > 0) {
        editorState.assetStatus =
            "Detected external asset removal. Cleared " + std::to_string(clearedBindings) + " invalid object binding(s)";
        AddEditorLog(editorState, EditorLogLevel::Warning, editorState.assetStatus);
    }
}

void Engine::syncProjectAssets(bool force) {
    if (editorState.projectRootPath.empty()) {
        return;
    }

    const auto nowPoint = std::chrono::steady_clock::now();
    const unsigned long long now = static_cast<unsigned long long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(nowPoint.time_since_epoch()).count());
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
        AddEditorLog(editorState, EditorLogLevel::Error, editorState.projectStatus);
        return;
    }

    editorState.projectStatus =
        "Detected external project changes: +" + std::to_string(summary.addedCount) +
        " / -" + std::to_string(summary.removedCount);
    if (summary.removedCount == 0) {
        editorState.assetStatus =
            "Detected " + std::to_string(summary.addedCount) + " new asset(s) from the project folder";
    }
    AddEditorLog(editorState, EditorLogLevel::Info, editorState.projectStatus);
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
        AddEditorLog(editorState, EditorLogLevel::Error, error);
    }

    editorState.pendingProjectCommand = ProjectCommand::None;
    editorState.pendingProjectDirectory.clear();
    editorState.pendingProjectName.clear();
}

void Engine::run() {
    PROFILE_FUNCTION();

    while (running) {
        PROFILE_SCOPE("Engine::Frame");

        const auto frameStart = std::chrono::steady_clock::now();
        const std::chrono::duration<float> frameDuration = frameStart - lastFrameTime;
        lastFrameTime = frameStart;
        const float deltaSeconds = frameDuration.count();

        windowManager.PollEvents();
        inputManager.processEvents(windowManager, sceneState, editorState);
        if (inputManager.shouldQuit()) {
            running = false;
        }

        imguiLayer.BeginFrame();

        renderer2D.clear();
        renderer2D.resizeSceneRenderTarget(
            static_cast<int>(editorState.sceneViewportWidth),
            static_cast<int>(editorState.sceneViewportHeight)
        );
        renderer2D.renderScene(sceneState, resourceManager, deltaSeconds);
        DrawEditorUI(sceneState, editorState, renderer2D.getSceneViewportImage());

        handleEditorCommands();
        handleProjectCommands();
        syncProjectAssets();

        gameLoop.update(sceneState, editorState);

        imguiLayer.EndFrame();
        windowManager.OnUpdate();
    }
}

void Engine::shutdown() {
    PROFILE_FUNCTION();

    if (!editorState.assetManifestPath.empty()) {
        editorState.assetRegistry.saveManifest(editorState.assetManifestPath);
    }

    imguiLayer.Shutdown();
    Renderer::Shutdown();
    resourceManager.destroy();
    renderer2D.destroy();
    windowManager.Shutdown();
    PROFILE_END_SESSION();
}
