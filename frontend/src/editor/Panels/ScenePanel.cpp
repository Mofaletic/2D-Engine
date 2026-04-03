#include "ScenePanel.h"
#include "imgui.h"
#include "../../../../backend/SceneSerializer.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

static std::string sceneStatus = "No scene operation yet";
static std::string sceneFilePath = "scene.json";

void DrawScenePanel(SceneState& sceneState, EditorState& editorState, SDL_Texture* sceneTexture)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

    ImGui::Begin("Scene");

    const char* modeText = "Edit";
    if (editorState.mode == EditorMode::Play) modeText = "Play";
    else if (editorState.mode == EditorMode::Pause) modeText = "Pause";

    // ===== 顶部状态 =====
    ImGui::Text("Scene Overview");
    ImGui::Separator();
    ImGui::Text("Mode: %s", modeText);
    ImGui::Text("Object count: %d", static_cast<int>(sceneState.objects.size()));

    int index = editorState.selectedObjectIndex;
    if (index >= 0 && index < static_cast<int>(sceneState.objects.size())) {
        ImGui::Text("Selected: %s", sceneState.objects[index].name.c_str());
    }
    else {
        ImGui::Text("Selected: None");
    }

    // ===== 简单操作按钮 =====
    ImGui::Separator();
    ImGui::Text("Viewport");

    ImVec2 availableRegion = ImGui::GetContentRegionAvail();
    float viewportHeight = std::max(180.0f, availableRegion.y * 0.45f);
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

    if (ImGui::Button("Reset Selected Position")) {
        if (index >= 0 && index < static_cast<int>(sceneState.objects.size())) {
            sceneState.objects[index].position[0] = 0.0f;
            sceneState.objects[index].position[1] = 0.0f;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset Selected Scale")) {
        if (index >= 0 && index < static_cast<int>(sceneState.objects.size())) {
            sceneState.objects[index].scale[0] = 1.0f;
            sceneState.objects[index].scale[1] = 1.0f;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Add Test Object")) {
        GameObject newObject;
        newObject.id = static_cast<int>(sceneState.objects.size());
        newObject.name = "New Object " + std::to_string(newObject.id);
        newObject.position[0] = 0.0f;
        newObject.position[1] = 0.0f;
        newObject.scale[0] = 1.0f;
        newObject.scale[1] = 1.0f;
        newObject.texturePath = "test.png";

        sceneState.objects.push_back(newObject);
        editorState.selectedObjectIndex = static_cast<int>(sceneState.objects.size()) - 1;
    }

    ImGui::SameLine();

    if (ImGui::Button("Delete Selected")) {
        if (index >= 0 && index < static_cast<int>(sceneState.objects.size())) {
            sceneState.objects.erase(sceneState.objects.begin() + index);

            if (sceneState.objects.empty()) {
                editorState.selectedObjectIndex = -1;
            }
            else if (index >= static_cast<int>(sceneState.objects.size())) {
                editorState.selectedObjectIndex = static_cast<int>(sceneState.objects.size()) - 1;
            }
        }
    }
	// ===== 场景文件操作 =====
    static char sceneNameBuffer[128] = "TestScene";

    ImGui::InputText("Scene Name", sceneNameBuffer, sizeof(sceneNameBuffer));

    if (ImGui::Button("Save Scene")) {
        if (SaveSceneToFile(sceneState, sceneNameBuffer, sceneFilePath)) {
            sceneStatus = "Scene saved successfully";
        }
        else {
            sceneStatus = "Failed to save scene";
        }
    }

    ImGui::SameLine();

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
    ImGui::Text("Scene File: %s", sceneFilePath.c_str());
    ImGui::Text("Status: %s", sceneStatus.c_str());

    // ===== 全部对象列表 =====
    ImGui::Separator();
    ImGui::Text("Scene Objects:");

    for (int i = 0; i < static_cast<int>(sceneState.objects.size()); ++i) {
        auto& obj = sceneState.objects[i];
        bool selected = (editorState.selectedObjectIndex == i);

        // 左边：对象名可点击
        if (ImGui::Selectable(obj.name.c_str(), selected, 0, ImVec2(150, 0))) {
            editorState.selectedObjectIndex = i;
        }

        // 右边：显示位置和缩放
        ImGui::SameLine(180);
        ImGui::Text("Pos(%.1f, %.1f)", obj.position[0], obj.position[1]);

        ImGui::SameLine(320);
        ImGui::Text("Scale(%.1f, %.1f)", obj.scale[0], obj.scale[1]);
    }

    index = editorState.selectedObjectIndex;
    // ===== 当前选中对象详细信息 =====
    ImGui::Separator();
    if (index >= 0 && index < static_cast<int>(sceneState.objects.size())) {
        auto& obj = sceneState.objects[index];
        ImGui::Text("Selected Object Details");
        ImGui::Text("Name: %s", obj.name.c_str());
        ImGui::Text("ID: %d", obj.id);
        ImGui::Text("Position: %.1f, %.1f", obj.position[0], obj.position[1]);
        ImGui::Text("Scale: %.1f, %.1f", obj.scale[0], obj.scale[1]);
    }
    else {
        ImGui::Text("No object selected");
    }

    ImGui::End();

    ImGui::PopStyleVar(2);
}
