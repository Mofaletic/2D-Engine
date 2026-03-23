#pragma once
#include "WindowManager.h"
#include "Renderer2D.h"
#include "InputManager.h"
#include "ResourceManager.h"

class Engine {
private:
    WindowManager windowManager;
    Renderer2D renderer2D;
    InputManager inputManager;
    ResourceManager resourceManager;
    bool running;

public:
    Engine();
    bool init();
    void run();
    void shutdown();
};