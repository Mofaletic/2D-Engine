#include "Engine.h"
#include <iostream>
#include "../../frontend/src/editor/EditorUI.h"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

Engine::Engine() : running(false) {}

extern void SetupEditorStyle();

bool Engine::init() {
    editorState.assetRegistry.setProjectAssetRoot("asset/imported");

    if (!windowManager.init("SDL3 Window Control Demo", 800, 600)) {
        return false;
    }

    if (!renderer2D.init(windowManager.getWindow())) {
        return false;
    }

    resourceManager.setRenderer(renderer2D.getRenderer());
    resourceManager.addSearchPath("asset/image/siheyuan");
    resourceManager.addSearchPath("asset/imported");
    const bool loadedManifest = editorState.assetRegistry.loadManifest("asset/asset_registry.json");
    if (!loadedManifest) {
        editorState.assetRegistry.rebuildFromProjectAssets();
        editorState.assetRegistry.importFolder("asset");
    }
    editorState.assetRegistry.saveManifest("asset/asset_registry.json");
    editorState.assetStatus = editorState.assetRegistry.getAssetCount() > 0
        ? "Project asset library ready"
        : "No registered project assets yet";

    // ✅ 初始化 ImGui
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    SetupEditorStyle();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui_ImplSDL3_InitForSDLRenderer(windowManager.getWindow(), renderer2D.getRenderer());
    ImGui_ImplSDLRenderer3_Init(renderer2D.getRenderer());

    // 初始化场景数据
    const AssetRecord* defaultTexture = editorState.assetRegistry.findByPath("pillar.png");
    const std::uint64_t defaultTextureId = defaultTexture ? defaultTexture->id : 0;
    const std::string defaultTexturePath =
        defaultTexture && !defaultTexture->relativePath.empty() ? defaultTexture->relativePath : "pillar.png";
    sceneState.objects.push_back({ 0, "Player", {100.0f, 100.0f}, {1.0f, 1.0f}, defaultTextureId, defaultTexturePath });
    sceneState.objects.push_back({ 1, "Enemy", {300.0f, 200.0f}, {1.0f, 1.0f}, defaultTextureId, defaultTexturePath });
    editorState.selectedObjectIndex = 0;

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

void Engine::run() {
    while (running) {
        // 1. 输入
        inputManager.processEvents(windowManager);
        if (inputManager.shouldQuit()) {
            running = false;
        }

        // 2. 开始 ImGui 帧

        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui::NewFrame();

        // 3. 编辑器 UI
        renderer2D.clear();
        DrawEditorUI(sceneState, editorState, renderer2D.getSceneRenderTarget());

        // 4. 处理编辑器命令（Play / Pause / Stop）
        handleEditorCommands();

        // 5. 逻辑更新
        gameLoop.update(sceneState, editorState);

        // 6. 渲染场景
        renderer2D.resizeSceneRenderTarget(
            static_cast<int>(editorState.sceneViewportWidth),
            static_cast<int>(editorState.sceneViewportHeight)
        );
        renderer2D.renderScene(sceneState, resourceManager);

        // 7. 渲染 ImGui
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer2D.getRenderer());

        // 8. 提交画面
        renderer2D.present();
    }
}

void Engine::shutdown() {
    editorState.assetRegistry.saveManifest("asset/asset_registry.json");

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    resourceManager.destroy();
    renderer2D.destroy();
    windowManager.destroy();
}
