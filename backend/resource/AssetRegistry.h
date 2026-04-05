#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum class AssetType {
    Unknown,
    Texture,
    Audio,
    Text,
    Scene
};

struct AssetRecord {
    std::uint64_t id = 0;
    std::string name;
    AssetType type = AssetType::Unknown;
    std::string typeName;
    std::string sourcePath;
    std::string relativePath;
};

struct AssetSyncSummary {
    std::size_t addedCount = 0;
    std::size_t removedCount = 0;
    std::vector<AssetRecord> removedAssets;
};

class AssetRegistry {
public:
    AssetRegistry();

    void clear();
    void setProjectAssetRoot(const std::string& folderPath);
    const std::string& getProjectAssetRoot() const;

    std::size_t importFolder(const std::string& folderPath);
    std::size_t importFolderToProject(const std::string& folderPath);
    std::size_t importFilesToProject(const std::vector<std::string>& filePaths);
    bool registerFile(const std::string& filePath);
    bool loadManifest(const std::string& manifestPath);
    bool saveManifest(const std::string& manifestPath) const;
    std::size_t rebuildFromProjectAssets();
    AssetSyncSummary synchronizeProjectAssets();

    const std::vector<AssetRecord>& getAssets() const;
    const AssetRecord* findById(std::uint64_t id) const;
    const AssetRecord* findByPath(const std::string& path) const;

    std::size_t getAssetCount() const;
    const std::string& getLastImportedFolder() const;
    const std::string& getLastError() const;

private:
    AssetType detectAssetType(const std::string& extension) const;
    std::string buildRelativePath(const std::string& absolutePath) const;
    std::string buildProjectRelativePath(const std::string& absolutePath) const;
    std::string copyFileToProject(const std::string& sourceFilePath, const std::string& subdirectory);
    std::uint64_t registerResolvedFile(const std::string& filePath);
    void rebuildIndexes();
    static std::string normalizePath(const std::string& path);
    static std::string assetTypeToString(AssetType type);

    std::uint64_t nextId_ = 1;
    std::vector<AssetRecord> assets_;
    std::unordered_map<std::string, std::size_t> pathToIndex_;
    std::unordered_map<std::uint64_t, std::size_t> idToIndex_;
    std::string lastImportedFolder_;
    std::string lastError_;
    std::string projectAssetRoot_ = "asset/imported";
};
