#include "ScenePanel.h"

#include "../EditorActions.h"
#include "../../../../backend/SceneSerializer.h"
#include "imgui.h"

#include <algorithm>
#include <cstdint>

namespace {

constexpr float kSceneObjectBaseSize = 64.0f;
constexpr float kMinSceneViewportHeight = 180.0f;

bool PointInRect(float x, float y, float left, float top, float width, float height) {
    return x >= left && y >= top && x <= left + width && y <= top + height;
}

bool MapScreenToScenePoint(const EditorState& editorState, const ImVec2& screenPos, float& sceneX, float& sceneY) {
    if (editorState.sceneViewportScreenWidth <= 0.0f || editorState.sceneViewportScreenHeight <= 0.0f ||
        editorState.sceneViewportWidth <= 0.0f || editorState.sceneViewportHeight <= 0.0f) {
        return false;
    }

    if (!PointInRect(
        screenPos.x,
        screenPos.y,
        editorState.sceneViewportScreenX,
        editorState.sceneViewportScreenY,
        editorState.sceneViewportScreenWidth,
        editorState.sceneViewportScreenHeight)) {
        return false;
    }

    const float normalizedX = (screenPos.x - editorState.sceneViewportScreenX) / editorState.sceneViewportScreenWidth;
    const float normalizedY = (screenPos.y - editorState.sceneViewportScreenY) / editorState.sceneViewportScreenHeight;
    sceneX = normalizedX * editorState.sceneViewportWidth;
    sceneY = normalizedY * editorState.sceneViewportHeight;
    return true;
}

int FindTopmostObjectAt(const SceneState& sceneState, float sceneX, float sceneY) {
    for (int index = static_cast<int>(sceneState.objects.size()) - 1; index >= 0; --index) {
        const GameObject& object = sceneState.objects[index];
        const float width = kSceneObjectBaseSize * std::max(object.scale[0], 0.0f);
        const float height = kSceneObjectBaseSize * std::max(object.scale[1], 0.0f);
        if (PointInRect(sceneX, sceneY, object.position[0], object.position[1], width, height)) {
            return index;
        }
    }

    return -1;
}

void AssignTextureAssetToObject(SceneState& sceneState, EditorState& editorState, int objectIndex, const AssetRecord& asset) {
    if (objectIndex < 0 || objectIndex >= static_cast<int>(sceneState.objects.size())) {
        editorState.assetStatus = "Dropped texture created a new sprite in the scene";
        return;
    }

    sceneState.objects[objectIndex].textureResourceId = asset.id;
    sceneState.objects[objectIndex].texturePath = !asset.relativePath.empty() ? asset.relativePath : asset.sourcePath;
    editorState.selectedObjectIndex = objectIndex;
    editorState.assetStatus = "Bound texture via drag: " + asset.name;
    AddEditorLog(editorState, EditorLogLevel::Info, editorState.assetStatus);
}

void AssignScriptAssetToObject(SceneState& sceneState, EditorState& editorState, int objectIndex, const AssetRecord& asset) {
    if (objectIndex < 0 || objectIndex >= static_cast<int>(sceneState.objects.size())) {
        editorState.assetStatus = "Drop a script onto a scene object or select one first";
        AddEditorLog(editorState, EditorLogLevel::Warning, editorState.assetStatus);
        return;
    }

    sceneState.objects[objectIndex].scriptResourceId = asset.id;
    sceneState.objects[objectIndex].scriptPath = asset.sourcePath;
    editorState.selectedObjectIndex = objectIndex;
    editorState.assetStatus = "Bound script via drag: " + asset.name;
    AddEditorLog(editorState, EditorLogLevel::Info, editorState.assetStatus);
}

void DrawSceneToolbar(SceneState& sceneState, EditorState& editorState, int index) {
    const bool hasSelection = index >= 0 && index < static_cast<int>(sceneState.objects.size());
    const bool hasProject = !editorState.sceneFilePath.empty();

    ImGui::BeginDisabled(!hasProject);
    if (ImGui::Button("Save Scene")) {
        std::string sceneName = "UntitledScene";
        if (hasSelection) {
            sceneName = sceneState.objects[index].name;
        } else if (!editorState.projectName.empty()) {
            sceneName = editorState.projectName;
        }

        if (SaveSceneToFile(sceneState, sceneName, editorState.sceneFilePath)) {
            AddEditorLog(editorState, EditorLogLevel::Info, "Saved scene: " + editorState.sceneFilePath);
        } else {
            AddEditorLog(editorState, EditorLogLevel::Error, "Failed to save scene: " + editorState.sceneFilePath);
        }
    }
    ImGui::EndDisabled();

    if (!hasProject && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("Create or open a project before saving the scene.");
    }

    ImGui::SameLine();
    if (ImGui::Button("Add Empty")) {
        CreateEmptyObject(sceneState, editorState, "GameObject", "Scene panel");
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!hasSelection);
    if (ImGui::Button("Delete Selected")) {
        DeleteSelectedObject(sceneState, editorState);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Position")) {
        ResetObjectPosition(sceneState.objects[index]);
        AddEditorLog(editorState, EditorLogLevel::Info, "Reset selected object position.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Scale")) {
        ResetObjectScale(sceneState.objects[index]);
        AddEditorLog(editorState, EditorLogLevel::Info, "Reset selected object scale.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Rotation")) {
        ResetObjectRotation(sceneState.objects[index]);
        AddEditorLog(editorState, EditorLogLevel::Info, "Reset selected object rotation.");
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (hasSelection) {
        ImGui::TextDisabled("Selected: %s", sceneState.objects[index].name.c_str());
    } else {
        ImGui::TextDisabled("Selected: None");
    }
}

}

void DrawScenePanel(SceneState& sceneState, EditorState& editorState, const SceneViewportImage& sceneImage)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

    ImGui::Begin(kScenePanelWindowName, &editorState.showScene);

    const int index = editorState.selectedObjectIndex;

    if (index >= 0 && index >= static_cast<int>(sceneState.objects.size())) {
        editorState.selectedObjectIndex = -1;
    }

    DrawSceneToolbar(sceneState, editorState, editorState.selectedObjectIndex);

    ImGui::Separator();

    ImVec2 availableRegion = ImGui::GetContentRegionAvail();
    ImVec2 viewportSize(
        std::max(availableRegion.x, 1.0f),
        std::max(availableRegion.y, kMinSceneViewportHeight));

    editorState.sceneViewportWidth = viewportSize.x;
    editorState.sceneViewportHeight = viewportSize.y;
    editorState.sceneViewportScreenWidth = 0.0f;
    editorState.sceneViewportScreenHeight = 0.0f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 255));
    ImGui::BeginChild("SceneViewport", viewportSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const ImVec2 childScreenPos = ImGui::GetCursorScreenPos();
    editorState.sceneViewportScreenX = childScreenPos.x;
    editorState.sceneViewportScreenY = childScreenPos.y;
    editorState.sceneViewportScreenWidth = viewportSize.x;
    editorState.sceneViewportScreenHeight = viewportSize.y;

    if (sceneImage.Handle != nullptr) {
        float textureWidth = sceneImage.Width > 0.0f ? sceneImage.Width : viewportSize.x;
        float textureHeight = sceneImage.Height > 0.0f ? sceneImage.Height : viewportSize.y;

        float scale = std::min(viewportSize.x / textureWidth, viewportSize.y / textureHeight);
        scale = (scale > 0.0f) ? scale : 1.0f;

        ImVec2 imageSize(textureWidth * scale, textureHeight * scale);
        ImVec2 cursor = ImGui::GetCursorPos();
        float offsetX = (viewportSize.x - imageSize.x) * 0.5f;
        float offsetY = (viewportSize.y - imageSize.y) * 0.5f;
        ImGui::SetCursorPos(ImVec2(cursor.x + std::max(0.0f, offsetX), cursor.y + std::max(0.0f, offsetY)));
        const ImVec2 imageScreenPos = ImGui::GetCursorScreenPos();
        editorState.sceneViewportScreenX = imageScreenPos.x;
        editorState.sceneViewportScreenY = imageScreenPos.y;
        editorState.sceneViewportScreenWidth = imageSize.x;
        editorState.sceneViewportScreenHeight = imageSize.y;
        ImGui::Image((ImTextureID)sceneImage.Handle, imageSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

        if (ImGui::BeginDragDropTarget()) {
            int dropTargetIndex = editorState.selectedObjectIndex;
            float sceneX = 0.0f;
            float sceneY = 0.0f;
            const bool hasScenePoint = MapScreenToScenePoint(editorState, ImGui::GetIO().MousePos, sceneX, sceneY);
            if (hasScenePoint) {
                const int hoveredObject = FindTopmostObjectAt(sceneState, sceneX, sceneY);
                if (hoveredObject >= 0) {
                    dropTargetIndex = hoveredObject;
                } else {
                    dropTargetIndex = -1;
                }
            }

            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE_ID")) {
                const std::uint64_t assetId = *static_cast<const std::uint64_t*>(payload->Data);
                if (const AssetRecord* asset = editorState.assetRegistry.findById(assetId)) {
                    if (dropTargetIndex >= 0) {
                        AssignTextureAssetToObject(sceneState, editorState, dropTargetIndex, *asset);
                    } else if (hasScenePoint) {
                        CreateObjectFromAsset(sceneState, editorState, *asset, sceneX, sceneY, "Scene drag-drop");
                    }
                }
            }

            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_SCRIPT_ID")) {
                const std::uint64_t assetId = *static_cast<const std::uint64_t*>(payload->Data);
                if (const AssetRecord* asset = editorState.assetRegistry.findById(assetId)) {
                    AssignScriptAssetToObject(sceneState, editorState, dropTargetIndex, *asset);
                }
            }

            ImGui::EndDragDropTarget();
        }
    }
    else {
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(childScreenPos.x + 12.0f, childScreenPos.y + 12.0f),
            IM_COL32(220, 220, 220, 255),
            "Scene viewport is initializing..."
        );
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleVar(2);
}
