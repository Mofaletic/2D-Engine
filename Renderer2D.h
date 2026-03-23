#pragma once
#include <SDL3/SDL.h>

class Renderer2D {
private:
    SDL_Renderer* renderer;

public:
    Renderer2D();
    bool init(SDL_Window* window);
    void clear();
    void drawTexture(SDL_Texture* texture);
    void present();
    void destroy();
    SDL_Renderer* getRenderer() const;
};