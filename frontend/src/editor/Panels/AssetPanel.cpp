#include "AssetPanel.h"

#include "../EditorActions.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "../../../../backend/SceneSerializer.h"
#include "../../../../backend/project/ProjectManager.h"
#include "../../../../backend/resource/ResourcePathUtils.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shobjidl.h>
#endif

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr std::size_t kEditorBufferSize = 64 * 1024;
constexpr float kTreeWidth = 260.0f;
constexpr float kBottomPanelHeight = 210.0f;
constexpr float kTileWidth = 92.0f;
constexpr float kTileHeight = 118.0f;

struct BrowserState {
    std::string projectRoot;
    std::string currentDirectory;
    std::string selectedPath;
    std::string loadedTextPath;
    std::vector<char> textBuffer = std::vector<char>(kEditorBufferSize, '\0');
    bool textDirty = false;
    std::string pendingCreatePath;
    std::string pendingDeletePath;
    std::string pendingRenamePath;
    bool openCreateNamePopup = false;
    bool openDeletePopup = false;
    bool openRenamePopup = false;
    char createItemName[128] = "NewItem";
    char renameItemName[128] = "";
    char searchBuffer[128] = "";
};

#ifdef _WIN32
std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return {};
    std::string result(static_cast<std::size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size - 1, nullptr, nullptr);
    return result;
}

std::vector<std::string> PickFilesFromNativeDialog() {
    std::vector<std::string> paths;
    HRESULT initResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool shouldUninitialize = SUCCEEDED(initResult);
    IFileOpenDialog* dialog = nullptr;
    HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dialog));
    if (FAILED(result) || dialog == nullptr) {
        if (shouldUninitialize) CoUninitialize();
        return paths;
    }

    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_ALLOWMULTISELECT | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
    dialog->SetTitle(L"Import Project Files");
    result = dialog->Show(nullptr);
    if (SUCCEEDED(result)) {
        IShellItemArray* items = nullptr;
        if (SUCCEEDED(dialog->GetResults(&items)) && items != nullptr) {
            DWORD count = 0;
            items->GetCount(&count);
            for (DWORD i = 0; i < count; ++i) {
                IShellItem* item = nullptr;
                if (SUCCEEDED(items->GetItemAt(i, &item)) && item != nullptr) {
                    PWSTR path = nullptr;
                    if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path != nullptr) {
                        paths.push_back(WideToUtf8(path));
                        CoTaskMemFree(path);
                    }
                    item->Release();
                }
            }
            items->Release();
        }
    }

    dialog->Release();
    if (shouldUninitialize) CoUninitialize();
    return paths;
}

bool PickFolderFromNativeDialog(std::string& folderPath) {
    folderPath.clear();
    HRESULT initResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool shouldUninitialize = SUCCEEDED(initResult);
    IFileOpenDialog* dialog = nullptr;
    HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&dialog));
    if (FAILED(result) || dialog == nullptr) {
        if (shouldUninitialize) CoUninitialize();
        return false;
    }

    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    result = dialog->Show(nullptr);
    if (SUCCEEDED(result)) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item)) && item != nullptr) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path != nullptr) {
                folderPath = WideToUtf8(path);
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }

    dialog->Release();
    if (shouldUninitialize) CoUninitialize();
    return !folderPath.empty();
}
#else
std::vector<std::string> PickFilesFromNativeDialog() { return {}; }
bool PickFolderFromNativeDialog(std::string& folderPath) { folderPath.clear(); return false; }
#endif

std::string NormalizePath(const fs::path& path) {
    return ResourcePathUtils::NormalizePathString(path);
}

ProjectDescriptor BuildProjectDescriptor(const EditorState& editorState) {
    ProjectDescriptor project;
    project.name = editorState.projectName;
    project.rootPath = editorState.projectRootPath;
    project.assetRootPath = editorState.assetRegistry.getProjectAssetRoot();
    project.assetManifestPath = editorState.assetManifestPath;
    project.defaultScenePath = editorState.sceneFilePath;
    project.projectFilePath = editorState.projectFilePath;
    return project;
}

ProjectItemType ToProjectItemType(int index) {
    if (index == 0) return ProjectItemType::Audio;
    if (index == 2) return ProjectItemType::Text;
    if (index == 3) return ProjectItemType::Scene;
    if (index == 4) return ProjectItemType::Script;
    return ProjectItemType::Image;
}

const char* ProjectItemTypeLabel(int index) {
    if (index == 0) return "Audio";
    if (index == 2) return "Text";
    if (index == 3) return "Scene";
    if (index == 4) return "C++ Script";
    return "Image";
}

bool IsSceneFile(const fs::path& path) {
    const std::string name = path.filename().generic_string();
    return name.size() >= 11 && name.substr(name.size() - 11) == ".scene.json";
}

bool IsTextEditable(const fs::path& path) {
    const std::string ext = path.extension().generic_string();
    return IsSceneFile(path) || ext == ".txt" || ext == ".json" || ext == ".md" || ext == ".ini" || ext == ".cpp" || ext == ".h" || ext == ".hpp";
}

bool IsSamePathOrChild(const std::string& candidatePath, const std::string& parentPath) {
    if (candidatePath.empty() || parentPath.empty()) return false;
    if (candidatePath == parentPath) return true;
    return candidatePath.size() > parentPath.size() &&
        candidatePath.compare(0, parentPath.size(), parentPath) == 0 &&
        candidatePath[parentPath.size()] == '/';
}

std::string ReplacePathPrefix(const std::string& value, const std::string& oldPrefix, const std::string& newPrefix) {
    if (!IsSamePathOrChild(value, oldPrefix)) return value;
    if (value == oldPrefix) return newPrefix;
    return newPrefix + value.substr(oldPrefix.size());
}

std::string RelativeToProject(const EditorState& editorState, const fs::path& path) {
    std::error_code ec;
    const fs::path relative = fs::relative(path, ResourcePathUtils::Utf8ToPath(editorState.projectRootPath), ec);
    return ec ? path.filename().generic_string() : relative.generic_string();
}

std::string RelativeToProject(const EditorState& editorState, const std::string& path) {
    return RelativeToProject(editorState, ResourcePathUtils::Utf8ToPath(path));
}

std::string RelativeDirectoryLabel(const EditorState& editorState, const std::string& path) {
    if (path.empty() || editorState.projectRootPath.empty()) return {};
    const std::string normalizedRoot = NormalizePath(ResourcePathUtils::Utf8ToPath(editorState.projectRootPath));
    const std::string normalizedPath = NormalizePath(ResourcePathUtils::Utf8ToPath(path));
    if (normalizedPath == normalizedRoot) return editorState.projectName.empty() ? "Project Root" : editorState.projectName;
    const std::string relative = RelativeToProject(editorState, normalizedPath);
    return relative.empty() ? "Project Root" : relative;
}

void ResetBrowserState(BrowserState& state, const EditorState& editorState) {
    state.projectRoot = editorState.projectRootPath;
    state.currentDirectory = editorState.projectRootPath;
    state.selectedPath.clear();
    state.loadedTextPath.clear();
    std::fill(state.textBuffer.begin(), state.textBuffer.end(), '\0');
    state.textDirty = false;
    state.pendingCreatePath.clear();
    state.pendingDeletePath.clear();
    state.pendingRenamePath.clear();
    state.openCreateNamePopup = false;
    state.openDeletePopup = false;
    state.openRenamePopup = false;
    std::strcpy(state.createItemName, "NewItem");
    state.renameItemName[0] = '\0';
    state.searchBuffer[0] = '\0';
}

void EnsureBrowserState(BrowserState& state, const EditorState& editorState) {
    if (state.projectRoot != editorState.projectRootPath) ResetBrowserState(state, editorState);
    if (!state.currentDirectory.empty() && !fs::exists(ResourcePathUtils::Utf8ToPath(state.currentDirectory))) state.currentDirectory = editorState.projectRootPath;
    if (!state.selectedPath.empty() && !fs::exists(ResourcePathUtils::Utf8ToPath(state.selectedPath))) state.selectedPath.clear();
}

void ApplyFocusedAssetSelection(BrowserState& state, EditorState& editorState) {
    if (editorState.focusAssetPath.empty()) return;
    const AssetRecord* asset = editorState.assetRegistry.findByPath(editorState.focusAssetPath);
    std::string resolvedPath = asset ? asset->sourcePath : editorState.focusAssetPath;
    if (resolvedPath.empty()) {
        editorState.focusAssetPath.clear();
        return;
    }

    resolvedPath = NormalizePath(ResourcePathUtils::Utf8ToPath(resolvedPath));
    std::error_code ec;
    const fs::path resolvedFsPath = ResourcePathUtils::Utf8ToPath(resolvedPath);
    if (!fs::exists(resolvedFsPath, ec)) {
        editorState.focusAssetPath.clear();
        return;
    }

    state.selectedPath = resolvedPath;
    state.currentDirectory = fs::is_directory(resolvedFsPath, ec) ? resolvedPath : NormalizePath(resolvedFsPath.parent_path());
    editorState.focusAssetPath.clear();
}

bool LoadTextFile(const std::string& path, std::vector<char>& buffer) {
    std::ifstream file(ResourcePathUtils::Utf8ToPath(path), std::ios::binary);
    if (!file.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (content.size() >= buffer.size()) content.resize(buffer.size() - 1);
    std::fill(buffer.begin(), buffer.end(), '\0');
    std::memcpy(buffer.data(), content.data(), content.size());
    return true;
}

bool SaveTextFile(const std::string& path, const std::vector<char>& buffer) {
    std::ofstream file(ResourcePathUtils::Utf8ToPath(path), std::ios::binary | std::ios::trunc);
    if (!file.is_open()) return false;
    file.write(buffer.data(), static_cast<std::streamsize>(std::strlen(buffer.data())));
    return true;
}

void RefreshProjectAssets(EditorState& editorState) {
    editorState.assetRegistry.synchronizeProjectAssets();
    if (!editorState.assetManifestPath.empty()) editorState.assetRegistry.saveManifest(editorState.assetManifestPath);
    editorState.pendingProjectCommand = ProjectCommand::Sync;
}

void ClearSelectionIfDeleted(BrowserState& state, const std::string& deletedPath, const std::string& projectRootPath) {
    if (IsSamePathOrChild(state.selectedPath, deletedPath)) state.selectedPath.clear();
    if (IsSamePathOrChild(state.loadedTextPath, deletedPath)) {
        state.loadedTextPath.clear();
        state.textDirty = false;
        std::fill(state.textBuffer.begin(), state.textBuffer.end(), '\0');
    }
    if (IsSamePathOrChild(state.currentDirectory, deletedPath)) state.currentDirectory = projectRootPath;
}

void UpdateBrowserPathsAfterMove(BrowserState& state, const std::string& sourcePath, const std::string& movedPath) {
    state.selectedPath = ReplacePathPrefix(state.selectedPath, sourcePath, movedPath);
    state.loadedTextPath = ReplacePathPrefix(state.loadedTextPath, sourcePath, movedPath);
    state.currentDirectory = ReplacePathPrefix(state.currentDirectory, sourcePath, movedPath);
    state.pendingCreatePath = ReplacePathPrefix(state.pendingCreatePath, sourcePath, movedPath);
    state.pendingDeletePath = ReplacePathPrefix(state.pendingDeletePath, sourcePath, movedPath);
    state.pendingRenamePath = ReplacePathPrefix(state.pendingRenamePath, sourcePath, movedPath);
}

void RebindSceneReferencesAfterMove(SceneState& sceneState, EditorState& editorState, const std::string& sourcePath, const std::string& movedPath) {
    const std::string oldRelative = RelativeToProject(editorState, sourcePath);
    const std::string newRelative = RelativeToProject(editorState, movedPath);

    editorState.sceneFilePath = ReplacePathPrefix(editorState.sceneFilePath, sourcePath, movedPath);
    editorState.focusAssetPath = ReplacePathPrefix(editorState.focusAssetPath, sourcePath, movedPath);

    for (GameObject& object : sceneState.objects) {
        object.texturePath = ReplacePathPrefix(object.texturePath, sourcePath, movedPath);
        object.texturePath = ReplacePathPrefix(object.texturePath, oldRelative, newRelative);
        object.scriptPath = ReplacePathPrefix(object.scriptPath, sourcePath, movedPath);
        object.scriptPath = ReplacePathPrefix(object.scriptPath, oldRelative, newRelative);

        if (!object.texturePath.empty()) {
            const AssetRecord* texture = editorState.assetRegistry.findByPath(object.texturePath);
            object.textureResourceId = texture && texture->type == AssetType::Texture ? texture->id : 0;
        }
        if (!object.scriptPath.empty()) {
            const AssetRecord* script = editorState.assetRegistry.findByPath(object.scriptPath);
            object.scriptResourceId = script && script->type == AssetType::Script ? script->id : 0;
        }
    }
}

void DeletePendingEntry(BrowserState& state, EditorState& editorState) {
    std::string error;
    const std::string deletedPath = state.pendingDeletePath;
    if (ProjectManager::DeleteProjectEntry(BuildProjectDescriptor(editorState), deletedPath, error)) {
        RefreshProjectAssets(editorState);
        editorState.assetStatus = "Deleted: " + deletedPath;
        ClearSelectionIfDeleted(state, deletedPath, editorState.projectRootPath);
        if (!editorState.sceneFilePath.empty() && IsSamePathOrChild(editorState.sceneFilePath, deletedPath)) {
            editorState.projectStatus = "The currently opened scene was deleted from disk";
            editorState.sceneFilePath.clear();
        }
    } else {
        editorState.assetStatus = error.empty() ? "Failed to delete entry" : error;
    }
}

void QueueDeleteForPath(BrowserState& state, const std::string& path) {
    state.pendingDeletePath = path;
    state.openDeletePopup = true;
}

void QueueCreateNameForPath(BrowserState& state, const std::string& path) {
    state.pendingCreatePath = path;
    const fs::path filePath = ResourcePathUtils::Utf8ToPath(path);
    const std::string filename = filePath.filename().string();
    const std::string baseName = IsSceneFile(filePath) ? filename.substr(0, filename.size() - 11) : filePath.stem().string();
    std::strncpy(state.createItemName, baseName.c_str(), sizeof(state.createItemName));
    state.createItemName[sizeof(state.createItemName) - 1] = '\0';
    state.openCreateNamePopup = true;
}

void ClearCreateNamePopupState(BrowserState& state) {
    state.pendingCreatePath.clear();
    state.openCreateNamePopup = false;
    std::strcpy(state.createItemName, "NewItem");
}

void QueueRenameForPath(BrowserState& state, const std::string& path) {
    state.pendingRenamePath = path;
    const fs::path filePath = ResourcePathUtils::Utf8ToPath(path);
    const std::string filename = filePath.filename().string();
    const std::string baseName = IsSceneFile(filePath) ? filename.substr(0, filename.size() - 11) : filePath.stem().string();
    std::strncpy(state.renameItemName, baseName.c_str(), sizeof(state.renameItemName));
    state.renameItemName[sizeof(state.renameItemName) - 1] = '\0';
    state.openRenamePopup = true;
}

void ClearRenamePopupState(BrowserState& state) {
    state.pendingRenamePath.clear();
    state.renameItemName[0] = '\0';
    state.openRenamePopup = false;
}

std::string GetEditableBaseName(const std::string& path) {
    const fs::path filePath = ResourcePathUtils::Utf8ToPath(path);
    const std::string filename = filePath.filename().string();
    return IsSceneFile(filePath) ? filename.substr(0, filename.size() - 11) : filePath.stem().string();
}

void BeginAssetPayloadDragSource(const AssetRecord* asset) {
    if (asset == nullptr) return;
    const char* payloadType = nullptr;
    if (asset->type == AssetType::Texture) payloadType = "ASSET_TEXTURE_ID";
    else if (asset->type == AssetType::Script) payloadType = "ASSET_SCRIPT_ID";
    if (payloadType == nullptr) return;

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoDisableHover)) {
        const std::uint64_t assetId = asset->id;
        ImGui::SetDragDropPayload(payloadType, &assetId, sizeof(assetId));
        ImGui::TextUnformatted(asset->name.c_str());
        ImGui::TextDisabled("%s", asset->typeName.c_str());
        ImGui::EndDragDropSource();
    }
}

void BeginProjectEntryDragSource(const std::string& path, bool isDirectory) {
    if (path.empty()) return;
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoDisableHover)) {
        ImGui::SetDragDropPayload("PROJECT_ENTRY_PATH", path.c_str(), path.size() + 1);
        ImGui::TextUnformatted(fs::path(ResourcePathUtils::Utf8ToPath(path)).filename().generic_string().c_str());
        ImGui::TextDisabled("%s", isDirectory ? "Folder" : "File");
        ImGui::EndDragDropSource();
    }
}

void AssignAssetToSelection(SceneState& sceneState, EditorState& editorState, const AssetRecord& asset) {
    const int index = editorState.selectedObjectIndex;
    const bool hasSelection = (index >= 0 && index < static_cast<int>(sceneState.objects.size()));
    if (!hasSelection) {
        editorState.assetStatus = "Select an object before binding a texture";
        return;
    }
    if (asset.type != AssetType::Texture) {
        editorState.assetStatus = "Only texture assets can be bound to scene objects";
        return;
    }
    sceneState.objects[index].textureResourceId = asset.id;
    sceneState.objects[index].texturePath = !asset.relativePath.empty() ? asset.relativePath : asset.sourcePath;
    editorState.assetStatus = "Bound texture: " + asset.name;
}

bool OpenScene(SceneState& sceneState, EditorState& editorState, const std::string& path) {
    std::string sceneName;
    if (!LoadSceneFromFile(sceneState, editorState, sceneName, path)) {
        editorState.projectStatus = "Failed to open scene: " + path;
        return false;
    }
    editorState.sceneFilePath = path;
    editorState.projectStatus = "Opened scene: " + sceneName;
    return true;
}

void CreateItemAndStartRename(BrowserState& state, EditorState& editorState, int typeIndex, const std::string& directory) {
    std::string createdPath;
    std::string error;
    if (ProjectManager::CreateProjectItemInDirectory(BuildProjectDescriptor(editorState), directory, ToProjectItemType(typeIndex), "NewItem", createdPath, error)) {
        RefreshProjectAssets(editorState);
        editorState.assetStatus = "Created: " + createdPath;
        state.selectedPath = createdPath;
        state.currentDirectory = directory;
        QueueCreateNameForPath(state, createdPath);
    } else {
        editorState.assetStatus = error.empty() ? "Failed to create item" : error;
    }
}

bool MoveProjectEntry(SceneState& sceneState, BrowserState& state, EditorState& editorState, const std::string& sourcePath, const std::string& targetDirectory) {
    if (sourcePath.empty() || targetDirectory.empty()) return false;
    std::string movedPath;
    std::string error;
    if (!ProjectManager::MoveProjectEntry(BuildProjectDescriptor(editorState), sourcePath, targetDirectory, movedPath, error)) {
        editorState.assetStatus = error.empty() ? "Failed to move entry" : error;
        return false;
    }
    if (movedPath == sourcePath) return false;

    RefreshProjectAssets(editorState);
    UpdateBrowserPathsAfterMove(state, sourcePath, movedPath);
    RebindSceneReferencesAfterMove(sceneState, editorState, sourcePath, movedPath);
    state.selectedPath = movedPath;
    editorState.assetStatus = "Moved: " + RelativeToProject(editorState, movedPath);
    AddEditorLog(editorState, EditorLogLevel::Info, "Moved project entry to '" + RelativeToProject(editorState, movedPath) + "'");
    return true;
}

AssetType GuessAssetTypeFromPath(const fs::path& path, const AssetRecord* asset) {
    if (asset != nullptr) {
        return asset->type;
    }

    const std::string filename = path.filename().generic_string();
    if (filename.size() >= 11 && filename.substr(filename.size() - 11) == ".scene.json") {
        return AssetType::Scene;
    }

    std::string ext = path.extension().generic_string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif" || ext == ".tga" || ext == ".webp") {
        return AssetType::Texture;
    }
    if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
        return AssetType::Audio;
    }
    if (ext == ".cpp" || ext == ".cc" || ext == ".h" || ext == ".hpp") {
        return AssetType::Script;
    }
    if (ext == ".json" || ext == ".txt" || ext == ".md" || ext == ".ini") {
        return AssetType::Text;
    }

    return AssetType::Unknown;
}

ImVec4 GetTileAccentColor(bool isDirectory, AssetType type) {
    if (isDirectory) return ImVec4(0.88f, 0.79f, 0.39f, 1.0f);
    switch (type) {
    case AssetType::Texture: return ImVec4(0.34f, 0.73f, 0.95f, 1.0f);
    case AssetType::Audio: return ImVec4(0.57f, 0.80f, 0.35f, 1.0f);
    case AssetType::Text: return ImVec4(0.71f, 0.71f, 0.74f, 1.0f);
    case AssetType::Scene: return ImVec4(0.98f, 0.60f, 0.24f, 1.0f);
    case AssetType::Script: return ImVec4(0.97f, 0.49f, 0.26f, 1.0f);
    case AssetType::Unknown:
    default: return ImVec4(0.60f, 0.63f, 0.70f, 1.0f);
    }
}

void DrawFolderIcon(const ImVec2& min, const ImVec2& max, ImU32 color) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const float width = max.x - min.x;
    const float height = max.y - min.y;
    const ImU32 shadow = IM_COL32(0, 0, 0, 55);
    const ImU32 highlight = IM_COL32(255, 255, 255, 38);
    const ImVec2 tabMin(min.x + width * 0.14f, min.y + height * 0.20f);
    const ImVec2 tabMax(min.x + width * 0.47f, min.y + height * 0.40f);
    const ImVec2 bodyMin(min.x + width * 0.08f, min.y + height * 0.33f);
    const ImVec2 bodyMax(min.x + width * 0.92f, min.y + height * 0.78f);

    drawList->AddRectFilled(ImVec2(bodyMin.x + 2.0f, bodyMin.y + 3.0f), ImVec2(bodyMax.x + 2.0f, bodyMax.y + 3.0f), shadow, 6.0f);
    drawList->AddRectFilled(tabMin, tabMax, color, 4.0f);
    drawList->AddRectFilled(bodyMin, bodyMax, color, 6.0f);
    drawList->AddLine(ImVec2(bodyMin.x + 3.0f, bodyMin.y + 3.0f), ImVec2(bodyMax.x - 4.0f, bodyMin.y + 3.0f), highlight, 2.0f);
}

void DrawFileIcon(const ImVec2& min, const ImVec2& max, ImU32 color, const char* marker) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const float width = max.x - min.x;
    const float height = max.y - min.y;
    const ImU32 shadow = IM_COL32(0, 0, 0, 55);
    const ImVec2 bodyMin(min.x + width * 0.20f, min.y + height * 0.08f);
    const ImVec2 bodyMax(min.x + width * 0.80f, min.y + height * 0.88f);
    drawList->AddRectFilled(ImVec2(bodyMin.x + 2.0f, bodyMin.y + 3.0f), ImVec2(bodyMax.x + 2.0f, bodyMax.y + 3.0f), shadow, 4.0f);
    drawList->AddRectFilled(bodyMin, bodyMax, color, 4.0f);
    drawList->AddTriangleFilled(ImVec2(bodyMax.x - width * 0.22f, bodyMin.y), ImVec2(bodyMax.x, bodyMin.y), ImVec2(bodyMax.x, bodyMin.y + height * 0.22f), IM_COL32(255, 255, 255, 70));
    drawList->AddText(ImVec2(bodyMin.x + width * 0.08f, bodyMin.y + height * 0.34f), IM_COL32(30, 30, 35, 220), marker);
}

const char* GetFileMarker(AssetType type, const fs::path& path) {
    switch (type) {
    case AssetType::Texture: return "IMG";
    case AssetType::Audio: return "AUD";
    case AssetType::Scene: return "SCN";
    case AssetType::Script:
        if (path.extension() == ".h" || path.extension() == ".hpp") return ".H";
        return "C++";
    case AssetType::Text:
        if (path.extension() == ".json") return "JSN";
        if (path.extension() == ".md") return "MD";
        if (path.extension() == ".ini") return "INI";
        return "TXT";
    case AssetType::Unknown:
    default:
        return "FILE";
    }
}

void DrawEntryIcon(const ImRect& rect, const fs::path& path, bool isDirectory, const AssetRecord* asset) {
    const AssetType type = GuessAssetTypeFromPath(path, asset);
    const ImU32 color = ImGui::ColorConvertFloat4ToU32(GetTileAccentColor(isDirectory, type));
    if (isDirectory) {
        DrawFolderIcon(rect.Min, rect.Max, color);
        return;
    }

    DrawFileIcon(rect.Min, rect.Max, color, GetFileMarker(type, path));
}

std::vector<fs::directory_entry> GetSortedEntries(const std::string& directory, const std::string& search) {
    std::vector<fs::directory_entry> entries;
    std::error_code ec;
    const fs::path currentDir = ResourcePathUtils::Utf8ToPath(directory);
    for (const auto& entry : fs::directory_iterator(currentDir, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        const std::string name = entry.path().filename().generic_string();
        if (!ContainsInsensitive(name, search)) continue;
        entries.push_back(entry);
    }

    std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
        if (a.is_directory() != b.is_directory()) return a.is_directory() > b.is_directory();
        return a.path().filename().generic_string() < b.path().filename().generic_string();
    });
    return entries;
}

void DrawTreeFolderTarget(SceneState& sceneState, BrowserState& state, EditorState& editorState, const std::string& targetDirectory) {
    if (!ImGui::BeginDragDropTarget()) return;
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PROJECT_ENTRY_PATH")) {
        const char* source = static_cast<const char*>(payload->Data);
        if (source != nullptr) MoveProjectEntry(sceneState, state, editorState, source, targetDirectory);
    }
    ImGui::EndDragDropTarget();
}

void DrawFolderTreeNode(SceneState& sceneState, const fs::path& directory, BrowserState& state, EditorState& editorState) {
    std::vector<fs::path> children;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(directory, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        if (entry.is_directory()) children.push_back(entry.path());
    }
    std::sort(children.begin(), children.end());

    for (const fs::path& child : children) {
        const std::string childPath = NormalizePath(child);
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (state.currentDirectory == childPath) flags |= ImGuiTreeNodeFlags_Selected;
        const bool open = ImGui::TreeNodeEx(childPath.c_str(), flags, "%s", child.filename().generic_string().c_str());
        if (ImGui::IsItemClicked()) {
            state.currentDirectory = childPath;
            state.selectedPath = childPath;
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) state.currentDirectory = childPath;

        BeginProjectEntryDragSource(childPath, true);
        DrawTreeFolderTarget(sceneState, state, editorState, childPath);

        if (ImGui::BeginPopupContextItem(childPath.c_str())) {
            if (ImGui::BeginMenu("Create Item")) {
                for (int i = 0; i < 5; ++i) {
                    if (ImGui::MenuItem(ProjectItemTypeLabel(i))) CreateItemAndStartRename(state, editorState, i, childPath);
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Rename")) QueueRenameForPath(state, childPath);
            if (ImGui::MenuItem("Delete")) QueueDeleteForPath(state, childPath);
            ImGui::EndPopup();
        }

        if (open) {
            DrawFolderTreeNode(sceneState, child, state, editorState);
            ImGui::TreePop();
        }
    }
}

void DrawBreadcrumbs(BrowserState& state, const EditorState& editorState) {
    if (editorState.projectRootPath.empty()) return;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 6.0f));
    const fs::path root = ResourcePathUtils::Utf8ToPath(editorState.projectRootPath);
    const fs::path current = ResourcePathUtils::Utf8ToPath(state.currentDirectory);
    std::error_code ec;
    const fs::path relative = fs::relative(current, root, ec);

    if (ImGui::SmallButton(editorState.projectName.empty() ? "Project" : editorState.projectName.c_str())) {
        state.currentDirectory = editorState.projectRootPath;
        state.selectedPath = editorState.projectRootPath;
    }

    if (!ec && !relative.empty()) {
        fs::path running;
        for (const auto& segment : relative) {
            running /= segment;
            ImGui::SameLine();
            ImGui::TextUnformatted(">");
            ImGui::SameLine();
            const std::string path = NormalizePath(root / running);
            if (ImGui::SmallButton(segment.generic_string().c_str())) {
                state.currentDirectory = path;
                state.selectedPath = path;
            }
        }
    }
    ImGui::PopStyleVar();
}

void DrawProjectHeader(EditorState& editorState) {
    ImGui::TextUnformatted("Project");
    ImGui::SameLine();
    ImGui::TextDisabled("| Unity-style browser");
    ImGui::SameLine();
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - 280.0f));
    ImGui::TextDisabled("%s", editorState.projectStatus.c_str());
}

void DrawProjectEntryContextMenu(SceneState& sceneState, BrowserState& state, EditorState& editorState, const std::string& normalizedPath, bool isDirectory) {
    if (!ImGui::BeginPopupContextItem(normalizedPath.c_str())) return;
    if (isDirectory) {
        if (ImGui::BeginMenu("Create Item")) {
            for (int i = 0; i < 5; ++i) {
                if (ImGui::MenuItem(ProjectItemTypeLabel(i))) CreateItemAndStartRename(state, editorState, i, normalizedPath);
            }
            ImGui::EndMenu();
        }
    } else if (IsSceneFile(ResourcePathUtils::Utf8ToPath(normalizedPath)) && ImGui::MenuItem("Open Scene")) {
        OpenScene(sceneState, editorState, normalizedPath);
    }
    if (ImGui::MenuItem("Rename")) QueueRenameForPath(state, normalizedPath);
    if (ImGui::MenuItem("Delete")) QueueDeleteForPath(state, normalizedPath);
    ImGui::EndPopup();
}

void DrawEntryTile(SceneState& sceneState, const fs::directory_entry& entry, BrowserState& state, EditorState& editorState, int index) {
    const fs::path path = entry.path();
    const std::string normalizedPath = NormalizePath(path);
    const bool isDirectory = entry.is_directory();
    const bool selected = state.selectedPath == normalizedPath;
    const AssetRecord* asset = !isDirectory ? editorState.assetRegistry.findByPath(normalizedPath) : nullptr;

    ImGui::PushID(index);
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const ImVec2 tileSize(kTileWidth, kTileHeight);
    ImGui::InvisibleButton("##ProjectTile", tileSize);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    const bool pressed = ImGui::IsItemClicked();
    const ImRect tileRect(start, ImVec2(start.x + tileSize.x, start.y + tileSize.y));
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 bgColor = IM_COL32(0, 0, 0, 0);
    if (selected) bgColor = IM_COL32(72, 118, 191, 90);
    else if (hovered) bgColor = IM_COL32(255, 255, 255, 18);
    drawList->AddRectFilled(tileRect.Min, tileRect.Max, bgColor, 8.0f);
    if (active) drawList->AddRect(tileRect.Min, tileRect.Max, IM_COL32(120, 170, 255, 160), 8.0f, 0, 1.5f);

    const ImRect iconRect(ImVec2(tileRect.Min.x + 14.0f, tileRect.Min.y + 8.0f), ImVec2(tileRect.Max.x - 14.0f, tileRect.Min.y + 52.0f));
    DrawEntryIcon(iconRect, path, isDirectory, asset);

    const auto fitTextToWidth = [&](const std::string& text, float width, bool allowEllipsis) {
        if (text.empty()) {
            return std::string{};
        }

        std::string result;
        for (char ch : text) {
            std::string candidate = result;
            candidate.push_back(ch);
            if (ImGui::CalcTextSize(candidate.c_str()).x > width) {
                break;
            }
            result.push_back(ch);
        }

        if (result.empty()) {
            result.push_back(text.front());
        }

        if (allowEllipsis && result.size() < text.size()) {
            while (!result.empty() && ImGui::CalcTextSize((result + "...").c_str()).x > width) {
                result.pop_back();
            }
            if (!result.empty()) {
                result += "...";
            }
        }

        return result;
    };

    const std::string entryName = path.filename().generic_string();
    const ImVec2 textMin(tileRect.Min.x + 6.0f, tileRect.Min.y + 60.0f);
    const ImVec2 textMax(tileRect.Max.x - 6.0f, tileRect.Max.y - 6.0f);
    const float textWidth = textMax.x - textMin.x;
    const float fontSize = ImGui::GetFontSize();

    std::string firstLine = fitTextToWidth(entryName, textWidth, false);
    std::string secondSource = entryName.substr(firstLine.size());
    while (!secondSource.empty() && secondSource.front() == ' ') {
        secondSource.erase(secondSource.begin());
    }

    drawList->PushClipRect(textMin, textMax, true);
    drawList->AddText(ImVec2(textMin.x, textMin.y), IM_COL32(228, 232, 238, 255), firstLine.c_str());

    if (!secondSource.empty()) {
        const float secondLineY = textMin.y + fontSize + 2.0f;
        if (secondLineY + fontSize <= textMax.y) {
            const std::string secondLine = fitTextToWidth(secondSource, textWidth, true);
            drawList->AddText(ImVec2(textMin.x, secondLineY), IM_COL32(228, 232, 238, 255), secondLine.c_str());
        }
    }
    drawList->PopClipRect();

    if (pressed) {
        state.selectedPath = normalizedPath;
        if (isDirectory) {
            state.currentDirectory = normalizedPath;
        } else if (IsTextEditable(path) && LoadTextFile(normalizedPath, state.textBuffer)) {
            state.loadedTextPath = normalizedPath;
            state.textDirty = false;
        }
    }

    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (isDirectory) {
            state.currentDirectory = normalizedPath;
            state.selectedPath = normalizedPath;
        } else if (IsSceneFile(path)) {
            OpenScene(sceneState, editorState, normalizedPath);
        } else if (IsTextEditable(path) && LoadTextFile(normalizedPath, state.textBuffer)) {
            state.loadedTextPath = normalizedPath;
            state.textDirty = false;
        }
    }

    BeginProjectEntryDragSource(normalizedPath, isDirectory);
    BeginAssetPayloadDragSource(asset);

    if (isDirectory && ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PROJECT_ENTRY_PATH")) {
            const char* source = static_cast<const char*>(payload->Data);
            if (source != nullptr) MoveProjectEntry(sceneState, state, editorState, source, normalizedPath);
        }
        ImGui::EndDragDropTarget();
    }

    DrawProjectEntryContextMenu(sceneState, state, editorState, normalizedPath, isDirectory);
    ImGui::PopID();
}

void DrawProjectGrid(SceneState& sceneState, BrowserState& state, EditorState& editorState) {
    const std::vector<fs::directory_entry> entries = GetSortedEntries(state.currentDirectory, state.searchBuffer);
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const int columns = std::max(1, static_cast<int>((availableWidth + spacing) / (kTileWidth + spacing)));

    if (ImGui::BeginChild("ProjectGrid", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoScrollbar)) {
        if (state.currentDirectory != editorState.projectRootPath) {
            if (ImGui::Selectable("..", false, 0, ImVec2(kTileWidth, 0.0f))) {
                const fs::path parent = ResourcePathUtils::Utf8ToPath(state.currentDirectory).parent_path();
                state.currentDirectory = NormalizePath(ProjectManager::IsPathInsideProject(BuildProjectDescriptor(editorState), NormalizePath(parent)) ? parent : ResourcePathUtils::Utf8ToPath(editorState.projectRootPath));
                state.selectedPath = state.currentDirectory;
            }
        }

        int column = 0;
        for (std::size_t i = 0; i < entries.size(); ++i) {
            if (column > 0) ImGui::SameLine();
            DrawEntryTile(sceneState, entries[i], state, editorState, static_cast<int>(i));
            column = (column + 1) % columns;
        }

        if (ImGui::BeginPopupContextWindow("ProjectGridBlankContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::BeginMenu("Create Item")) {
                for (int i = 0; i < 5; ++i) {
                    if (ImGui::MenuItem(ProjectItemTypeLabel(i))) CreateItemAndStartRename(state, editorState, i, state.currentDirectory);
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PROJECT_ENTRY_PATH")) {
                const char* source = static_cast<const char*>(payload->Data);
                if (source != nullptr) MoveProjectEntry(sceneState, state, editorState, source, state.currentDirectory);
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImGui::EndChild();
}

void DrawSelectionDetails(SceneState& sceneState, BrowserState& state, EditorState& editorState) {
    ImGui::Separator();
    ImGui::Text("Selection");
    ImGui::SameLine();
    ImGui::TextDisabled("%s", state.selectedPath.empty() ? "Nothing selected" : RelativeDirectoryLabel(editorState, state.selectedPath).c_str());

    if (state.selectedPath.empty()) {
        ImGui::TextUnformatted("Select a file or folder to inspect it.");
        return;
    }

    const fs::path selected = ResourcePathUtils::Utf8ToPath(state.selectedPath);
    std::error_code ec;
    if (!fs::exists(selected, ec)) {
        ImGui::TextUnformatted("The selected entry no longer exists.");
        return;
    }

    ImGui::TextWrapped("Path: %s", RelativeToProject(editorState, state.selectedPath).c_str());
    if (fs::is_directory(selected, ec)) {
        ImGui::TextUnformatted("Folder");
        if (ImGui::Button("Open Folder")) state.currentDirectory = state.selectedPath;
        return;
    }

    const AssetRecord* asset = editorState.assetRegistry.findByPath(state.selectedPath);
    if (asset != nullptr) {
        ImGui::Text("Asset Type: %s", asset->typeName.c_str());
        int usageCount = 0;
        for (const GameObject& object : sceneState.objects) {
            if (object.textureResourceId == asset->id || object.scriptResourceId == asset->id || object.texturePath == asset->sourcePath || object.texturePath == asset->relativePath || object.scriptPath == asset->sourcePath || object.scriptPath == asset->relativePath) {
                ++usageCount;
            }
        }
        ImGui::Text("Used By Objects: %d", usageCount);

        if (asset->type == AssetType::Texture) {
            if (ImGui::Button("Bind Texture To Selected Object")) AssignAssetToSelection(sceneState, editorState, *asset);
            ImGui::SameLine();
            if (ImGui::Button("Create Sprite In Scene")) {
                const float x = 80.0f + 24.0f * static_cast<float>(sceneState.objects.size() % 8);
                const float y = 80.0f + 24.0f * static_cast<float>(sceneState.objects.size() % 6);
                CreateObjectFromAsset(sceneState, editorState, *asset, x, y, "Project panel");
            }
        }

        if (asset->type == AssetType::Script && ImGui::Button("Bind Script To Selected Object")) {
            const int index = editorState.selectedObjectIndex;
            if (index >= 0 && index < static_cast<int>(sceneState.objects.size())) {
                sceneState.objects[index].scriptResourceId = asset->id;
                sceneState.objects[index].scriptPath = asset->sourcePath;
                editorState.assetStatus = "Bound script: " + asset->name;
                AddEditorLog(editorState, EditorLogLevel::Info, editorState.assetStatus);
            } else {
                editorState.assetStatus = "Select an object before binding a script";
                AddEditorLog(editorState, EditorLogLevel::Warning, editorState.assetStatus);
            }
        }
    }

    if (IsSceneFile(selected) && ImGui::Button("Open Scene File")) OpenScene(sceneState, editorState, state.selectedPath);

    if (IsTextEditable(selected)) {
        if (state.loadedTextPath != state.selectedPath && LoadTextFile(state.selectedPath, state.textBuffer)) {
            state.loadedTextPath = state.selectedPath;
            state.textDirty = false;
        }
        if (ImGui::InputTextMultiline("##ProjectFileEditor", state.textBuffer.data(), state.textBuffer.size(), ImVec2(-1.0f, 120.0f))) state.textDirty = true;
        if (state.textDirty) ImGui::TextDisabled("Unsaved changes");
        if (ImGui::Button("Save File")) {
            if (SaveTextFile(state.selectedPath, state.textBuffer)) {
                state.textDirty = false;
                editorState.assetStatus = "Saved file: " + state.selectedPath;
                RefreshProjectAssets(editorState);
            } else {
                editorState.assetStatus = "Failed to save file";
            }
        }
    }
}

void DrawProjectModals(BrowserState& state, EditorState& editorState) {
    if (state.openCreateNamePopup) {
        ImGui::OpenPopup("CreateProjectEntryNamePopup");
        state.openCreateNamePopup = false;
    }
    if (state.openRenamePopup) {
        ImGui::OpenPopup("RenameProjectEntryPopup");
        state.openRenamePopup = false;
    }
    if (state.openDeletePopup) {
        ImGui::OpenPopup("DeleteProjectEntryPopup");
        state.openDeletePopup = false;
    }

    if (ImGui::BeginPopupModal("DeleteProjectEntryPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Delete this entry from the project?");
        ImGui::TextWrapped("%s", state.pendingDeletePath.c_str());
        if (ImGui::Button("Delete", ImVec2(120.0f, 0.0f))) {
            DeletePendingEntry(state, editorState);
            state.pendingDeletePath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##Delete", ImVec2(120.0f, 0.0f))) {
            state.pendingDeletePath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("CreateProjectEntryNamePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Name the new entry:");
        ImGui::TextWrapped("%s", state.pendingCreatePath.c_str());
        ImGui::InputText("##CreateProjectEntryName", state.createItemName, sizeof(state.createItemName));
        if (ImGui::Button("Create", ImVec2(120.0f, 0.0f))) {
            const std::string previousPath = state.pendingCreatePath;
            const std::string requestedName = state.createItemName;
            const std::string currentBaseName = GetEditableBaseName(previousPath);
            if (requestedName == currentBaseName) {
                editorState.assetStatus = "Created: " + previousPath;
                state.selectedPath = previousPath;
                ClearCreateNamePopupState(state);
                ImGui::CloseCurrentPopup();
            } else {
                std::string renamedPath;
                std::string error;
                if (ProjectManager::RenameProjectEntry(BuildProjectDescriptor(editorState), previousPath, requestedName, renamedPath, error)) {
                    RefreshProjectAssets(editorState);
                    state.selectedPath = renamedPath;
                    state.loadedTextPath = ReplacePathPrefix(state.loadedTextPath, previousPath, renamedPath);
                    editorState.assetStatus = "Created: " + renamedPath;
                    ClearCreateNamePopupState(state);
                    ImGui::CloseCurrentPopup();
                } else {
                    editorState.assetStatus = error.empty() ? "Failed to name new entry" : error;
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Keep Default Name", ImVec2(160.0f, 0.0f))) {
            ClearCreateNamePopupState(state);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("RenameProjectEntryPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Rename entry:");
        ImGui::TextWrapped("%s", state.pendingRenamePath.c_str());
        ImGui::InputText("##RenameProjectEntryName", state.renameItemName, sizeof(state.renameItemName));
        if (ImGui::Button("Rename", ImVec2(120.0f, 0.0f))) {
            std::string renamedPath;
            std::string error;
            const std::string previousPath = state.pendingRenamePath;
            if (ProjectManager::RenameProjectEntry(BuildProjectDescriptor(editorState), previousPath, state.renameItemName, renamedPath, error)) {
                RefreshProjectAssets(editorState);
                UpdateBrowserPathsAfterMove(state, previousPath, renamedPath);
                editorState.sceneFilePath = ReplacePathPrefix(editorState.sceneFilePath, previousPath, renamedPath);
                editorState.focusAssetPath = ReplacePathPrefix(editorState.focusAssetPath, previousPath, renamedPath);
                editorState.assetStatus = "Renamed: " + renamedPath;
                ClearRenamePopupState(state);
                ImGui::CloseCurrentPopup();
            } else {
                editorState.assetStatus = error.empty() ? "Failed to rename entry" : error;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##Rename", ImVec2(120.0f, 0.0f))) {
            ClearRenamePopupState(state);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

}  // namespace

void DrawAssetPanel(SceneState& sceneState, EditorState& editorState)
{
    ImGui::Begin("Project", &editorState.showProject);

    static char projectNameBuffer[128] = "MyProject";
    static BrowserState browserState;
    EnsureBrowserState(browserState, editorState);
    ApplyFocusedAssetSelection(browserState, editorState);
    const bool hasProject = !editorState.projectRootPath.empty();

    DrawProjectHeader(editorState);
    ImGui::Separator();

    ImGui::TextUnformatted("Project Name");
    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputText("##ProjectNameInput", projectNameBuffer, sizeof(projectNameBuffer));
    ImGui::SameLine();
    if (ImGui::Button("New Project")) {
        std::string folderPath;
        if (PickFolderFromNativeDialog(folderPath)) {
            editorState.pendingProjectName = projectNameBuffer;
            editorState.pendingProjectDirectory = folderPath;
            editorState.pendingProjectCommand = ProjectCommand::Create;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Open Project")) {
        std::string folderPath;
        if (PickFolderFromNativeDialog(folderPath)) {
            editorState.pendingProjectDirectory = folderPath;
            editorState.pendingProjectCommand = ProjectCommand::Open;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Sync Files")) editorState.pendingProjectCommand = ProjectCommand::Sync;

    ImGui::TextWrapped("Project Status: %s", editorState.projectStatus.c_str());
    if (!hasProject) {
        ImGui::TextWrapped("Create or open a project first. Once loaded, this panel switches to a Unity-style folder tree and icon grid.");
        ImGui::End();
        return;
    }

    if (ImGui::Button("Import Files")) {
        const std::vector<std::string> filePaths = PickFilesFromNativeDialog();
        if (!filePaths.empty()) {
            const std::size_t importedCount = editorState.assetRegistry.importFilesToProject(filePaths);
            editorState.assetRegistry.saveManifest(editorState.assetManifestPath);
            editorState.assetStatus = importedCount > 0 ? "Imported " + std::to_string(importedCount) + " file(s)" : editorState.assetRegistry.getLastError();
            editorState.pendingProjectCommand = ProjectCommand::Sync;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Import Folder")) {
        std::string folderPath;
        if (PickFolderFromNativeDialog(folderPath)) {
            const std::size_t importedCount = editorState.assetRegistry.importFolderToProject(folderPath);
            editorState.assetRegistry.saveManifest(editorState.assetManifestPath);
            editorState.assetStatus = importedCount > 0 ? "Imported " + std::to_string(importedCount) + " asset(s)" : editorState.assetRegistry.getLastError();
            editorState.pendingProjectCommand = ProjectCommand::Sync;
        }
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(210.0f);
    ImGui::InputTextWithHint("##ProjectSearch", "Search current folder...", browserState.searchBuffer, sizeof(browserState.searchBuffer));

    ImGui::TextWrapped("Project Root: %s", editorState.projectRootPath.c_str());
    ImGui::TextWrapped("%s", editorState.assetStatus.c_str());
    ImGui::Separator();

    if (browserState.currentDirectory.empty()) browserState.currentDirectory = editorState.projectRootPath;
    DrawProjectModals(browserState, editorState);

    const float availableHeight = ImGui::GetContentRegionAvail().y;
    const float topHeight = std::max(220.0f, availableHeight - kBottomPanelHeight);

    if (ImGui::BeginTable("UnityStyleProjectLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV, ImVec2(0.0f, topHeight))) {
        ImGui::TableSetupColumn("ProjectTree", ImGuiTableColumnFlags_WidthFixed, kTreeWidth);
        ImGui::TableSetupColumn("ProjectContent", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();
        if (ImGui::BeginChild("ProjectTreePane", ImVec2(0.0f, 0.0f), true)) {
            ImGui::TextUnformatted("Folders");
            ImGui::Separator();

            ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (browserState.currentDirectory == editorState.projectRootPath) rootFlags |= ImGuiTreeNodeFlags_Selected;
            const bool rootOpen = ImGui::TreeNodeEx(editorState.projectRootPath.c_str(), rootFlags, "%s", editorState.projectName.c_str());
            if (ImGui::IsItemClicked()) {
                browserState.currentDirectory = editorState.projectRootPath;
                browserState.selectedPath = editorState.projectRootPath;
            }
            BeginProjectEntryDragSource(editorState.projectRootPath, true);
            DrawTreeFolderTarget(sceneState, browserState, editorState, editorState.projectRootPath);
            if (ImGui::BeginPopupContextItem("ProjectRootContext")) {
                if (ImGui::BeginMenu("Create Item")) {
                    for (int i = 0; i < 5; ++i) {
                        if (ImGui::MenuItem(ProjectItemTypeLabel(i))) CreateItemAndStartRename(browserState, editorState, i, editorState.projectRootPath);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }
            if (rootOpen) {
                DrawFolderTreeNode(sceneState, ResourcePathUtils::Utf8ToPath(editorState.projectRootPath), browserState, editorState);
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();

        ImGui::TableNextColumn();
        if (ImGui::BeginChild("ProjectContentPane", ImVec2(0.0f, 0.0f), false)) {
            DrawBreadcrumbs(browserState, editorState);
            ImGui::Separator();
            DrawProjectGrid(sceneState, browserState, editorState);
        }
        ImGui::EndChild();
        ImGui::EndTable();
    }

    if (ImGui::BeginChild("ProjectDetailsPane", ImVec2(0.0f, 0.0f), false)) {
        DrawSelectionDetails(sceneState, browserState, editorState);
    }
    ImGui::EndChild();
    ImGui::End();
}
