#pragma once
#include <SDL3/SDL.h>
#include <string>

class WindowManager {
private:
    SDL_Window* window;
    bool fullscreen;

public:
    WindowManager();
    bool init(const std::string& title, int width, int height);
    void destroy();
    void toggleFullscreen();
    void resize(int width, int height);
    SDL_Window* getWindow() const;
};