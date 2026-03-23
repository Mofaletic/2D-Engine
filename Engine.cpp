#include "Engine.h"
#include <iostream>
#include "editor/EditorUI.h"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
Engine::Engine() : running(false) {}

extern void SetupEditorStyle();

bool Engine::init() {
    if (!windowManager.init("SDL3 Window Control Demo", 800, 600)) {
        return false;
    }

    if (!renderer2D.init(windowManager.getWindow())) {
        return false;
    }

    if (!resourceManager.loadTexture("test.png", renderer2D.getRenderer())) {
        return false;
    }

    // ✅ 初始化 ImGui
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    SetupEditorStyle();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui_ImplSDL3_InitForSDLRenderer(windowManager.getWindow(), renderer2D.getRenderer());
    ImGui_ImplSDLRenderer3_Init(renderer2D.getRenderer());

    running = true;
    return true;
}

void Engine::run() {
    SDL_Event event;
    while (running) {
        // 1️⃣ 处理事件
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);   // ImGui事件处理
            if (event.type == SDL_EVENT_QUIT) running = false;
        }

        // 开始 ImGui 帧
        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui::NewFrame();

        // 绘制 UI
        DrawEditorUI();

        // ✅ 渲染 2D 内容
        renderer2D.clear();
        renderer2D.drawTexture(resourceManager.getTexture());


        // 渲染 ImGui
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer2D.getRenderer());

        // 渲染 2D
        renderer2D.present();
    }
}

void Engine::shutdown() {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    resourceManager.destroy();
    renderer2D.destroy();
    windowManager.destroy();
}