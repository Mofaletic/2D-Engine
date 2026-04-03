#define IMGUI_ENABLE_DOCKING
#include "EditorUI.h"
#include <iostream>
#include "imgui.h"
#include "imgui_internal.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/ScenePanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/AssetPanel.h"
#include "Panels/ConsolePanel.h"
#include <cstdio>



// ================= 样式 =================
void SetupEditorStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.0f);

    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.25f, 0.35f, 1.0f);

    style.Colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.40f, 0.60f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.35f, 0.55f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.12f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.25f, 0.35f, 1.0f);

    style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.0f);
}

// ================= Toolbar =================
void DrawToolbarContent(EditorState& editorState)
{
    float width = ImGui::GetWindowWidth();
    float buttonWidth = 70.0f;
    float totalWidth = buttonWidth * 3 + 10 * 2;

    ImGui::SetCursorPosX((width - totalWidth) * 0.5f);

    if (ImGui::Button("Play", ImVec2(buttonWidth, 0)))
        editorState.pendingCommand = EditorCommand::Play;

    ImGui::SameLine();
    if (ImGui::Button("Pause", ImVec2(buttonWidth, 0)))
        editorState.pendingCommand = EditorCommand::Pause;

    ImGui::SameLine();
    if (ImGui::Button("Stop", ImVec2(buttonWidth, 0)))
        editorState.pendingCommand = EditorCommand::Stop;

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    const char* modeText = "Edit";
    if (editorState.mode == EditorMode::Play) modeText = "Play";
    else if (editorState.mode == EditorMode::Pause) modeText = "Pause";

    ImGui::Text("Mode: %s", modeText);
}

// ================= 主界面 =================
void DrawEditorUI(SceneState& sceneState, EditorState& editorState, SDL_Texture* sceneTexture)
{
    static bool dockspaceOpen = true;
    static bool layout_initialized = false;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove;

    // ===== DockSpace =====
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + 40));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - 40));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);

    ImGuiID dockspace_id = ImGui::GetID("EditorDockspace");
    ImGui::DockSpace(dockspace_id);

    // ===== 默认布局 =====
    if (!layout_initialized)
    {
        layout_initialized = true;

        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        ImGuiID dock_main = dockspace_id;

        ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.18f, nullptr, &dock_main);
        ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.22f, nullptr, &dock_main);
        ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.25f, nullptr, &dock_main);
        ImGuiID dock_bottom_left = ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Left, 0.5f, nullptr, &dock_bottom);

        ImGui::DockBuilderDockWindow("Hierarchy", dock_left);
        ImGui::DockBuilderDockWindow("Inspector", dock_right);
        ImGui::DockBuilderDockWindow("Scene", dock_main);
        ImGui::DockBuilderDockWindow("Project", dock_bottom_left);
        ImGui::DockBuilderDockWindow("Console", dock_bottom);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::End();
    ImGui::PopStyleVar();

    // ===== Toolbar（顶层）=====
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 40));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));

    ImGui::Begin("##ToolbarBar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoDocking);

    DrawToolbarContent(editorState);

    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    // ===== Panels =====
    DrawHierarchyPanel(sceneState, editorState);
    DrawScenePanel(sceneState, editorState, sceneTexture);
    DrawInspectorPanel(sceneState, editorState);
    DrawAssetPanel(sceneState, editorState);
    DrawConsolePanel();
}
