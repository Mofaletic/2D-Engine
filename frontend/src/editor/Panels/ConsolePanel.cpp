#include "ConsolePanel.h"
#include "imgui.h"

void DrawConsolePanel()
{
    ImGui::Begin("Console");

    ImGui::TextUnformatted("Console");
    ImGui::Separator();

    ImGui::BeginChild("ConsoleLog", ImVec2(0.0f, 0.0f), true);
    {
        ImGui::TextColored(ImVec4(0.58f, 0.82f, 1.0f, 1.0f), "[INFO]");
        ImGui::SameLine();
        ImGui::TextWrapped("Editor UI initialized and ready.");

        ImGui::TextColored(ImVec4(0.67f, 0.86f, 0.62f, 1.0f), "[ASSET]");
        ImGui::SameLine();
        ImGui::TextWrapped("Imported resources and scene changes will appear here in a fuller runtime logging pass.");

        ImGui::TextColored(ImVec4(0.95f, 0.78f, 0.42f, 1.0f), "[TIP]");
        ImGui::SameLine();
        ImGui::TextWrapped("Project and Console share the lower dock as tabs, similar to Unity's editor layout.");
    }
    ImGui::EndChild();

    ImGui::End();
}
