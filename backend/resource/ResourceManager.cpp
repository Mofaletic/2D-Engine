#include "ResourceManager.h"

#include "ResourcePathUtils.h"

#include <SDL3_image/SDL_image.h>

#include <algorithm>
#include <filesystem>
#include <utility>

namespace fs = std::filesystem;

ResourceManager::ResourceManager() {
    addSearchPath(".");
    addSearchPath("asset");
    addSearchPath("asset/image");
}

ResourceManager::~ResourceManager() {
    releaseAllTextures();
}

void ResourceManager::setRenderer(SDL_Renderer* renderer) {
    renderer_ = renderer;
}

SDL_Renderer* ResourceManager::getRenderer() const {
    return renderer_;
}

void ResourceManager::addSearchPath(const std::string& path) {
    if (path.empty()) {
        return;
    }

    const std::string normalized = ResourcePathUtils::NormalizePathString(path);
    if (std::find(searchPaths_.begin(), searchPaths_.end(), normalized) != searchPaths_.end()) {
        return;
    }

    searchPaths_.push_back(normalized);
    clearPathCache();
}

void ResourceManager::setSearchPaths(const std::vector<std::string>& paths) {
    searchPaths_.clear();
    for (const std::string& path : paths) {
        addSearchPath(path);
    }
    clearPathCache();
}

void ResourceManager::clearSearchPaths() {
    searchPaths_.clear();
    clearPathCache();
}

const std::vector<std::string>& ResourceManager::getSearchPaths() const {
    return searchPaths_;
}

SDL_Texture* ResourceManager::loadTexture(const std::string& identifier) {
    return loadTextureInternal(identifier);
}

SDL_Texture* ResourceManager::getTexture(const std::string& identifier, SDL_Renderer* renderer) {
    if (renderer != nullptr) {
        setRenderer(renderer);
    }

    return loadTextureInternal(identifier);
}

SDL_Texture* ResourceManager::findTexture(const std::string& identifier) const {
    const TextureResourceInfo* info = findTextureInfo(identifier);
    return info ? info->texture : nullptr;
}

const TextureResourceInfo* ResourceManager::findTextureInfo(const std::string& identifier) const {
    const std::string resolvedPath = lookupResolvedPath(identifier);
    if (resolvedPath.empty()) {
        return nullptr;
    }

    const auto it = textures_.find(resolvedPath);
    if (it == textures_.end()) {
        return nullptr;
    }

    tempTextureInfo_.texture = it->second.texture;
    tempTextureInfo_.resolvedPath = it->second.resolvedPath;
    tempTextureInfo_.width = it->second.width;
    tempTextureInfo_.height = it->second.height;
    return &tempTextureInfo_;
}

bool ResourceManager::hasTexture(const std::string& identifier) const {
    return findTexture(identifier) != nullptr;
}

std::string ResourceManager::resolveTexturePath(const std::string& identifier) const {
    if (identifier.empty()) {
        return {};
    }

    const auto cacheIt = resolvedPathCache_.find(identifier);
    if (cacheIt != resolvedPathCache_.end()) {
        return cacheIt->second;
    }

    const std::string resolved = ResourcePathUtils::ResolvePath(identifier, searchPaths_);
    resolvedPathCache_[identifier] = resolved;
    return resolved;
}

std::vector<std::string> ResourceManager::collectTextureCandidates(const std::string& identifier) const {
    std::vector<std::string> candidates;
    if (identifier.empty()) {
        return candidates;
    }

    candidates.push_back(identifier);

    const std::vector<fs::path> roots = ResourcePathUtils::BuildSearchRoots(searchPaths_);
    for (const fs::path& root : roots) {
        const std::string candidate = ResourcePathUtils::NormalizePathString(root / identifier);
        if (std::find(candidates.begin(), candidates.end(), candidate) == candidates.end()) {
            candidates.push_back(candidate);
        }
    }

    return candidates;
}

bool ResourceManager::releaseTexture(const std::string& identifier) {
    const std::string resolvedPath = lookupResolvedPath(identifier);
    if (resolvedPath.empty()) {
        return false;
    }

    destroyRecord(resolvedPath);
    return true;
}

void ResourceManager::releaseAllTextures() {
    for (auto& [_, record] : textures_) {
        if (record.texture != nullptr) {
            SDL_DestroyTexture(record.texture);
            record.texture = nullptr;
        }
    }

    textures_.clear();
    aliasToResolvedPath_.clear();
}

void ResourceManager::destroy() {
    releaseAllTextures();
}

std::size_t ResourceManager::getTextureCount() const {
    return textures_.size();
}

const std::string& ResourceManager::getLastError() const {
    return lastError_;
}

SDL_Texture* ResourceManager::loadTextureInternal(const std::string& identifier) {
    if (identifier.empty()) {
        setLastError("Texture identifier is empty.");
        return nullptr;
    }

    if (renderer_ == nullptr) {
        setLastError("Renderer is not set for ResourceManager.");
        return nullptr;
    }

    if (SDL_Texture* cached = findTexture(identifier); cached != nullptr) {
        lastError_.clear();
        return cached;
    }

    const std::string resolvedPath = resolveTexturePath(identifier);
    if (resolvedPath.empty()) {
        setLastError(std::string("Unable to resolve texture path: ") + identifier);
        return nullptr;
    }

    SDL_Surface* surface = IMG_Load(resolvedPath.c_str());
    if (surface == nullptr) {
        setLastError(std::string("IMG_Load failed for '") + resolvedPath + "': " + SDL_GetError());
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (texture == nullptr) {
        const std::string message =
            std::string("SDL_CreateTextureFromSurface failed for '") + resolvedPath + "': " +
            SDL_GetError();
        SDL_DestroySurface(surface);
        setLastError(message);
        return nullptr;
    }

    TextureRecord record;
    record.texture = texture;
    record.resolvedPath = resolvedPath;
    record.width = surface->w;
    record.height = surface->h;

    SDL_DestroySurface(surface);

    textures_[resolvedPath] = std::move(record);
    registerAlias(identifier, resolvedPath);
    registerAlias(resolvedPath, resolvedPath);
    lastError_.clear();
    return textures_[resolvedPath].texture;
}

void ResourceManager::registerAlias(const std::string& alias, const std::string& resolvedPath) {
    aliasToResolvedPath_[alias] = resolvedPath;

    auto it = textures_.find(resolvedPath);
    if (it != textures_.end()) {
        it->second.aliases.insert(alias);
    }
}

std::string ResourceManager::lookupResolvedPath(const std::string& identifier) const {
    const auto aliasIt = aliasToResolvedPath_.find(identifier);
    if (aliasIt != aliasToResolvedPath_.end()) {
        return aliasIt->second;
    }

    const std::string resolved = resolveTexturePath(identifier);
    if (resolved.empty()) {
        return {};
    }

    return textures_.contains(resolved) ? resolved : std::string{};
}

void ResourceManager::destroyRecord(const std::string& resolvedPath) {
    const auto it = textures_.find(resolvedPath);
    if (it == textures_.end()) {
        return;
    }

    for (const std::string& alias : it->second.aliases) {
        aliasToResolvedPath_.erase(alias);
    }

    if (it->second.texture != nullptr) {
        SDL_DestroyTexture(it->second.texture);
    }

    textures_.erase(it);
}

void ResourceManager::setLastError(std::string message) {
    lastError_ = std::move(message);
    SDL_Log("ResourceManager: %s", lastError_.c_str());
}

void ResourceManager::clearPathCache() {
    resolvedPathCache_.clear();
}
