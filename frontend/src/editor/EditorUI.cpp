#define IMGUI_ENABLE_DOCKING
#include "EditorUI.h"

#include "EditorActions.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/ScenePanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/AssetPanel.h"
#include "Panels/ConsolePanel.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>

namespace {

float GetUiScale(const ImGuiViewport* viewport) {
    return std::clamp(viewport->DpiScale, 1.0f, 2.0f);
}

float GetResponsiveToolbarHeight(float viewportWidth, float uiScale) {
    const bool compact = viewportWidth < 1360.0f;
    return (compact ? 126.0f : 96.0f) * uiScale;
}

float GetResponsiveToolbarButtonWidth(float viewportWidth) {
    if (viewportWidth < 980.0f) return 88.0f;
    if (viewportWidth < 1380.0f) return 96.0f;
    return 108.0f;
}

void DrawToolbarButton(const char* label, EditorCommand command, EditorState& editorState, float width) {
    if (ImGui::Button(label, ImVec2(width, 0.0f))) {
        editorState.pendingCommand = command;
    }
}

void ResetDefaultLayout(EditorState& editorState) {
    editorState.showHierarchy = true;
    editorState.showScene = true;
    editorState.showInspector = true;
    editorState.showProject = true;
    editorState.showConsole = true;
    editorState.resetLayoutRequested = true;
    AddEditorLog(editorState, EditorLogLevel::Info, "Reset editor layout to default.");
}

void DrawFileMenu(EditorState& editorState) {
    if (ImGui::MenuItem("Sync Project", "F5", false, !editorState.projectRootPath.empty())) {
        editorState.pendingProjectCommand = ProjectCommand::Sync;
    }

    ImGui::Separator();
    ImGui::MenuItem("New Project...", nullptr, false, false);
    ImGui::MenuItem("Open Project...", nullptr, false, false);
}

void DrawEditMenu(SceneState& sceneState, EditorState& editorState) {
    const bool hasSelection = HasSelectedObject(sceneState, editorState);

    ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
    ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
    ImGui::Separator();

    if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, hasSelection)) {
        DuplicateSelectedObject(sceneState, editorState, "Edit menu");
    }

    if (ImGui::MenuItem("Delete", "Delete", false, hasSelection)) {
        DeleteSelectedObject(sceneState, editorState);
    }
}

void DrawAssetsMenu(EditorState& editorState) {
    if (ImGui::MenuItem("Open Project Panel")) {
        editorState.showProject = true;
    }

    if (ImGui::MenuItem("Open Console")) {
        editorState.showConsole = true;
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Sync Imported Assets", nullptr, false, !editorState.projectRootPath.empty())) {
        editorState.pendingProjectCommand = ProjectCommand::Sync;
    }
}

void DrawGameObjectMenu(SceneState& sceneState, EditorState& editorState) {
    if (ImGui::MenuItem("Create Empty")) {
        CreateEmptyObject(sceneState, editorState, "GameObject", "GameObject menu");
    }

    const AssetRecord* defaultTexture = editorState.assetRegistry.findByPath("pillar.png");
    if (ImGui::MenuItem("Create Sprite", nullptr, false, defaultTexture != nullptr)) {
        CreateObjectFromAsset(sceneState, editorState, *defaultTexture, 120.0f, 120.0f, "GameObject menu");
    }
}

void DrawComponentMenu(SceneState& sceneState, EditorState& editorState) {
    GameObject* selected = GetSelectedObject(sceneState, editorState);
    const bool hasSelection = (selected != nullptr);

    if (ImGui::MenuItem("Reset Position", nullptr, false, hasSelection)) {
        ResetObjectPosition(*selected);
        AddEditorLog(editorState, EditorLogLevel::Info, "Reset selected object position.");
    }

    if (ImGui::MenuItem("Reset Scale", nullptr, false, hasSelection)) {
        ResetObjectScale(*selected);
        AddEditorLog(editorState, EditorLogLevel::Info, "Reset selected object scale.");
    }

    if (ImGui::MenuItem("Reset Rotation", nullptr, false, hasSelection)) {
        ResetObjectRotation(*selected);
        AddEditorLog(editorState, EditorLogLevel::Info, "Reset selected object rotation.");
    }
}

void DrawWindowMenu(EditorState& editorState) {
    ImGui::MenuItem(kScenePanelWindowName, nullptr, &editorState.showScene);
    ImGui::MenuItem("Hierarchy", nullptr, &editorState.showHierarchy);
    ImGui::MenuItem("Inspector", nullptr, &editorState.showInspector);
    ImGui::MenuItem("Project", nullptr, &editorState.showProject);
    ImGui::MenuItem("Console", nullptr, &editorState.showConsole);

    ImGui::Separator();
    if (ImGui::MenuItem("Reset Layout")) {
        ResetDefaultLayout(editorState);
    }
}

void DrawHelpMenu() {
    ImGui::MenuItem("About 2D Engine", nullptr, false, false);
}

void DrawToolbarMenuBar(SceneState& sceneState, EditorState& editorState) {
    if (!ImGui::BeginMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        DrawFileMenu(editorState);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        DrawEditMenu(sceneState, editorState);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Assets")) {
        DrawAssetsMenu(editorState);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("GameObject")) {
        DrawGameObjectMenu(sceneState, editorState);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Component")) {
        DrawComponentMenu(sceneState, editorState);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Window")) {
        DrawWindowMenu(editorState);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
        DrawHelpMenu();
        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

}

void SetupEditorStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(12.0f, 8.0f);
    style.ItemSpacing = ImVec2(10.0f, 10.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.WindowRounding = 6.0f;
    style.ChildRounding = 5.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 4.0f;
    style.TabRounding = 5.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.DockingSeparatorSize = 1.0f;
    style.WindowMinSize = ImVec2(220.0f, 140.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(0.91f, 0.93f, 0.96f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.58f, 0.62f, 0.69f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.105f, 0.115f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.125f, 0.14f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.125f, 0.14f, 0.98f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.23f, 0.25f, 0.29f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.16f, 0.19f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.22f, 0.27f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.23f, 0.26f, 0.33f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.12f, 0.14f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.18f, 0.24f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.18f, 0.24f, 0.34f, 0.75f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.32f, 0.46f, 0.85f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.28f, 0.38f, 0.54f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.33f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.33f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.29f, 0.39f, 0.56f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.17f, 0.20f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.31f, 0.43f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.21f, 0.28f, 0.39f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.14f, 0.17f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.22f, 0.30f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.24f, 0.27f, 0.32f, 1.0f);
    style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.09f, 0.095f, 0.105f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.12f, 0.14f, 1.0f);
}

void DrawToolbarContent(EditorState& editorState)
{
    const float width = ImGui::GetContentRegionAvail().x;
    const float buttonWidth = GetResponsiveToolbarButtonWidth(width);
    const bool compact = width < 1360.0f;
    const char* modeText = "Edit";
    if (editorState.mode == EditorMode::Play) modeText = "Play";
    else if (editorState.mode == EditorMode::Pause) modeText = "Pause";

    if (compact) {
        ImGui::TextUnformatted("SDLTest 2D Editor");
        ImGui::SameLine();
        ImGui::TextDisabled("| Project-aware workspace");
        DrawToolbarButton("Play##Toolbar", EditorCommand::Play, editorState, buttonWidth);
        ImGui::SameLine();
        DrawToolbarButton("Pause##Toolbar", EditorCommand::Pause, editorState, buttonWidth);
        ImGui::SameLine();
        DrawToolbarButton("Stop##Toolbar", EditorCommand::Stop, editorState, buttonWidth);
        ImGui::SameLine();
        ImGui::Text("Mode: %s", modeText);
        return;
    }

    if (ImGui::BeginTable("ToolbarLayout", 3, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthStretch, 1.2f);
        ImGui::TableSetupColumn("Center", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch, 1.1f);

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("SDLTest 2D Editor");
        ImGui::SameLine();
        ImGui::TextDisabled("| Project-aware workspace");

        ImGui::TableNextColumn();
        const float controlsWidth = buttonWidth * 3.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, (ImGui::GetColumnWidth() - controlsWidth) * 0.5f));
        DrawToolbarButton("Play##Toolbar", EditorCommand::Play, editorState, buttonWidth);
        ImGui::SameLine();
        DrawToolbarButton("Pause##Toolbar", EditorCommand::Pause, editorState, buttonWidth);
        ImGui::SameLine();
        DrawToolbarButton("Stop##Toolbar", EditorCommand::Stop, editorState, buttonWidth);

        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Mode: %s", modeText);
        ImGui::Text("Selection: %d", editorState.selectedObjectIndex);
        ImGui::EndTable();
    }
}

void DrawEditorUI(SceneState& sceneState, EditorState& editorState, const SceneViewportImage& sceneImage)
{
    static bool dockspaceOpen = true;
    static bool layout_initialized = false;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float uiScale = GetUiScale(viewport);
    ImGui::GetIO().FontGlobalScale = uiScale;
    const float toolbarHeight = GetResponsiveToolbarHeight(viewport->Size.x, uiScale);

    if (editorState.resetLayoutRequested) {
        layout_initialized = false;
        editorState.resetLayoutRequested = false;
    }

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + toolbarHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - toolbarHeight));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);

    ImGuiID dockspace_id = ImGui::GetID("EditorDockspace");
    ImGui::DockSpace(dockspace_id);

    if (!layout_initialized)
    {
        layout_initialized = true;

        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(viewport->Size.x, viewport->Size.y - toolbarHeight));

        ImGuiID dock_main = dockspace_id;
        ImGuiID dock_project = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.22f, nullptr, &dock_main);
        ImGuiID dock_hierarchy = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.13f, nullptr, &dock_main);
        ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.28f, nullptr, &dock_main);
        ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.24f, nullptr, &dock_main);

        ImGui::DockBuilderDockWindow("Project", dock_project);
        ImGui::DockBuilderDockWindow("Hierarchy", dock_hierarchy);
        ImGui::DockBuilderDockWindow("Inspector", dock_right);
        ImGui::DockBuilderDockWindow(kScenePanelWindowName, dock_main);
        ImGui::DockBuilderDockWindow("Console", dock_bottom);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, toolbarHeight));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 12.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.14f, 1.0f));

    ImGui::Begin("##ToolbarBar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus);

    DrawToolbarMenuBar(sceneState, editorState);
    DrawToolbarContent(editorState);
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    if (editorState.showHierarchy) DrawHierarchyPanel(sceneState, editorState);
    if (editorState.showScene) DrawScenePanel(sceneState, editorState, sceneImage);
    if (editorState.showInspector) DrawInspectorPanel(sceneState, editorState);
    if (editorState.showProject) DrawAssetPanel(sceneState, editorState);
    if (editorState.showConsole) DrawConsolePanel(editorState);
}
