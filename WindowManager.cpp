#include "WindowManager.h"
#include <iostream>

WindowManager::WindowManager() : window(nullptr), fullscreen(false) {}

bool WindowManager::init(const std::string& title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    return true;
}

void WindowManager::destroy() {
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

void WindowManager::toggleFullscreen() {
    fullscreen = !fullscreen;
    SDL_SetWindowFullscreen(window, fullscreen);
}

void WindowManager::resize(int width, int height) {
    SDL_SetWindowSize(window, width, height);
}

SDL_Window* WindowManager::getWindow() const {
    return window;
}