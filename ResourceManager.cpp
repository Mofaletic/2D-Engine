#include "ResourceManager.h"
#include <SDL3_image/SDL_image.h>
#include <iostream>

ResourceManager::ResourceManager() : texture(nullptr) {}

SDL_Texture* ResourceManager::loadTexture(const std::string& path, SDL_Renderer* renderer) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "IMG_Load failed: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (!texture) {
        std::cerr << "SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    return texture;
}

SDL_Texture* ResourceManager::getTexture() const {
    return texture;
}

void ResourceManager::destroy() {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}