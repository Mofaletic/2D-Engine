#include "InspectorPanel.h"

#include "../EditorActions.h"
#include "imgui.h"

#include <array>
#include <cstdio>
#include <cstring>

namespace {

void DrawResponsiveInspectorButtonRow(
    const char* firstLabel,
    const char* secondLabel,
    const char* thirdLabel,
    bool& firstPressed,
    bool& secondPressed,
    bool& thirdPressed)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float spacing = style.ItemSpacing.x;
    const float minButtonWidth = 96.0f;
    const float rowButtonWidth = (availableWidth - spacing * 2.0f) / 3.0f;

    firstPressed = false;
    secondPressed = false;
    thirdPressed = false;

    if (rowButtonWidth >= minButtonWidth) {
        firstPressed = ImGui::Button(firstLabel, ImVec2(rowButtonWidth, 0.0f));
        ImGui::SameLine();
        secondPressed = ImGui::Button(secondLabel, ImVec2(rowButtonWidth, 0.0f));
        ImGui::SameLine();
        thirdPressed = ImGui::Button(thirdLabel, ImVec2(rowButtonWidth, 0.0f));
        return;
    }

    firstPressed = ImGui::Button(firstLabel, ImVec2(-1.0f, 0.0f));
    secondPressed = ImGui::Button(secondLabel, ImVec2(-1.0f, 0.0f));
    thirdPressed = ImGui::Button(thirdLabel, ImVec2(-1.0f, 0.0f));
}

}

void DrawInspectorPanel(SceneState& sceneState, EditorState& editorState)
{
    ImGui::Begin("Inspector", &editorState.showInspector);
    ImGui::PushTextWrapPos(0.0f);
    ImGui::PushItemWidth(-1.0f);

    static int lastSelectedIndex = -1;
    static char textureBuffer[128] = "";

    const int index = editorState.selectedObjectIndex;
    if (index >= 0 && index < static_cast<int>(sceneState.objects.size())) {
        GameObject& obj = sceneState.objects[index];

        if (lastSelectedIndex != index) {
            std::strncpy(textureBuffer, obj.texturePath.c_str(), sizeof(textureBuffer));
            textureBuffer[sizeof(textureBuffer) - 1] = '\0';
            lastSelectedIndex = index;
        }

        std::array<char, 128> nameBuffer{};
        std::snprintf(nameBuffer.data(), nameBuffer.size(), "%s", obj.name.c_str());

        ImGui::TextUnformatted("Selected Object");
        ImGui::Separator();
        ImGui::TextUnformatted("Name");
        if (ImGui::InputText("##InspectorName", nameBuffer.data(), nameBuffer.size())) {
            obj.name = nameBuffer.data();
        }
        ImGui::Text("GameObject ID: %d", obj.id);

        if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("Bind imported project assets or enter a relative texture path.");
            ImGui::TextUnformatted("Texture Path");
            if (ImGui::InputText("##InspectorTexturePath", textureBuffer, sizeof(textureBuffer))) {
                obj.texturePath = textureBuffer;
                const AssetRecord* record = editorState.assetRegistry.findByPath(obj.texturePath);
                obj.textureResourceId = record ? record->id : 0;
            }

            ImGui::TextWrapped("Bound Texture: %s", obj.texturePath.empty() ? "None" : obj.texturePath.c_str());
            if (!obj.texturePath.empty() && ImGui::Button("Ping Texture Asset")) {
                FocusAssetPath(editorState, obj.texturePath);
            }
        }

        if (ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextWrapped("Bound Script: %s", obj.scriptPath.empty() ? "None" : obj.scriptPath.c_str());
            ImGui::TextUnformatted("Script Asset");
            if (ImGui::BeginCombo("##InspectorScriptAsset", obj.scriptPath.empty() ? "None" : obj.scriptPath.c_str())) {
                const bool noScriptSelected = obj.scriptPath.empty();
                if (ImGui::Selectable("None", noScriptSelected)) {
                    obj.scriptResourceId = 0;
                    obj.scriptPath.clear();
                }

                for (const AssetRecord& asset : editorState.assetRegistry.getAssets()) {
                    if (asset.type != AssetType::Script) {
                        continue;
                    }

                    const std::string label = asset.relativePath.empty() ? asset.name : asset.relativePath;
                    const bool selected = (obj.scriptResourceId == asset.id);
                    if (ImGui::Selectable(label.c_str(), selected)) {
                        obj.scriptResourceId = asset.id;
                        obj.scriptPath = asset.sourcePath;
                    }
                }
                ImGui::EndCombo();
            }

            if (!obj.scriptPath.empty() && ImGui::Button("Ping Script Asset")) {
                FocusAssetPath(editorState, obj.scriptPath);
            }

            if (!editorState.scriptStatus.empty()) {
                ImGui::TextWrapped("Script Status: %s", editorState.scriptStatus.c_str());
            }
        }

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextUnformatted("Position");
            ImGui::InputFloat2("##InspectorPosition", obj.position);
            ImGui::TextUnformatted("Scale");
            ImGui::InputFloat2("##InspectorScale", obj.scale);
            ImGui::TextUnformatted("Rotation");
            ImGui::InputFloat("##InspectorRotation", &obj.rotation);

            bool resetPositionPressed = false;
            bool resetScalePressed = false;
            bool resetRotationPressed = false;
            DrawResponsiveInspectorButtonRow(
                "Reset Position",
                "Reset Scale",
                "Reset Rotation",
                resetPositionPressed,
                resetScalePressed,
                resetRotationPressed);

            if (resetPositionPressed) {
                ResetObjectPosition(obj);
                AddEditorLog(editorState, EditorLogLevel::Info, "Reset position from Inspector.");
            }

            if (resetScalePressed) {
                ResetObjectScale(obj);
                AddEditorLog(editorState, EditorLogLevel::Info, "Reset scale from Inspector.");
            }

            if (resetRotationPressed) {
                ResetObjectRotation(obj);
                AddEditorLog(editorState, EditorLogLevel::Info, "Reset rotation from Inspector.");
            }
        }
    }
    else {
        ImGui::TextUnformatted("No object selected");
        ImGui::Separator();
        ImGui::TextWrapped("Select a GameObject in Hierarchy or Scene to edit its transform and bindings here.");
        lastSelectedIndex = -1;
    }

    ImGui::PopItemWidth();
    ImGui::PopTextWrapPos();
    ImGui::End();
}
