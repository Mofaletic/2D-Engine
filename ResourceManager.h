#pragma once
#include <SDL3/SDL.h>
#include <string>

class ResourceManager {
private:
    SDL_Texture* texture;

public:
    ResourceManager();
    SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* renderer);
    SDL_Texture* getTexture() const;
    void destroy();
};