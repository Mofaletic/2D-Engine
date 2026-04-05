#include "AssetRegistry.h"

#include "ResourcePathUtils.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;
using json = nlohmann::json;

AssetRegistry::AssetRegistry() = default;

void AssetRegistry::clear() {
    nextId_ = 1;
    assets_.clear();
    pathToIndex_.clear();
    idToIndex_.clear();
    lastImportedFolder_.clear();
    lastError_.clear();
}

void AssetRegistry::setProjectAssetRoot(const std::string& folderPath) {
    if (!folderPath.empty()) {
        projectAssetRoot_ = normalizePath(folderPath);
    }
}

const std::string& AssetRegistry::getProjectAssetRoot() const {
    return projectAssetRoot_;
}

std::size_t AssetRegistry::importFolder(const std::string& folderPath) {
    lastError_.clear();

    if (folderPath.empty()) {
        lastError_ = "Folder path is empty.";
        return 0;
    }

    const std::string normalizedFolder = normalizePath(folderPath);
    std::error_code ec;
    const fs::path folder = ResourcePathUtils::Utf8ToPath(normalizedFolder);
    if (!fs::exists(folder, ec) || !fs::is_directory(folder, ec)) {
        lastError_ = "Folder does not exist: " + normalizedFolder;
        return 0;
    }

    lastImportedFolder_ = normalizedFolder;

    std::size_t importedCount = 0;
    fs::recursive_directory_iterator it(folder, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;
    if (ec) {
        lastError_ = "Failed to read folder: " + normalizedFolder;
        return 0;
    }

    for (; it != end; it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        if (!it->is_regular_file(ec)) {
            continue;
        }

        if (registerFile(it->path().string())) {
            ++importedCount;
        }
    }

    return importedCount;
}

std::size_t AssetRegistry::importFolderToProject(const std::string& folderPath) {
    lastError_.clear();

    if (folderPath.empty()) {
        lastError_ = "Folder path is empty.";
        return 0;
    }

    const std::string normalizedFolder = normalizePath(folderPath);
    const fs::path sourceRoot(normalizedFolder);
    std::error_code ec;
    if (!fs::exists(sourceRoot, ec) || !fs::is_directory(sourceRoot, ec)) {
        lastError_ = "Folder does not exist: " + normalizedFolder;
        return 0;
    }

    lastImportedFolder_ = normalizedFolder;

    const std::string folderName = sourceRoot.filename().string();
    std::size_t importedCount = 0;
    fs::recursive_directory_iterator it(sourceRoot, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;
    if (ec) {
        lastError_ = "Failed to read folder: " + normalizedFolder;
        return 0;
    }

    for (; it != end; it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        if (!it->is_regular_file(ec)) {
            continue;
        }

        std::string filename = it->path().filename().string();
        std::string extension = it->path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        const AssetType type = filename.ends_with(".scene.json") ? AssetType::Scene : detectAssetType(extension);
        if (type == AssetType::Unknown) {
            continue;
        }

        std::error_code relativeEc;
        fs::path relativeSubdir = fs::relative(it->path().parent_path(), sourceRoot, relativeEc);
        std::string subdirectory = folderName;
        if (!relativeEc && !relativeSubdir.empty() && relativeSubdir.generic_string() != ".") {
            subdirectory += "/" + relativeSubdir.generic_string();
        }

        const std::string copiedPath = copyFileToProject(it->path().string(), subdirectory);
        if (!copiedPath.empty() && registerResolvedFile(copiedPath) != 0) {
            ++importedCount;
        }
    }

    return importedCount;
}

std::size_t AssetRegistry::importFilesToProject(const std::vector<std::string>& filePaths) {
    lastError_.clear();

    std::size_t importedCount = 0;
    for (const std::string& filePath : filePaths) {
        const std::string copiedPath = copyFileToProject(filePath, {});
        if (!copiedPath.empty() && registerResolvedFile(copiedPath) != 0) {
            ++importedCount;
        }
    }

    if (importedCount == 0 && lastError_.empty()) {
        lastError_ = "No supported files were imported.";
    }

    return importedCount;
}

bool AssetRegistry::registerFile(const std::string& filePath) {
    lastError_.clear();
    return registerResolvedFile(filePath) != 0;
}

bool AssetRegistry::loadManifest(const std::string& manifestPath) {
    lastError_.clear();

    std::ifstream file(ResourcePathUtils::Utf8ToPath(manifestPath));
    if (!file.is_open()) {
        lastError_ = "Manifest not found: " + manifestPath;
        return false;
    }

    json data;
    file >> data;

    clear();
    if (data.contains("projectAssetRoot")) {
        setProjectAssetRoot(data.value("projectAssetRoot", projectAssetRoot_));
    }
    nextId_ = data.value("nextId", static_cast<std::uint64_t>(1));

    if (data.contains("assets") && data["assets"].is_array()) {
        for (const auto& item : data["assets"]) {
            AssetRecord record;
            record.id = item.value("id", static_cast<std::uint64_t>(0));
            record.name = item.value("name", "");
            record.typeName = item.value("typeName", "Unknown");
            record.sourcePath = normalizePath(item.value("sourcePath", ""));
            record.relativePath = item.value("relativePath", "");
            if (record.typeName == "Texture") record.type = AssetType::Texture;
            else if (record.typeName == "Audio") record.type = AssetType::Audio;
            else if (record.typeName == "Text") record.type = AssetType::Text;
            else if (record.typeName == "Scene") record.type = AssetType::Scene;
            else record.type = AssetType::Unknown;

            if (!record.sourcePath.empty()) {
                assets_.push_back(record);
            }
        }
    }

    rebuildIndexes();
    return true;
}

bool AssetRegistry::saveManifest(const std::string& manifestPath) const {
    json data;
    data["projectAssetRoot"] = projectAssetRoot_;
    data["nextId"] = nextId_;
    data["assets"] = json::array();

    for (const AssetRecord& record : assets_) {
        json item;
        item["id"] = record.id;
        item["name"] = record.name;
        item["typeName"] = record.typeName;
        item["sourcePath"] = record.sourcePath;
        item["relativePath"] = record.relativePath;
        data["assets"].push_back(item);
    }

    std::error_code ec;
    const fs::path manifestFile = ResourcePathUtils::Utf8ToPath(manifestPath);
    if (manifestFile.has_parent_path()) {
        fs::create_directories(manifestFile.parent_path(), ec);
    }

    std::ofstream file(manifestPath);
    if (!file.is_open()) {
        return false;
    }

    file << data.dump(4);
    return true;
}

std::size_t AssetRegistry::rebuildFromProjectAssets() {
    lastError_.clear();

    const std::string root = normalizePath(projectAssetRoot_);
    std::error_code ec;
    if (!fs::exists(root, ec)) {
        return 0;
    }

    std::size_t importedCount = 0;
    fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;
    if (ec) {
        lastError_ = "Failed to read project assets: " + root;
        return 0;
    }

    for (; it != end; it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        if (!it->is_regular_file(ec)) {
            continue;
        }

        std::string filename = it->path().filename().string();
        std::string extension = it->path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        const AssetType type = filename.ends_with(".scene.json") ? AssetType::Scene : detectAssetType(extension);
        if (type == AssetType::Unknown) {
            continue;
        }

        if (registerResolvedFile(it->path().string()) != 0) {
            ++importedCount;
        }
    }

    return importedCount;
}

AssetSyncSummary AssetRegistry::synchronizeProjectAssets() {
    AssetSyncSummary summary;
    lastError_.clear();

    const std::string root = normalizePath(projectAssetRoot_);
    std::error_code ec;
    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) {
        if (!assets_.empty()) {
            summary.removedAssets = assets_;
            summary.removedCount = assets_.size();
            assets_.clear();
            rebuildIndexes();
        }
        return summary;
    }

    std::vector<std::string> discoveredFiles;
    fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;
    if (ec) {
        lastError_ = "Failed to sync project assets: " + root;
        return summary;
    }

    for (; it != end; it.increment(ec)) {
        if (ec) {
            ec.clear();
            continue;
        }

        if (!it->is_regular_file(ec)) {
            continue;
        }

        std::string filename = it->path().filename().string();
        std::string extension = it->path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        const AssetType type = filename.ends_with(".scene.json") ? AssetType::Scene : detectAssetType(extension);
        if (type == AssetType::Unknown) {
            continue;
        }

        discoveredFiles.push_back(normalizePath(it->path().string()));
    }

    std::sort(discoveredFiles.begin(), discoveredFiles.end());
    discoveredFiles.erase(std::unique(discoveredFiles.begin(), discoveredFiles.end()), discoveredFiles.end());

    std::unordered_map<std::string, bool> discoveredLookup;
    discoveredLookup.reserve(discoveredFiles.size());
    for (const std::string& file : discoveredFiles) {
        discoveredLookup[file] = true;
    }

    std::vector<AssetRecord> keptAssets;
    keptAssets.reserve(assets_.size());
    for (const AssetRecord& asset : assets_) {
        if (!discoveredLookup.contains(asset.sourcePath)) {
            summary.removedAssets.push_back(asset);
            ++summary.removedCount;
            continue;
        }

        AssetRecord updated = asset;
        const fs::path assetPath = ResourcePathUtils::Utf8ToPath(asset.sourcePath);
        updated.name = assetPath.filename().string();
        updated.relativePath = buildProjectRelativePath(asset.sourcePath);
        keptAssets.push_back(std::move(updated));
    }

    assets_ = std::move(keptAssets);
    rebuildIndexes();

    for (const std::string& file : discoveredFiles) {
        if (pathToIndex_.contains(file)) {
            continue;
        }

        if (registerResolvedFile(file) != 0) {
            ++summary.addedCount;
        }
    }

    return summary;
}

const std::vector<AssetRecord>& AssetRegistry::getAssets() const {
    return assets_;
}

const AssetRecord* AssetRegistry::findById(std::uint64_t id) const {
    const auto it = idToIndex_.find(id);
    if (it == idToIndex_.end()) {
        return nullptr;
    }

    return &assets_[it->second];
}

const AssetRecord* AssetRegistry::findByPath(const std::string& path) const {
    const std::string normalizedPath = normalizePath(path);
    const auto direct = pathToIndex_.find(normalizedPath);
    if (direct != pathToIndex_.end()) {
        return &assets_[direct->second];
    }

    for (const AssetRecord& record : assets_) {
        if (record.relativePath == path || record.name == path) {
            return &record;
        }
    }

    return nullptr;
}

std::size_t AssetRegistry::getAssetCount() const {
    return assets_.size();
}

const std::string& AssetRegistry::getLastImportedFolder() const {
    return lastImportedFolder_;
}

const std::string& AssetRegistry::getLastError() const {
    return lastError_;
}

AssetType AssetRegistry::detectAssetType(const std::string& extension) const {
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
        extension == ".bmp" || extension == ".gif" || extension == ".tga" ||
        extension == ".webp") {
        return AssetType::Texture;
    }

    if (extension == ".wav" || extension == ".mp3" || extension == ".ogg") {
        return AssetType::Audio;
    }

    if (extension == ".txt" || extension == ".json" || extension == ".md") {
        return AssetType::Text;
    }

    if (extension == ".scene" || extension == ".scene.json") {
        return AssetType::Scene;
    }

    return AssetType::Unknown;
}

std::string AssetRegistry::buildRelativePath(const std::string& absolutePath) const {
    std::error_code ec;
    const fs::path current = fs::current_path(ec);
    const fs::path target = ResourcePathUtils::Utf8ToPath(absolutePath);

    if (!ec) {
        const fs::path relative = fs::relative(target, current, ec);
        if (!ec) {
            return relative.generic_string();
        }
    }

    return target.filename().string();
}

std::string AssetRegistry::buildProjectRelativePath(const std::string& absolutePath) const {
    std::error_code ec;
    const fs::path target = ResourcePathUtils::Utf8ToPath(absolutePath);
    const fs::path projectRoot = ResourcePathUtils::Utf8ToPath(projectAssetRoot_);
    const fs::path relative = fs::relative(target, projectRoot, ec);
    if (!ec && !relative.empty()) {
        return relative.generic_string();
    }

    return buildRelativePath(absolutePath);
}

std::string AssetRegistry::copyFileToProject(const std::string& sourceFilePath, const std::string& subdirectory) {
    std::error_code ec;
    const fs::path sourcePath = ResourcePathUtils::Utf8ToPath(normalizePath(sourceFilePath));
    if (!fs::exists(sourcePath, ec) || !fs::is_regular_file(sourcePath, ec)) {
        lastError_ = "File does not exist: " + sourcePath.string();
        return {};
    }

    std::string filename = sourcePath.filename().string();
    std::string extension = sourcePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const AssetType sourceType = filename.ends_with(".scene.json") ? AssetType::Scene : detectAssetType(extension);
    if (sourceType == AssetType::Unknown) {
        return {};
    }

    fs::path targetDir = ResourcePathUtils::Utf8ToPath(projectAssetRoot_);
    if (!subdirectory.empty()) {
        targetDir /= ResourcePathUtils::Utf8ToPath(subdirectory);
    }
    fs::create_directories(targetDir, ec);
    if (ec) {
        lastError_ = "Failed to create asset directory: " + targetDir.string();
        return {};
    }

    fs::path targetPath = targetDir / sourcePath.filename();
    if (fs::exists(targetPath, ec)) {
        if (normalizePath(ResourcePathUtils::PathToUtf8String(targetPath)) ==
            normalizePath(ResourcePathUtils::PathToUtf8String(sourcePath))) {
            return normalizePath(ResourcePathUtils::PathToUtf8String(targetPath));
        }

        const std::string stem = sourcePath.stem().string();
        const std::string ext = sourcePath.extension().string();
        int suffix = 1;
        do {
            targetPath = targetDir / ResourcePathUtils::Utf8ToPath(stem + "_" + std::to_string(suffix) + ext);
            ++suffix;
        } while (fs::exists(targetPath, ec));
    }

    fs::copy_file(sourcePath, targetPath, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        lastError_ = "Failed to copy file into project: " + sourcePath.string();
        return {};
    }

    return normalizePath(ResourcePathUtils::PathToUtf8String(targetPath));
}

std::uint64_t AssetRegistry::registerResolvedFile(const std::string& filePath) {
    lastError_.clear();

    const std::string normalizedPath = normalizePath(filePath);
    const fs::path absolutePath = ResourcePathUtils::Utf8ToPath(normalizedPath);
    std::error_code ec;
    if (!fs::exists(absolutePath, ec) || !fs::is_regular_file(absolutePath, ec)) {
        lastError_ = "File does not exist: " + normalizedPath;
        return 0;
    }

    const auto existing = pathToIndex_.find(normalizedPath);
    if (existing != pathToIndex_.end()) {
        AssetRecord& record = assets_[existing->second];
        record.name = absolutePath.filename().string();
        record.relativePath = buildProjectRelativePath(normalizedPath);
        return record.id;
    }

    std::string filename = absolutePath.filename().string();
    std::string extension = absolutePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    const AssetType type = filename.ends_with(".scene.json") ? AssetType::Scene : detectAssetType(extension);
    if (type == AssetType::Unknown) {
        return 0;
    }

    AssetRecord record;
    record.id = nextId_++;
    record.name = absolutePath.filename().string();
    record.type = type;
    record.typeName = assetTypeToString(type);
    record.sourcePath = normalizedPath;
    record.relativePath = buildProjectRelativePath(normalizedPath);

    const std::size_t index = assets_.size();
    assets_.push_back(record);
    pathToIndex_[record.sourcePath] = index;
    idToIndex_[record.id] = index;
    return record.id;
}

void AssetRegistry::rebuildIndexes() {
    pathToIndex_.clear();
    idToIndex_.clear();

    std::uint64_t maxId = 0;
    for (std::size_t i = 0; i < assets_.size(); ++i) {
        pathToIndex_[assets_[i].sourcePath] = i;
        idToIndex_[assets_[i].id] = i;
        if (assets_[i].id > maxId) {
            maxId = assets_[i].id;
        }
    }

    if (nextId_ <= maxId) {
        nextId_ = maxId + 1;
    }
}

std::string AssetRegistry::normalizePath(const std::string& path) {
    return ResourcePathUtils::NormalizePathString(ResourcePathUtils::Utf8ToPath(path));
}

std::string AssetRegistry::assetTypeToString(AssetType type) {
    switch (type) {
    case AssetType::Texture:
        return "Texture";
    case AssetType::Audio:
        return "Audio";
    case AssetType::Text:
        return "Text";
    case AssetType::Scene:
        return "Scene";
    case AssetType::Unknown:
    default:
        return "Unknown";
    }
}
