#include "WindowManager.h"

#include "../render/GraphicsContext.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>

WindowManager::~WindowManager() {
    Shutdown();
}

bool WindowManager::Init(const WindowSpecification& specification) {
    m_Specification = specification;

    if (glfwInit() == GLFW_FALSE) {
        std::cerr << "glfwInit failed." << std::endl;
        return false;
    }

    if (m_Specification.API == GraphicsAPI::OpenGL) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }

    m_Window = glfwCreateWindow(
        static_cast<int>(m_Specification.Width),
        static_cast<int>(m_Specification.Height),
        m_Specification.Title.c_str(),
        nullptr,
        nullptr
    );

    if (m_Window == nullptr) {
        std::cerr << "glfwCreateWindow failed." << std::endl;
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);
    glfwSetScrollCallback(m_Window, ScrollCallback);

    m_Context = GraphicsContext::Create(m_Window, m_Specification.API);
    m_Context->Init();
    if (m_Specification.Fullscreen) {
        ToggleFullscreen();
    } else if (m_Specification.Maximized) {
        glfwMaximizeWindow(m_Window);
    }
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(m_Window, &framebufferWidth, &framebufferHeight);
    m_FramebufferWidth = framebufferWidth > 0 ? static_cast<unsigned int>(framebufferWidth) : 0U;
    m_FramebufferHeight = framebufferHeight > 0 ? static_cast<unsigned int>(framebufferHeight) : 0U;
    SetVSync(m_Specification.VSync);
    m_Initialized = true;
    return true;
}

void WindowManager::Shutdown() {
    if (!m_Initialized && m_Window == nullptr) {
        return;
    }

    m_Context.reset();

    if (m_Window != nullptr) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }

    glfwTerminate();
    m_Initialized = false;
}

void WindowManager::PollEvents() {
    glfwPollEvents();
}

void WindowManager::OnUpdate() {
    if (m_Context) {
        m_Context->SwapBuffers();
    }
}

bool WindowManager::ShouldClose() const {
    return m_Window == nullptr || glfwWindowShouldClose(m_Window) != 0;
}

void WindowManager::ToggleFullscreen() {
    if (m_Window == nullptr) {
        return;
    }

    m_Fullscreen = !m_Fullscreen;
    if (m_Fullscreen) {
        glfwGetWindowPos(m_Window, &m_WindowedX, &m_WindowedY);
        glfwGetWindowSize(m_Window, &m_WindowedWidth, &m_WindowedHeight);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_Window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(m_Window, nullptr, m_WindowedX, m_WindowedY, m_WindowedWidth, m_WindowedHeight, 0);
    }
}

void WindowManager::Resize(int width, int height) {
    if (m_Window == nullptr) {
        return;
    }

    glfwSetWindowSize(m_Window, width, height);
}

void WindowManager::SetVSync(bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
    m_Specification.VSync = enabled;
}

bool WindowManager::IsVSync() const {
    return m_Specification.VSync;
}

unsigned int WindowManager::GetWidth() const {
    return m_Specification.Width;
}

unsigned int WindowManager::GetHeight() const {
    return m_Specification.Height;
}

unsigned int WindowManager::GetFramebufferWidth() const {
    return m_FramebufferWidth;
}

unsigned int WindowManager::GetFramebufferHeight() const {
    return m_FramebufferHeight;
}

bool WindowManager::IsMinimized() const {
    return m_FramebufferWidth == 0 || m_FramebufferHeight == 0;
}

void WindowManager::SetFramebufferResizeCallback(FramebufferResizeCallback callback) {
    m_FramebufferResizeCallback = std::move(callback);
}

void WindowManager::SetMouseScrollCallback(MouseScrollCallback callback) {
    m_MouseScrollCallback = std::move(callback);
}

GraphicsAPI WindowManager::GetGraphicsAPI() const {
    return m_Specification.API;
}

GLFWwindow* WindowManager::GetNativeWindow() const {
    return m_Window;
}

void WindowManager::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* windowManager = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (windowManager == nullptr) {
        return;
    }

    windowManager->m_FramebufferWidth = width > 0 ? static_cast<unsigned int>(width) : 0U;
    windowManager->m_FramebufferHeight = height > 0 ? static_cast<unsigned int>(height) : 0U;

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    windowManager->m_Specification.Width = windowWidth > 0 ? static_cast<unsigned int>(windowWidth) : 0U;
    windowManager->m_Specification.Height = windowHeight > 0 ? static_cast<unsigned int>(windowHeight) : 0U;

    if (windowManager->m_FramebufferResizeCallback) {
        windowManager->m_FramebufferResizeCallback(windowManager->m_FramebufferWidth, windowManager->m_FramebufferHeight);
    }
}

void WindowManager::ScrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    auto* windowManager = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    if (windowManager == nullptr || !windowManager->m_MouseScrollCallback) {
        return;
    }

    windowManager->m_MouseScrollCallback(static_cast<float>(xOffset), static_cast<float>(yOffset));
}
