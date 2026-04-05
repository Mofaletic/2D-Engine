#include "ScenePanel.h"
#include "imgui.h"
#include "../../../../backend/SceneSerializer.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

static std::string sceneStatus = "No scene operation yet";
static std::string sceneFilePath = "scene.json";

namespace {

void DrawActionButtonRow(SceneState& sceneState, EditorState& editorState, int index) {
    const float width = ImGui::GetContentRegionAvail().x;
    const int columns = width < 780.0f ? 2 : 4;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float buttonWidth = (width - spacing * static_cast<float>(columns - 1)) / static_cast<float>(columns);

    auto drawButton = [&](const char* label, auto&& fn) {
        if (ImGui::Button(label, ImVec2(buttonWidth, 0.0f))) {
            fn();
        }
    };

    drawButton("Reset Position", [&]() {
        if (index >= 0 && index < static_cast<int>(sceneState.objects.size())) {
            sceneState.objects[index].position[0] = 0.0f;
            sceneState.objects[index].position[1] = 0.0f;
        }
    });

    if (columns > 1) ImGui::SameLine();
    drawButton("Reset Scale", [&]() {
        if (index >= 0 && index < static_cast<int>(sceneState.objects.size())) {
            sceneState.objects[index].scale[0] = 1.0f;
            sceneState.objects[index].scale[1] = 1.0f;
        }
    });

    if (columns > 2) {
        ImGui::SameLine();
    } else {
        ImGui::NewLine();
    }
    drawButton("Add Object", [&]() {
        GameObject newObject;
        newObject.id = static_cast<int>(sceneState.objects.size());
        newObject.name = "New Object " + std::to_string(newObject.id);
        newObject.position[0] = 0.0f;
        newObject.position[1] = 0.0f;
        newObject.scale[0] = 1.0f;
        newObject.scale[1] = 1.0f;
        newObject.textureResourceId = 0;
        newObject.texturePath = "pillar.png";
        sceneState.objects.push_back(newObject);
        editorState.selectedObjectIndex = static_cast<int>(sceneState.objects.size()) - 1;
    });

    if (columns > 3) {
        ImGui::SameLine();
    } else {
        ImGui::NewLine();
    }
    drawButton("Delete Selected", [&]() {
        if (index >= 0 && index < static_cast<int>(sceneState.objects.size())) {
            sceneState.objects.erase(sceneState.objects.begin() + index);

            if (sceneState.objects.empty()) {
                editorState.selectedObjectIndex = -1;
            }
            else if (index >= static_cast<int>(sceneState.objects.size())) {
                editorState.selectedObjectIndex = static_cast<int>(sceneState.objects.size()) - 1;
            }
        }
    });
}

}

void DrawScenePanel(SceneState& sceneState, EditorState& editorState, SDL_Texture* sceneTexture)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

    ImGui::Begin("Scene");

    const char* modeText = "Edit";
    if (editorState.mode == EditorMode::Play) modeText = "Play";
    else if (editorState.mode == EditorMode::Pause) modeText = "Pause";

    // ===== 顶部状态 =====
    ImGui::TextUnformatted("Scene Overview");
    ImGui::Separator();

    if (ImGui::BeginTable("SceneOverview", 3, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableNextColumn();
        ImGui::Text("Mode: %s", modeText);
        ImGui::TableNextColumn();
        ImGui::Text("Objects: %d", static_cast<int>(sceneState.objects.size()));
        ImGui::TableNextColumn();
        int index = editorState.selectedObjectIndex;
        if (index >= 0 && index < static_cast<int>(sceneState.objects.size())) {
            ImGui::TextWrapped("Selected: %s", sceneState.objects[index].name.c_str());
        }
        else {
            ImGui::TextUnformatted("Selected: None");
        }
        ImGui::EndTable();
    }

    int index = editorState.selectedObjectIndex;
    ImGui::Separator();
    ImGui::TextUnformatted("Viewport");

    ImVec2 availableRegion = ImGui::GetContentRegionAvail();
    float viewportHeight = std::clamp(availableRegion.y * 0.34f, 140.0f, 420.0f);
    ImVec2 viewportSize(availableRegion.x, viewportHeight);

    editorState.sceneViewportWidth = viewportSize.x;
    editorState.sceneViewportHeight = viewportSize.y;

    ImGui::BeginChild("SceneViewport", viewportSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    if (sceneTexture) {
        float textureWidth = 1.0f;
        float textureHeight = 1.0f;
        if (!SDL_GetTextureSize(sceneTexture, &textureWidth, &textureHeight)) {
            textureWidth = viewportSize.x;
            textureHeight = viewportSize.y;
        }

        float scale = std::min(viewportSize.x / textureWidth, viewportSize.y / textureHeight);
        scale = (scale > 0.0f) ? scale : 1.0f;

        ImVec2 imageSize(textureWidth * scale, textureHeight * scale);
        ImVec2 cursor = ImGui::GetCursorPos();
        float offsetX = (viewportSize.x - imageSize.x) * 0.5f;
        float offsetY = (viewportSize.y - imageSize.y) * 0.5f;
        ImGui::SetCursorPos(ImVec2(cursor.x + std::max(0.0f, offsetX), cursor.y + std::max(0.0f, offsetY)));
        ImGui::Image((ImTextureID)(intptr_t)sceneTexture, imageSize);
    }
    else {
        ImVec2 childPos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(childPos.x + 12.0f, childPos.y + 12.0f),
            IM_COL32(220, 220, 220, 255),
            "Scene viewport is initializing..."
        );
    }

    ImGui::EndChild();

    ImGui::Separator();
    DrawActionButtonRow(sceneState, editorState, index);
    ImGui::Spacing();

    static char sceneNameBuffer[128] = "TestScene";

    ImGui::InputText("Scene Name", sceneNameBuffer, sizeof(sceneNameBuffer));

    const float sceneToolsWidth = ImGui::GetContentRegionAvail().x;
    const bool stackSceneButtons = sceneToolsWidth < 520.0f;

    if (ImGui::Button("Save Scene")) {
        if (SaveSceneToFile(sceneState, sceneNameBuffer, sceneFilePath)) {
            sceneStatus = "Scene saved successfully";
        }
        else {
            sceneStatus = "Failed to save scene";
        }
    }

    if (!stackSceneButtons) {
        ImGui::SameLine();
    }

    if (ImGui::Button("Load Scene")) {
        std::string loadedSceneName;
        if (LoadSceneFromFile(sceneState, editorState, loadedSceneName, sceneFilePath)) {
            strncpy(sceneNameBuffer, loadedSceneName.c_str(), sizeof(sceneNameBuffer));
            sceneNameBuffer[sizeof(sceneNameBuffer) - 1] = '\0';
            sceneStatus = "Scene loaded successfully";
        }
        else {
            sceneStatus = "Failed to load scene";
        }
    }
    ImGui::TextWrapped("Scene File: %s", sceneFilePath.c_str());
    ImGui::TextWrapped("Status: %s", sceneStatus.c_str());

    ImGui::Separator();
    ImGui::TextUnformatted("Scene Objects");

    float listHeight = std::clamp(ImGui::GetContentRegionAvail().y * 0.45f, 110.0f, 220.0f);
    if (ImGui::BeginTable("SceneObjectTable", 3,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
        ImVec2(0.0f, listHeight))) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.5f);
        ImGui::TableSetupColumn("Position", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Scale", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableHeadersRow();

        for (int i = 0; i < static_cast<int>(sceneState.objects.size()); ++i) {
            auto& obj = sceneState.objects[i];
            bool selected = (editorState.selectedObjectIndex == i);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            if (ImGui::Selectable(obj.name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
                editorState.selectedObjectIndex = i;
            }
            ImGui::TableNextColumn();
            ImGui::Text("X %.1f  Y %.1f", obj.position[0], obj.position[1]);
            ImGui::TableNextColumn();
            ImGui::Text("X %.1f  Y %.1f", obj.scale[0], obj.scale[1]);
        }
        ImGui::EndTable();
    }

    index = editorState.selectedObjectIndex;
    ImGui::Separator();
    ImGui::TextUnformatted("Selected Object Details");
    const float detailsHeight = std::max(70.0f, ImGui::GetContentRegionAvail().y);
    ImGui::BeginChild("SceneSelectedDetails", ImVec2(0.0f, detailsHeight), true);
    if (index >= 0 && index < static_cast<int>(sceneState.objects.size())) {
        auto& obj = sceneState.objects[index];
        ImGui::TextWrapped("Name: %s", obj.name.c_str());
        ImGui::Text("ID: %d", obj.id);
        ImGui::Text("Position: %.1f, %.1f", obj.position[0], obj.position[1]);
        ImGui::Text("Scale: %.1f, %.1f", obj.scale[0], obj.scale[1]);
        ImGui::TextWrapped("Texture Path: %s", obj.texturePath.c_str());
    }
    else {
        ImGui::TextWrapped("No object selected. Pick an object from the list above to inspect it.");
    }
    ImGui::EndChild();

    ImGui::End();

    ImGui::PopStyleVar(2);
}
