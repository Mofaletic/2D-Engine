#pragma once

#include <functional>
#include <string>

#include "../core/Ref.h"
#include "../render/GraphicsAPI.h"
#include "../render/GraphicsContext.h"

struct GLFWwindow;

struct WindowSpecification {
    std::string Title = "Lancelot Engine";
    unsigned int Width = 1280;
    unsigned int Height = 720;
    bool Fullscreen = false;
    bool Maximized = false;
    bool VSync = true;
    GraphicsAPI API = GraphicsAPI::OpenGL;
};

class WindowManager {
public:
    using FramebufferResizeCallback = std::function<void(unsigned int, unsigned int)>;
    using MouseScrollCallback = std::function<void(float, float)>;

    WindowManager() = default;
    ~WindowManager();

    bool Init(const WindowSpecification& specification);
    void Shutdown();
    void PollEvents();
    void OnUpdate();
    bool ShouldClose() const;
    void ToggleFullscreen();
    void Resize(int width, int height);
    void SetVSync(bool enabled);
    bool IsVSync() const;
    unsigned int GetWidth() const;
    unsigned int GetHeight() const;
    unsigned int GetFramebufferWidth() const;
    unsigned int GetFramebufferHeight() const;
    bool IsMinimized() const;
    void SetFramebufferResizeCallback(FramebufferResizeCallback callback);
    void SetMouseScrollCallback(MouseScrollCallback callback);
    GraphicsAPI GetGraphicsAPI() const;
    GLFWwindow* GetNativeWindow() const;

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset);

private:
    WindowSpecification m_Specification;
    GLFWwindow* m_Window = nullptr;
    Scope<GraphicsContext> m_Context;
    bool m_Initialized = false;
    bool m_Fullscreen = false;
    int m_WindowedX = 100;
    int m_WindowedY = 100;
    int m_WindowedWidth = 1280;
    int m_WindowedHeight = 720;
    unsigned int m_FramebufferWidth = 0;
    unsigned int m_FramebufferHeight = 0;
    FramebufferResizeCallback m_FramebufferResizeCallback;
    MouseScrollCallback m_MouseScrollCallback;
};
