#include "Renderer2D.h"
#include <iostream>

Renderer2D::Renderer2D() : renderer(nullptr) {}

bool Renderer2D::init(SDL_Window* window) {
    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

void Renderer2D::clear() {
    SDL_RenderClear(renderer);
}

void Renderer2D::drawTexture(SDL_Texture* texture) {
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
}

void Renderer2D::present() {
    SDL_RenderPresent(renderer);
}

void Renderer2D::destroy() {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
}

SDL_Renderer* Renderer2D::getRenderer() const {
    return renderer;
}