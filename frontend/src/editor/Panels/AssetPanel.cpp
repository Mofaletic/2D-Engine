#include "AssetPanel.h"
#include "../EditorActions.h"
#include "imgui.h"
#include "../../../../backend/SceneSerializer.h"
#include "../../../../backend/project/ProjectManager.h"
#include "../../../../backend/resource/ResourcePathUtils.h"

#ifdef _WIN32
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

struct BrowserState {
    std::string projectRoot;
    std::string currentDirectory;
    std::string selectedPath;
    std::string loadedTextPath;
    std::vector<char> textBuffer = std::vector<char>(kEditorBufferSize, '\0');
    bool textDirty = false;
    std::string pendingCreateDirectory;
    std::string pendingCreatePath;
    std::string pendingDeletePath;
    std::string pendingRenamePath;
    bool openCreateNamePopup = false;
    bool openDeletePopup = false;
    bool openRenamePopup = false;
    char createItemName[128] = "NewItem";
    int createItemTypeIndex = 1;
    char renameItemName[128] = "";
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

std::string RelativeToProject(const EditorState& editorState, const fs::path& path) {
    std::error_code ec;
    const fs::path relative = fs::relative(path, ResourcePathUtils::Utf8ToPath(editorState.projectRootPath), ec);
    return ec ? path.filename().generic_string() : relative.generic_string();
}

std::string RelativeDirectoryLabel(const EditorState& editorState, const std::string& path) {
    if (path.empty() || editorState.projectRootPath.empty()) {
        return {};
    }

    const std::string normalizedProjectRoot = NormalizePath(ResourcePathUtils::Utf8ToPath(editorState.projectRootPath));
    const std::string normalizedPath = NormalizePath(ResourcePathUtils::Utf8ToPath(path));
    if (normalizedPath == normalizedProjectRoot) {
        return "Project Root";
    }

    const std::string relative = RelativeToProject(editorState, ResourcePathUtils::Utf8ToPath(path));
    return relative.empty() ? "Project Root" : relative;
}

void ResetBrowserState(BrowserState& state, const EditorState& editorState) {
    state.projectRoot = editorState.projectRootPath;
    state.currentDirectory = editorState.projectRootPath;
    state.selectedPath.clear();
    state.loadedTextPath.clear();
    std::fill(state.textBuffer.begin(), state.textBuffer.end(), '\0');
    state.textDirty = false;
    state.pendingCreateDirectory.clear();
    state.pendingCreatePath.clear();
    state.pendingDeletePath.clear();
    state.pendingRenamePath.clear();
    state.openCreateNamePopup = false;
    state.openDeletePopup = false;
    state.openRenamePopup = false;
    std::strcpy(state.createItemName, "NewItem");
    state.createItemTypeIndex = 1;
    state.renameItemName[0] = '\0';
}

void EnsureBrowserState(BrowserState& state, const EditorState& editorState) {
    if (state.projectRoot != editorState.projectRootPath) ResetBrowserState(state, editorState);
    if (!state.currentDirectory.empty() && !fs::exists(ResourcePathUtils::Utf8ToPath(state.currentDirectory))) {
        state.currentDirectory = editorState.projectRootPath;
    }
    if (!state.selectedPath.empty() && !fs::exists(ResourcePathUtils::Utf8ToPath(state.selectedPath))) {
        state.selectedPath.clear();
    }
}

void ApplyFocusedAssetSelection(BrowserState& state, EditorState& editorState) {
    if (editorState.focusAssetPath.empty()) {
        return;
    }

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
    state.currentDirectory = fs::is_directory(resolvedFsPath, ec)
        ? resolvedPath
        : NormalizePath(resolvedFsPath.parent_path());
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

bool IsSamePathOrChild(const std::string& candidatePath, const std::string& parentPath) {
    if (candidatePath.empty() || parentPath.empty()) {
        return false;
    }

    if (candidatePath == parentPath) {
        return true;
    }

    const char separator = '/';
    return candidatePath.size() > parentPath.size() &&
        candidatePath.compare(0, parentPath.size(), parentPath) == 0 &&
        candidatePath[parentPath.size()] == separator;
}

void ClearSelectionIfDeleted(BrowserState& state, const std::string& deletedPath, const std::string& projectRootPath) {
    if (IsSamePathOrChild(state.selectedPath, deletedPath)) {
        state.selectedPath.clear();
    }

    if (IsSamePathOrChild(state.loadedTextPath, deletedPath)) {
        state.loadedTextPath.clear();
        state.textDirty = false;
        std::fill(state.textBuffer.begin(), state.textBuffer.end(), '\0');
    }

    if (IsSamePathOrChild(state.currentDirectory, deletedPath)) {
        state.currentDirectory = projectRootPath;
    }
}

void DeletePendingEntry(BrowserState& state, EditorState& editorState) {
    std::string error;
    const std::string deletedPath = state.pendingDeletePath;
    if (ProjectManager::DeleteProjectEntry(BuildProjectDescriptor(editorState), deletedPath, error)) {
        editorState.assetRegistry.synchronizeProjectAssets();
        if (!editorState.assetManifestPath.empty()) {
            editorState.assetRegistry.saveManifest(editorState.assetManifestPath);
        }
        editorState.assetStatus = "Deleted: " + deletedPath;
        editorState.pendingProjectCommand = ProjectCommand::Sync;
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
    std::string baseName;
    const std::string filename = filePath.filename().string();
    if (IsSceneFile(filePath)) {
        baseName = filename.substr(0, filename.size() - 11);
    } else {
        baseName = filePath.stem().string();
    }
    std::strncpy(state.createItemName, baseName.c_str(), sizeof(state.createItemName));
    state.createItemName[sizeof(state.createItemName) - 1] = '\0';
    state.openCreateNamePopup = true;
}

void ClearCreateNamePopupState(BrowserState& state) {
    state.pendingCreatePath.clear();
    state.openCreateNamePopup = false;
    std::strcpy(state.createItemName, "NewItem");
}

void ClearRenamePopupState(BrowserState& state) {
    state.pendingRenamePath.clear();
    state.renameItemName[0] = '\0';
    state.openRenamePopup = false;
}

void BeginAssetDragSource(const AssetRecord* asset) {
    if (asset == nullptr) {
        return;
    }

    const char* payloadType = nullptr;
    if (asset->type == AssetType::Texture) {
        payloadType = "ASSET_TEXTURE_ID";
    } else if (asset->type == AssetType::Script) {
        payloadType = "ASSET_SCRIPT_ID";
    }

    if (payloadType == nullptr) {
        return;
    }

    if (ImGui::BeginDragDropSource()) {
        const std::uint64_t assetId = asset->id;
        ImGui::SetDragDropPayload(payloadType, &assetId, sizeof(assetId));
        ImGui::TextUnformatted(asset->name.c_str());
        ImGui::TextDisabled("%s", asset->typeName.c_str());
        if (!asset->relativePath.empty()) {
            ImGui::TextDisabled("%s", asset->relativePath.c_str());
        }
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

void QueueRenameForPath(BrowserState& state, const std::string& path);
void CreateItemAndStartRename(BrowserState& state, EditorState& editorState, int typeIndex, const std::string& directory);

void DrawFolderTree(const fs::path& directory, BrowserState& state, EditorState& editorState) {
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
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (state.currentDirectory == childPath) flags |= ImGuiTreeNodeFlags_Selected;
        const bool open = ImGui::TreeNodeEx(childPath.c_str(), flags, "%s", child.filename().generic_string().c_str());
        if (ImGui::IsItemClicked()) {
            state.currentDirectory = childPath;
            state.selectedPath = childPath;
        }
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::BeginMenu("Create Item")) {
                if (ImGui::MenuItem("Audio")) {
                    CreateItemAndStartRename(state, editorState, 0, childPath);
                }
                if (ImGui::MenuItem("Image")) {
                    CreateItemAndStartRename(state, editorState, 1, childPath);
                }
                if (ImGui::MenuItem("Text")) {
                    CreateItemAndStartRename(state, editorState, 2, childPath);
                }
                if (ImGui::MenuItem("Scene")) {
                    CreateItemAndStartRename(state, editorState, 3, childPath);
                }
                if (ImGui::MenuItem("C++ Script")) {
                    CreateItemAndStartRename(state, editorState, 4, childPath);
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Rename")) {
                QueueRenameForPath(state, childPath);
            }
            if (ImGui::MenuItem("Delete")) {
                QueueDeleteForPath(state, childPath);
            }
            ImGui::EndPopup();
        }
        if (open) {
            DrawFolderTree(child, state, editorState);
            ImGui::TreePop();
        }
    }
}

void QueueRenameForPath(BrowserState& state, const std::string& path) {
    state.pendingRenamePath = path;
    const fs::path filePath = ResourcePathUtils::Utf8ToPath(path);
    std::string baseName;
    const std::string filename = filePath.filename().string();
    if (IsSceneFile(filePath)) {
        baseName = filename.substr(0, filename.size() - 11);
    } else {
        baseName = filePath.stem().string();
    }
    std::strncpy(state.renameItemName, baseName.c_str(), sizeof(state.renameItemName));
    state.renameItemName[sizeof(state.renameItemName) - 1] = '\0';
    state.openRenamePopup = true;
}

std::string GetEditableBaseName(const std::string& path) {
    const fs::path filePath = ResourcePathUtils::Utf8ToPath(path);
    const std::string filename = filePath.filename().string();
    if (IsSceneFile(filePath)) {
        return filename.substr(0, filename.size() - 11);
    }
    return filePath.stem().string();
}

void CreateItemAndStartRename(BrowserState& state, EditorState& editorState, int typeIndex, const std::string& directory) {
    std::string createdPath;
    std::string error;
    if (ProjectManager::CreateProjectItemInDirectory(
        BuildProjectDescriptor(editorState),
        directory,
        ToProjectItemType(typeIndex),
        "NewItem",
        createdPath,
        error)) {
        editorState.assetStatus = "Created: " + createdPath;
        editorState.pendingProjectCommand = ProjectCommand::Sync;
        state.selectedPath = createdPath;
        state.currentDirectory = directory;
        QueueCreateNameForPath(state, createdPath);
    } else {
        editorState.assetStatus = error.empty() ? "Failed to create item" : error;
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

    ImGui::TextUnformatted("Project Browser");
    ImGui::TextWrapped("Open a project, browse folders, right click to create or delete files, and double click scene files to switch scenes.");
    ImGui::Separator();
    ImGui::InputText("Project Name", projectNameBuffer, sizeof(projectNameBuffer));

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
    if (ImGui::Button("Sync Files")) {
        editorState.pendingProjectCommand = ProjectCommand::Sync;
    }

    ImGui::TextWrapped("Project Status: %s", editorState.projectStatus.c_str());
    if (!hasProject) {
        ImGui::TextWrapped("Load a project first to browse its directory.");
        ImGui::End();
        return;
    }

    if (ImGui::Button("Import Files")) {
        const std::vector<std::string> filePaths = PickFilesFromNativeDialog();
        if (!filePaths.empty()) {
            const std::size_t importedCount = editorState.assetRegistry.importFilesToProject(filePaths);
            editorState.assetRegistry.saveManifest(editorState.assetManifestPath);
            editorState.assetStatus = importedCount > 0
                ? "Imported " + std::to_string(importedCount) + " file(s)"
                : editorState.assetRegistry.getLastError();
            editorState.pendingProjectCommand = ProjectCommand::Sync;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Import Folder")) {
        std::string folderPath;
        if (PickFolderFromNativeDialog(folderPath)) {
            const std::size_t importedCount = editorState.assetRegistry.importFolderToProject(folderPath);
            editorState.assetRegistry.saveManifest(editorState.assetManifestPath);
            editorState.assetStatus = importedCount > 0
                ? "Imported " + std::to_string(importedCount) + " asset(s)"
                : editorState.assetRegistry.getLastError();
            editorState.pendingProjectCommand = ProjectCommand::Sync;
        }
    }

    ImGui::TextWrapped("Project Root: %s", editorState.projectRootPath.c_str());
    ImGui::TextWrapped("%s", editorState.assetStatus.c_str());
    ImGui::Separator();
    ImGui::Text("Current Folder: %s", RelativeDirectoryLabel(editorState, browserState.currentDirectory).c_str());
    if (browserState.currentDirectory != editorState.projectRootPath) {
        ImGui::SameLine();
        if (ImGui::Button("Back To Project Root")) {
            browserState.currentDirectory = editorState.projectRootPath;
            browserState.selectedPath = editorState.projectRootPath;
        }
    }

    if (browserState.openCreateNamePopup) {
        ImGui::OpenPopup("CreateProjectEntryNamePopup");
        browserState.openCreateNamePopup = false;
    }
    if (browserState.openRenamePopup) {
        ImGui::OpenPopup("RenameProjectEntryPopup");
        browserState.openRenamePopup = false;
    }
    if (browserState.openDeletePopup) {
        ImGui::OpenPopup("DeleteProjectEntryPopup");
        browserState.openDeletePopup = false;
    }

    if (ImGui::BeginPopupModal("DeleteProjectEntryPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Delete this entry from the project?");
        ImGui::TextWrapped("%s", browserState.pendingDeletePath.c_str());
        if (ImGui::Button("Delete", ImVec2(120.0f, 0.0f))) {
            DeletePendingEntry(browserState, editorState);
            if (browserState.pendingDeletePath.empty() || editorState.assetStatus.rfind("Deleted: ", 0) == 0) {
                browserState.pendingDeletePath.clear();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##Delete", ImVec2(120.0f, 0.0f))) {
            browserState.pendingDeletePath.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("CreateProjectEntryNamePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Name the new entry:");
        ImGui::TextWrapped("%s", browserState.pendingCreatePath.c_str());
        ImGui::InputText("File Name", browserState.createItemName, sizeof(browserState.createItemName));
        if (ImGui::Button("Create", ImVec2(120.0f, 0.0f))) {
            const std::string previousCreatePath = browserState.pendingCreatePath;
            const std::string requestedName = browserState.createItemName;
            const std::string currentBaseName = GetEditableBaseName(previousCreatePath);
            if (requestedName == currentBaseName) {
                editorState.assetStatus = "Created: " + previousCreatePath;
                editorState.pendingProjectCommand = ProjectCommand::Sync;
                browserState.selectedPath = previousCreatePath;
                ClearCreateNamePopupState(browserState);
                ImGui::CloseCurrentPopup();
            } else {
                std::string renamedPath;
                std::string error;
                if (ProjectManager::RenameProjectEntry(
                    BuildProjectDescriptor(editorState),
                    previousCreatePath,
                    requestedName,
                    renamedPath,
                    error)) {
                    editorState.assetStatus = "Created: " + renamedPath;
                    editorState.pendingProjectCommand = ProjectCommand::Sync;
                    browserState.selectedPath = renamedPath;
                    if (browserState.loadedTextPath == previousCreatePath) {
                        browserState.loadedTextPath = renamedPath;
                    }
                    ClearCreateNamePopupState(browserState);
                    ImGui::CloseCurrentPopup();
                } else {
                    editorState.assetStatus = error.empty() ? "Failed to name new entry" : error;
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Keep Default Name", ImVec2(160.0f, 0.0f))) {
            ClearCreateNamePopupState(browserState);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("RenameProjectEntryPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Rename entry:");
        ImGui::TextWrapped("%s", browserState.pendingRenamePath.c_str());
        ImGui::InputText("New Name", browserState.renameItemName, sizeof(browserState.renameItemName));
        if (ImGui::Button("Rename", ImVec2(120.0f, 0.0f))) {
            std::string renamedPath;
            std::string error;
            const std::string previousRenamePath = browserState.pendingRenamePath;
            if (ProjectManager::RenameProjectEntry(
                BuildProjectDescriptor(editorState),
                previousRenamePath,
                browserState.renameItemName,
                renamedPath,
                error)) {
                editorState.assetStatus = "Renamed: " + renamedPath;
                editorState.pendingProjectCommand = ProjectCommand::Sync;
                browserState.selectedPath = renamedPath;
                if (browserState.loadedTextPath == previousRenamePath) {
                    browserState.loadedTextPath = renamedPath;
                }
                ClearRenamePopupState(browserState);
                ImGui::CloseCurrentPopup();
            } else {
                editorState.assetStatus = error.empty() ? "Failed to rename entry" : error;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##Rename", ImVec2(120.0f, 0.0f))) {
            ClearRenamePopupState(browserState);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginTable("ProjectBrowser", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
        ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthStretch, 0.34f);
        ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthStretch, 0.66f);

        ImGui::TableNextColumn();
        ImGui::BeginChild("FolderTree", ImVec2(0.0f, 280.0f), true);
        {
            ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
            if (browserState.currentDirectory == editorState.projectRootPath) rootFlags |= ImGuiTreeNodeFlags_Selected;
            const bool open = ImGui::TreeNodeEx(editorState.projectRootPath.c_str(), rootFlags, "%s", editorState.projectName.c_str());
            if (ImGui::IsItemClicked()) {
                browserState.currentDirectory = editorState.projectRootPath;
                browserState.selectedPath = editorState.projectRootPath;
            }
            if (ImGui::BeginPopupContextItem("RootContext")) {
                if (ImGui::BeginMenu("Create Item")) {
                    if (ImGui::MenuItem("Audio")) {
                        CreateItemAndStartRename(browserState, editorState, 0, editorState.projectRootPath);
                    }
                    if (ImGui::MenuItem("Image")) {
                        CreateItemAndStartRename(browserState, editorState, 1, editorState.projectRootPath);
                    }
                    if (ImGui::MenuItem("Text")) {
                        CreateItemAndStartRename(browserState, editorState, 2, editorState.projectRootPath);
                    }
                    if (ImGui::MenuItem("Scene")) {
                        CreateItemAndStartRename(browserState, editorState, 3, editorState.projectRootPath);
                    }
                    if (ImGui::MenuItem("C++ Script")) {
                        CreateItemAndStartRename(browserState, editorState, 4, editorState.projectRootPath);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }
            if (open) {
                DrawFolderTree(ResourcePathUtils::Utf8ToPath(editorState.projectRootPath), browserState, editorState);
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();

        ImGui::TableNextColumn();
        ImGui::BeginChild("DirectoryContents", ImVec2(0.0f, 280.0f), true);
        {
            std::vector<fs::directory_entry> entries;
            std::error_code ec;
            const fs::path currentDir = ResourcePathUtils::Utf8ToPath(browserState.currentDirectory);
            const fs::path projectRoot = ResourcePathUtils::Utf8ToPath(editorState.projectRootPath);
            if (NormalizePath(currentDir) != NormalizePath(projectRoot)) {
                if (ImGui::Selectable("[Up] ..", false)) {
                    const fs::path parent = currentDir.parent_path();
                    browserState.currentDirectory =
                        NormalizePath(IsSamePathOrChild(NormalizePath(parent), NormalizePath(projectRoot)) ? parent : projectRoot);
                    browserState.selectedPath = browserState.currentDirectory;
                }
                ImGui::Separator();
            }
            for (const auto& entry : fs::directory_iterator(currentDir, fs::directory_options::skip_permission_denied, ec)) {
                if (ec) {
                    ec.clear();
                    continue;
                }
                entries.push_back(entry);
            }
            std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
                if (a.is_directory() != b.is_directory()) return a.is_directory() > b.is_directory();
                return a.path().filename().generic_string() < b.path().filename().generic_string();
            });

            for (const auto& entry : entries) {
                const fs::path path = entry.path();
                const std::string normalized = NormalizePath(path);
                const bool isDir = entry.is_directory();
                const bool selected = browserState.selectedPath == normalized;
                const AssetRecord* asset = !isDir ? editorState.assetRegistry.findByPath(normalized) : nullptr;
                std::string label = isDir ? "[Dir] " : "[File] ";
                label += path.filename().generic_string();
                if (ImGui::Selectable((label + "##" + normalized).c_str(), selected)) {
                    browserState.selectedPath = normalized;
                    if (isDir) {
                        browserState.currentDirectory = normalized;
                    } else if (IsTextEditable(path)) {
                        if (LoadTextFile(normalized, browserState.textBuffer)) {
                            browserState.loadedTextPath = normalized;
                            browserState.textDirty = false;
                        }
                    }
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (isDir) {
                        browserState.currentDirectory = normalized;
                    } else if (IsSceneFile(path)) {
                        OpenScene(sceneState, editorState, normalized);
                    } else if (IsTextEditable(path) && LoadTextFile(normalized, browserState.textBuffer)) {
                        browserState.loadedTextPath = normalized;
                        browserState.textDirty = false;
                    }
                }
                if (ImGui::BeginPopupContextItem(normalized.c_str())) {
                    if (isDir) {
                        if (ImGui::BeginMenu("Create Item")) {
                            if (ImGui::MenuItem("Audio")) {
                                CreateItemAndStartRename(browserState, editorState, 0, normalized);
                            }
                            if (ImGui::MenuItem("Image")) {
                                CreateItemAndStartRename(browserState, editorState, 1, normalized);
                            }
                            if (ImGui::MenuItem("Text")) {
                                CreateItemAndStartRename(browserState, editorState, 2, normalized);
                            }
                            if (ImGui::MenuItem("Scene")) {
                                CreateItemAndStartRename(browserState, editorState, 3, normalized);
                            }
                            if (ImGui::MenuItem("C++ Script")) {
                                CreateItemAndStartRename(browserState, editorState, 4, normalized);
                            }
                            ImGui::EndMenu();
                        }
                        if (ImGui::MenuItem("Rename")) {
                            QueueRenameForPath(browserState, normalized);
                        }
                        if (ImGui::MenuItem("Delete")) {
                            QueueDeleteForPath(browserState, normalized);
                        }
                    } else {
                        if (IsSceneFile(path) && ImGui::MenuItem("Open Scene")) {
                            OpenScene(sceneState, editorState, normalized);
                        }
                        if (ImGui::MenuItem("Rename")) {
                            QueueRenameForPath(browserState, normalized);
                        }
                        if (ImGui::MenuItem("Delete")) {
                            QueueDeleteForPath(browserState, normalized);
                        }
                    }
                    ImGui::EndPopup();
                }
                BeginAssetDragSource(asset);
                ImGui::TextDisabled("%s", RelativeToProject(editorState, path).c_str());
            }
            if (ImGui::BeginPopupContextWindow("DirectoryBlankContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
                if (ImGui::BeginMenu("Create Item")) {
                    if (ImGui::MenuItem("Audio")) {
                        CreateItemAndStartRename(browserState, editorState, 0, browserState.currentDirectory);
                    }
                    if (ImGui::MenuItem("Image")) {
                        CreateItemAndStartRename(browserState, editorState, 1, browserState.currentDirectory);
                    }
                    if (ImGui::MenuItem("Text")) {
                        CreateItemAndStartRename(browserState, editorState, 2, browserState.currentDirectory);
                    }
                    if (ImGui::MenuItem("Scene")) {
                        CreateItemAndStartRename(browserState, editorState, 3, browserState.currentDirectory);
                    }
                    if (ImGui::MenuItem("C++ Script")) {
                        CreateItemAndStartRename(browserState, editorState, 4, browserState.currentDirectory);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Selection");
    if (!browserState.selectedPath.empty() && fs::exists(ResourcePathUtils::Utf8ToPath(browserState.selectedPath))) {
        const fs::path selected = ResourcePathUtils::Utf8ToPath(browserState.selectedPath);
        ImGui::TextWrapped("Path: %s", browserState.selectedPath.c_str());
        if (fs::is_regular_file(selected)) {
            const AssetRecord* asset = editorState.assetRegistry.findByPath(browserState.selectedPath);
            if (asset != nullptr) {
                ImGui::Text("Asset Type: %s", asset->typeName.c_str());
                int usageCount = 0;
                for (const GameObject& object : sceneState.objects) {
                    if (object.textureResourceId == asset->id ||
                        object.scriptResourceId == asset->id ||
                        object.texturePath == asset->sourcePath ||
                        (!asset->relativePath.empty() && object.texturePath == asset->relativePath) ||
                        object.scriptPath == asset->sourcePath) {
                        ++usageCount;
                    }
                }
                ImGui::Text("Used By Objects: %d", usageCount);
                if (asset->type == AssetType::Texture && ImGui::Button("Bind Texture To Selected Object")) {
                    AssignAssetToSelection(sceneState, editorState, *asset);
                }
                if (asset->type == AssetType::Texture) {
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
            if (IsSceneFile(selected) && ImGui::Button("Open Scene File")) {
                OpenScene(sceneState, editorState, browserState.selectedPath);
            }
            if (IsTextEditable(selected)) {
                if (browserState.loadedTextPath != browserState.selectedPath && LoadTextFile(browserState.selectedPath, browserState.textBuffer)) {
                    browserState.loadedTextPath = browserState.selectedPath;
                    browserState.textDirty = false;
                }
                if (ImGui::InputTextMultiline("##FileEditor", browserState.textBuffer.data(), browserState.textBuffer.size(), ImVec2(-1.0f, 180.0f))) {
                    browserState.textDirty = true;
                }
                if (browserState.textDirty) ImGui::TextDisabled("Unsaved changes");
                if (ImGui::Button("Save File")) {
                    if (SaveTextFile(browserState.selectedPath, browserState.textBuffer)) {
                        browserState.textDirty = false;
                        editorState.assetStatus = "Saved file: " + browserState.selectedPath;
                        editorState.pendingProjectCommand = ProjectCommand::Sync;
                    } else {
                        editorState.assetStatus = "Failed to save file";
                    }
                }
            }
        }
    } else {
        ImGui::TextUnformatted("Select a file or folder to inspect it.");
    }

    ImGui::End();
}
