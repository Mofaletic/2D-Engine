#include "AssetPanel.h"
#include "imgui.h"

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#endif

#include <filesystem>
#include <string>
#include <vector>

namespace {

#ifdef _WIN32
std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) {
        return {};
    }

    std::string result(static_cast<std::size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size - 1, nullptr, nullptr);
    return result;
}

std::vector<std::string> PickFilesFromNativeDialog() {
    std::vector<std::string> filePaths;

    HRESULT initResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool shouldUninitialize = SUCCEEDED(initResult);

    IFileOpenDialog* dialog = nullptr;
    HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dialog));
    if (FAILED(result) || dialog == nullptr) {
        if (shouldUninitialize) {
            CoUninitialize();
        }
        return filePaths;
    }

    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_ALLOWMULTISELECT | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
    dialog->SetTitle(L"Import Image Files");

    COMDLG_FILTERSPEC filters[] = {
        { L"Image Files", L"*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tga;*.webp" },
        { L"All Files", L"*.*" }
    };
    dialog->SetFileTypes(2, filters);

    result = dialog->Show(nullptr);
    if (FAILED(result)) {
        dialog->Release();
        if (shouldUninitialize) {
            CoUninitialize();
        }
        return filePaths;
    }

    IShellItemArray* items = nullptr;
    result = dialog->GetResults(&items);
    if (SUCCEEDED(result) && items != nullptr) {
        DWORD count = 0;
        items->GetCount(&count);
        for (DWORD i = 0; i < count; ++i) {
            IShellItem* item = nullptr;
            if (SUCCEEDED(items->GetItemAt(i, &item)) && item != nullptr) {
                PWSTR path = nullptr;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path != nullptr) {
                    filePaths.push_back(WideToUtf8(path));
                    CoTaskMemFree(path);
                }
                item->Release();
            }
        }
        items->Release();
    }

    dialog->Release();
    if (shouldUninitialize) {
        CoUninitialize();
    }

    return filePaths;
}

bool PickFolderFromNativeDialog(std::string& folderPath) {
    HRESULT initResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool shouldUninitialize = SUCCEEDED(initResult);

    IFileOpenDialog* dialog = nullptr;
    HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dialog));
    if (FAILED(result) || dialog == nullptr) {
        if (shouldUninitialize) {
            CoUninitialize();
        }
        return false;
    }

    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    dialog->SetTitle(L"Import Asset Folder");

    result = dialog->Show(nullptr);
    if (FAILED(result)) {
        dialog->Release();
        if (shouldUninitialize) {
            CoUninitialize();
        }
        return false;
    }

    IShellItem* item = nullptr;
    result = dialog->GetResult(&item);
    if (SUCCEEDED(result) && item != nullptr) {
        PWSTR path = nullptr;
        result = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
        if (SUCCEEDED(result) && path != nullptr) {
            folderPath = WideToUtf8(path);
            CoTaskMemFree(path);
        }
        item->Release();
    }

    dialog->Release();
    if (shouldUninitialize) {
        CoUninitialize();
    }

    return !folderPath.empty();
}
#else
std::vector<std::string> PickFilesFromNativeDialog() {
    return {};
}

bool PickFolderFromNativeDialog(std::string& folderPath) {
    folderPath.clear();
    return false;
}
#endif

void AssignAssetToSelection(SceneState& sceneState, EditorState& editorState, const AssetRecord& asset) {
    const int index = editorState.selectedObjectIndex;
    const bool hasSelection = (index >= 0 && index < static_cast<int>(sceneState.objects.size()));
    if (!hasSelection) {
        return;
    }

    sceneState.objects[index].textureResourceId = asset.id;
    sceneState.objects[index].texturePath = !asset.relativePath.empty() ? asset.relativePath : asset.sourcePath;
}

}  // namespace

void DrawAssetPanel(SceneState& sceneState, EditorState& editorState)
{
    ImGui::Begin("Project");

    static char filterBuffer[96] = "";

    ImGui::TextUnformatted("Project Assets");
    ImGui::TextWrapped("Import textures into the project library, then bind a registered resource to the selected object.");
    ImGui::Separator();

    int index = editorState.selectedObjectIndex;
    bool hasSelection = (index >= 0 && index < static_cast<int>(sceneState.objects.size()));
    const float width = ImGui::GetContentRegionAvail().x;
    const bool compact = width < 760.0f;

    if (ImGui::Button("Import Images")) {
        const std::vector<std::string> filePaths = PickFilesFromNativeDialog();
        if (!filePaths.empty()) {
            const std::size_t importedCount = editorState.assetRegistry.importFilesToProject(filePaths);
            if (importedCount > 0) {
                editorState.assetRegistry.saveManifest("asset/asset_registry.json");
                editorState.assetStatus =
                    "Imported " + std::to_string(importedCount) + " image(s) into project assets";
            } else {
                editorState.assetStatus = editorState.assetRegistry.getLastError();
            }
        } else {
            editorState.assetStatus = "Image import canceled";
        }
    }

    if (!compact) ImGui::SameLine();
    if (ImGui::Button("Import Folder")) {
        std::string folderPath;
        if (PickFolderFromNativeDialog(folderPath)) {
            const std::size_t importedCount = editorState.assetRegistry.importFolderToProject(folderPath);
            if (importedCount > 0) {
                editorState.assetRegistry.saveManifest("asset/asset_registry.json");
                editorState.assetStatus =
                    "Imported " + std::to_string(importedCount) + " asset(s) into project from " + folderPath;
            }
            else {
                const std::string& error = editorState.assetRegistry.getLastError();
                editorState.assetStatus = error.empty()
                    ? "Selected folder contains no new supported asset files"
                    : error;
            }
        }
        else {
            editorState.assetStatus = "Import canceled";
        }
    }

    if (!compact) ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        const std::size_t refreshedCount = editorState.assetRegistry.rebuildFromProjectAssets();
        editorState.assetRegistry.saveManifest("asset/asset_registry.json");
        editorState.assetStatus =
            "Refreshed project asset library, found " + std::to_string(refreshedCount) + " importable file(s)";
    }

    if (!compact) ImGui::SameLine();
    ImGui::Text("Registered: %d", static_cast<int>(editorState.assetRegistry.getAssetCount()));
    ImGui::InputTextWithHint("##AssetFilter", "Search assets", filterBuffer, sizeof(filterBuffer));
    ImGui::TextWrapped("%s", editorState.assetStatus.c_str());
    if (!editorState.assetRegistry.getLastImportedFolder().empty()) {
        ImGui::TextWrapped("Last Folder: %s", editorState.assetRegistry.getLastImportedFolder().c_str());
    }
    ImGui::TextWrapped("Project Asset Root: %s", editorState.assetRegistry.getProjectAssetRoot().c_str());

    if (hasSelection) {
        ImGui::Text("Selected Object: %s", sceneState.objects[index].name.c_str());
        ImGui::Text("Current Resource ID: %llu",
            static_cast<unsigned long long>(sceneState.objects[index].textureResourceId));
        ImGui::Text("Current Texture: %s", sceneState.objects[index].texturePath.c_str());
    }
    else {
        ImGui::Text("No object selected");
    }

    ImGui::Separator();

    if (!hasSelection) {
        ImGui::BeginDisabled();
    }

    const auto& assets = editorState.assetRegistry.getAssets();
    ImGui::BeginChild("AssetList", ImVec2(0.0f, 180.0f), true);
    {
        if (ImGui::BeginTable("AssetTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 52.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.4f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 86.0f);
            ImGui::TableHeadersRow();

            for (const AssetRecord& asset : assets) {
                if (filterBuffer[0] != '\0' &&
                    asset.name.find(filterBuffer) == std::string::npos &&
                    asset.relativePath.find(filterBuffer) == std::string::npos) {
                    continue;
                }
                const bool selected = hasSelection && (sceneState.objects[index].textureResourceId == asset.id);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%llu", static_cast<unsigned long long>(asset.id));
                ImGui::TableNextColumn();
                if (ImGui::Selectable(asset.name.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
                    AssignAssetToSelection(sceneState, editorState, asset);
                }
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(asset.typeName.c_str());

                if (ImGui::IsItemHovered() || ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::BeginTooltip();
                    ImGui::Text("ID: %llu", static_cast<unsigned long long>(asset.id));
                    ImGui::Text("Type: %s", asset.typeName.c_str());
                    ImGui::TextWrapped("Source: %s", asset.sourcePath.c_str());
                    ImGui::TextWrapped("Path: %s", asset.relativePath.c_str());
                    ImGui::EndTooltip();
                }
            }

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

    if (!hasSelection) {
        ImGui::EndDisabled();
    }

    ImGui::Separator();
    ImGui::Text("Preview");
    if (hasSelection) {
        ImGui::BulletText("Resource ID: %llu",
            static_cast<unsigned long long>(sceneState.objects[index].textureResourceId));
        ImGui::BulletText("Texture file: %s", sceneState.objects[index].texturePath.c_str());
    }
    else {
        ImGui::BulletText("No texture selected");
    }

    ImGui::End();
}
