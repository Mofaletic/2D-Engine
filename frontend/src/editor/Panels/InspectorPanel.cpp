#include "InspectorPanel.h"

#include "../EditorActions.h"
#include "imgui.h"

#include <array>
#include <cstdio>
#include <cstring>

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
        if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size())) {
            obj.name = nameBuffer.data();
        }
        ImGui::Text("GameObject ID: %d", obj.id);

        if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("Bind imported project assets or enter a relative texture path.");
            if (ImGui::InputText("Texture Path", textureBuffer, sizeof(textureBuffer))) {
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
            if (ImGui::BeginCombo("Script Asset", obj.scriptPath.empty() ? "None" : obj.scriptPath.c_str())) {
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
            ImGui::InputFloat2("Position", obj.position);
            ImGui::InputFloat2("Scale", obj.scale);
            ImGui::InputFloat("Rotation", &obj.rotation);

            if (ImGui::Button("Reset Position")) {
                ResetObjectPosition(obj);
                AddEditorLog(editorState, EditorLogLevel::Info, "Reset position from Inspector.");
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Scale")) {
                ResetObjectScale(obj);
                AddEditorLog(editorState, EditorLogLevel::Info, "Reset scale from Inspector.");
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Rotation")) {
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
