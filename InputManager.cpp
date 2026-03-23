#include "InputManager.h"
#include "WindowManager.h"
#include <iostream>

InputManager::InputManager() : quitRequested(false) {}

void InputManager::processEvents(WindowManager& windowManager) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            quitRequested = true;
            break;

        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_ESCAPE) {
                quitRequested = true;
            }
            else if (event.key.key == SDLK_F) {
                windowManager.toggleFullscreen();
                std::cout << "Toggled fullscreen mode" << std::endl;
            }
            else if (event.key.key == SDLK_1) {
                windowManager.resize(800, 600);
                std::cout << "Window resized to 800x600" << std::endl;
            }
            else if (event.key.key == SDLK_2) {
                windowManager.resize(1024, 768);
                std::cout << "Window resized to 1024x768" << std::endl;
            }
            else if (event.key.key == SDLK_3) {
                windowManager.resize(1280, 720);
                std::cout << "Window resized to 1280x720" << std::endl;
            }
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            std::cout << "Window resized event: "
                << event.window.data1 << " x "
                << event.window.data2 << std::endl;
            break;

        case SDL_EVENT_WINDOW_MINIMIZED:
            std::cout << "Window minimized" << std::endl;
            break;

        case SDL_EVENT_WINDOW_MAXIMIZED:
            std::cout << "Window maximized" << std::endl;
            break;

        case SDL_EVENT_WINDOW_RESTORED:
            std::cout << "Window restored" << std::endl;
            break;

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            std::cout << "Window focus gained" << std::endl;
            break;

        case SDL_EVENT_WINDOW_FOCUS_LOST:
            std::cout << "Window focus lost" << std::endl;
            break;
        }
    }
}

bool InputManager::shouldQuit() const {
    return quitRequested;
}