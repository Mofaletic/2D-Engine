#pragma once
#include <SDL3/SDL.h>
#include "../core/SceneState.h"
#include "../resource/ResourceManager.h"

class Renderer2D {
private:
    SDL_Renderer* renderer;
    SDL_Texture* sceneRenderTarget;
    int sceneRenderTargetWidth;
    int sceneRenderTargetHeight;

public:
    Renderer2D();
    bool init(SDL_Window* window);
    void clear();
    void drawTexture(SDL_Texture* texture);
    bool resizeSceneRenderTarget(int width, int height);
    void renderScene(const SceneState& sceneState, ResourceManager& resourceManager);
    SDL_Texture* getSceneRenderTarget() const;
    void present();
    void destroy();
    SDL_Renderer* getRenderer() const;
};
