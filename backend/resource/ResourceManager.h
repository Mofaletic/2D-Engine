#pragma once

#include <SDL3/SDL.h>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct TextureResourceInfo {
    SDL_Texture* texture = nullptr;
    std::string resolvedPath;
    int width = 0;
    int height = 0;
};

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    void setRenderer(SDL_Renderer* renderer);
    SDL_Renderer* getRenderer() const;

    void addSearchPath(const std::string& path);
    void setSearchPaths(const std::vector<std::string>& paths);
    void clearSearchPaths();
    const std::vector<std::string>& getSearchPaths() const;

    SDL_Texture* loadTexture(const std::string& identifier);
    SDL_Texture* getTexture(const std::string& identifier, SDL_Renderer* renderer = nullptr);
    SDL_Texture* findTexture(const std::string& identifier) const;
    const TextureResourceInfo* findTextureInfo(const std::string& identifier) const;
    bool hasTexture(const std::string& identifier) const;

    std::string resolveTexturePath(const std::string& identifier) const;
    std::vector<std::string> collectTextureCandidates(const std::string& identifier) const;

    bool releaseTexture(const std::string& identifier);
    void releaseAllTextures();
    void destroy();

    std::size_t getTextureCount() const;
    const std::string& getLastError() const;

private:
    struct TextureRecord {
        SDL_Texture* texture = nullptr;
        std::string resolvedPath;
        int width = 0;
        int height = 0;
        std::unordered_set<std::string> aliases;
    };

    SDL_Texture* loadTextureInternal(const std::string& identifier);
    void registerAlias(const std::string& alias, const std::string& resolvedPath);
    std::string lookupResolvedPath(const std::string& identifier) const;
    void destroyRecord(const std::string& resolvedPath);
    void setLastError(std::string message);
    void clearPathCache();

    SDL_Renderer* renderer_ = nullptr;
    std::vector<std::string> searchPaths_;
    std::unordered_map<std::string, TextureRecord> textures_;
    std::unordered_map<std::string, std::string> aliasToResolvedPath_;
    mutable std::unordered_map<std::string, std::string> resolvedPathCache_;
    mutable TextureResourceInfo tempTextureInfo_;
    std::string lastError_;
};
