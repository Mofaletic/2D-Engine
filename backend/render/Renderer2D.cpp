#include "Renderer2D.h"
#include <iostream>

Renderer2D::Renderer2D()
    : renderer(nullptr), sceneRenderTarget(nullptr), sceneRenderTargetWidth(0), sceneRenderTargetHeight(0) {}

bool Renderer2D::init(SDL_Window* window) {
    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        return false;
    }

    int windowWidth = 0;
    int windowHeight = 0;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    if (!resizeSceneRenderTarget(windowWidth, windowHeight)) {
        return false;
    }

    return true;
}

void Renderer2D::clear() {
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_RenderClear(renderer);
}

void Renderer2D::drawTexture(SDL_Texture* texture) {
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
}

bool Renderer2D::resizeSceneRenderTarget(int width, int height) {
    width = (width > 0) ? width : 1;
    height = (height > 0) ? height : 1;

    if (sceneRenderTarget &&
        sceneRenderTargetWidth == width &&
        sceneRenderTargetHeight == height) {
        return true;
    }

    if (sceneRenderTarget) {
        SDL_DestroyTexture(sceneRenderTarget);
        sceneRenderTarget = nullptr;
    }

    sceneRenderTarget = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width,
        height
    );

    if (!sceneRenderTarget) {
        std::cerr << "SDL_CreateTexture (scene render target) failed: "
            << SDL_GetError() << std::endl;
        sceneRenderTargetWidth = 0;
        sceneRenderTargetHeight = 0;
        return false;
    }

    sceneRenderTargetWidth = width;
    sceneRenderTargetHeight = height;
    SDL_SetTextureScaleMode(sceneRenderTarget, SDL_SCALEMODE_LINEAR);
    return true;
}

void Renderer2D::renderScene(const SceneState& sceneState, ResourceManager& resourceManager) {
    if (!sceneRenderTarget) {
        return;
    }

    SDL_SetRenderTarget(renderer, sceneRenderTarget);
    SDL_SetRenderDrawColor(renderer, 30, 30, 35, 255);
    SDL_RenderClear(renderer);

    for (const auto& obj : sceneState.objects) {
        SDL_Texture* texture = resourceManager.getTexture(obj.texturePath, renderer);
        if (!texture) continue;

        SDL_FRect dst;
        dst.x = obj.position[0];
        dst.y = obj.position[1];
        dst.w = 64.0f * obj.scale[0];
        dst.h = 64.0f * obj.scale[1];

        SDL_RenderTexture(renderer, texture, nullptr, &dst);
    }

    SDL_SetRenderTarget(renderer, nullptr);
}

SDL_Texture* Renderer2D::getSceneRenderTarget() const {
    return sceneRenderTarget;
}

void Renderer2D::present() {
    SDL_RenderPresent(renderer);
}

void Renderer2D::destroy() {
    if (sceneRenderTarget) {
        SDL_DestroyTexture(sceneRenderTarget);
        sceneRenderTarget = nullptr;
        sceneRenderTargetWidth = 0;
        sceneRenderTargetHeight = 0;
    }

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
}

SDL_Renderer* Renderer2D::getRenderer() const {
    return renderer;
}
