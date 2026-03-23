#pragma once
#include <SDL3/SDL.h>

class WindowManager;

class InputManager {
private:
    bool quitRequested;

public:
    InputManager();
    void processEvents(WindowManager& windowManager);
    bool shouldQuit() const;
};