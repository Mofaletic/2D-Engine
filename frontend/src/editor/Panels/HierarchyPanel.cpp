#include "HierarchyPanel.h"
#include "imgui.h"

#include <string>

void DrawHierarchyPanel(SceneState& sceneState, EditorState& editorState)
{
    ImGui::Begin("Hierarchy");

    static char filterBuffer[96] = "";

    ImGui::TextUnformatted("Scene Objects");
    ImGui::SameLine();
    ImGui::TextDisabled("(%d)", static_cast<int>(sceneState.objects.size()));
    ImGui::Separator();
    ImGui::InputTextWithHint("##HierarchyFilter", "Search objects", filterBuffer, sizeof(filterBuffer));
    ImGui::TextDisabled("Names stay readable while the panel width changes.");

    ImGui::BeginChild("HierarchyList", ImVec2(0.0f, 0.0f), true);
    {
        for (int i = 0; i < static_cast<int>(sceneState.objects.size()); ++i) {
            if (filterBuffer[0] != '\0' &&
                sceneState.objects[i].name.find(filterBuffer) == std::string::npos) {
                continue;
            }
            const bool selected = (editorState.selectedObjectIndex == i);
            const std::string label = std::to_string(sceneState.objects[i].id) + "  " + sceneState.objects[i].name;
            if (ImGui::Selectable(label.c_str(), selected)) {
                editorState.selectedObjectIndex = i;
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}
