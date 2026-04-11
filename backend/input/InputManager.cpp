#include "InputManager.h"

#include "../core/SceneState.h"
#include "../window/WindowManager.h"
#include "../../frontend/src/EditorState.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

constexpr float kSceneObjectBaseSize = 64.0f;

bool PointInRect(float x, float y, float left, float top, float width, float height) {
    return x >= left && y >= top && x <= left + width && y <= top + height;
}

bool MapScreenToScenePoint(
    float screenX,
    float screenY,
    const EditorState& editorState,
    float& sceneX,
    float& sceneY,
    bool clampToViewport = false) {
    if (editorState.sceneViewportScreenWidth <= 0.0f || editorState.sceneViewportScreenHeight <= 0.0f ||
        editorState.sceneViewportWidth <= 0.0f || editorState.sceneViewportHeight <= 0.0f) {
        return false;
    }

    float localX = screenX - editorState.sceneViewportScreenX;
    float localY = screenY - editorState.sceneViewportScreenY;
    if (clampToViewport) {
        localX = std::clamp(localX, 0.0f, editorState.sceneViewportScreenWidth);
        localY = std::clamp(localY, 0.0f, editorState.sceneViewportScreenHeight);
    } else if (!PointInRect(
        screenX,
        screenY,
        editorState.sceneViewportScreenX,
        editorState.sceneViewportScreenY,
        editorState.sceneViewportScreenWidth,
        editorState.sceneViewportScreenHeight)) {
        return false;
    }

    const float zoom = editorState.sceneViewZoom > 0.0f ? editorState.sceneViewZoom : 1.0f;
    sceneX = (localX - editorState.sceneViewOffsetX) / zoom;
    sceneY = (localY - editorState.sceneViewOffsetY) / zoom;
    return true;
}

int FindTopmostObjectAt(const SceneState& sceneState, float sceneX, float sceneY) {
    for (int index = static_cast<int>(sceneState.objects.size()) - 1; index >= 0; --index) {
        const GameObject& object = sceneState.objects[index];
        const float width = kSceneObjectBaseSize * std::max(object.scale[0], 0.0f);
        const float height = kSceneObjectBaseSize * std::max(object.scale[1], 0.0f);
        if (PointInRect(sceneX, sceneY, object.position[0], object.position[1], width, height)) {
            return index;
        }
    }

    return -1;
}

void StopSceneDrag(EditorState& editorState) {
    editorState.isDraggingSceneObject = false;
    editorState.draggingObjectIndex = -1;
    editorState.sceneDragOffsetX = 0.0f;
    editorState.sceneDragOffsetY = 0.0f;
}

bool IsInsideSceneViewport(float screenX, float screenY, const EditorState& editorState) {
    return PointInRect(
        screenX,
        screenY,
        editorState.sceneViewportScreenX,
        editorState.sceneViewportScreenY,
        editorState.sceneViewportScreenWidth,
        editorState.sceneViewportScreenHeight);
}

void RefreshSceneViewOffset(EditorState& editorState) {
    const float viewportWidth = std::max(editorState.sceneViewportWidth, 1.0f);
    const float viewportHeight = std::max(editorState.sceneViewportHeight, 1.0f);
    editorState.sceneViewOffsetX = viewportWidth * 0.5f - editorState.sceneViewCenterX * editorState.sceneViewZoom;
    editorState.sceneViewOffsetY = viewportHeight * 0.5f - editorState.sceneViewCenterY * editorState.sceneViewZoom;
}

}

InputManager::InputManager() = default;

void InputManager::processEvents(WindowManager& windowManager, SceneState& sceneState, EditorState& editorState) {
    GLFWwindow* nativeWindow = windowManager.GetNativeWindow();
    if (nativeWindow == nullptr) {
        return;
    }

    if (windowManager.ShouldClose()) {
        m_QuitRequested = true;
    }

    const bool escapePressed = glfwGetKey(nativeWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (escapePressed && !m_PreviousEscapePressed) {
        m_QuitRequested = true;
    }
    m_PreviousEscapePressed = escapePressed;

    const bool fullscreenPressed = glfwGetKey(nativeWindow, GLFW_KEY_F) == GLFW_PRESS;
    if (fullscreenPressed && !m_PreviousFullscreenPressed && !ImGui::GetIO().WantCaptureKeyboard) {
        windowManager.ToggleFullscreen();
        std::cout << "Toggled fullscreen mode" << std::endl;
    }
    m_PreviousFullscreenPressed = fullscreenPressed;

    const bool resize1Pressed = glfwGetKey(nativeWindow, GLFW_KEY_1) == GLFW_PRESS;
    if (resize1Pressed && !m_PreviousResize1Pressed && !ImGui::GetIO().WantCaptureKeyboard) {
        windowManager.Resize(800, 600);
    }
    m_PreviousResize1Pressed = resize1Pressed;

    const bool resize2Pressed = glfwGetKey(nativeWindow, GLFW_KEY_2) == GLFW_PRESS;
    if (resize2Pressed && !m_PreviousResize2Pressed && !ImGui::GetIO().WantCaptureKeyboard) {
        windowManager.Resize(1024, 768);
    }
    m_PreviousResize2Pressed = resize2Pressed;

    const bool resize3Pressed = glfwGetKey(nativeWindow, GLFW_KEY_3) == GLFW_PRESS;
    if (resize3Pressed && !m_PreviousResize3Pressed && !ImGui::GetIO().WantCaptureKeyboard) {
        windowManager.Resize(1280, 720);
    }
    m_PreviousResize3Pressed = resize3Pressed;

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(nativeWindow, &mouseX, &mouseY);

    const bool leftMousePressed = glfwGetMouseButton(nativeWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (leftMousePressed && !m_PreviousLeftMousePressed) {
        float sceneX = 0.0f;
        float sceneY = 0.0f;
        const bool insideSceneViewport = MapScreenToScenePoint(
            static_cast<float>(mouseX),
            static_cast<float>(mouseY),
            editorState,
            sceneX,
            sceneY);

        if (insideSceneViewport) {
            const int hitObjectIndex = FindTopmostObjectAt(sceneState, sceneX, sceneY);
            editorState.selectedObjectIndex = hitObjectIndex;

            if (hitObjectIndex >= 0 && hitObjectIndex < static_cast<int>(sceneState.objects.size())) {
                const GameObject& object = sceneState.objects[hitObjectIndex];
                editorState.isDraggingSceneObject = true;
                editorState.draggingObjectIndex = hitObjectIndex;
                editorState.sceneDragOffsetX = sceneX - object.position[0];
                editorState.sceneDragOffsetY = sceneY - object.position[1];
            } else {
                StopSceneDrag(editorState);
            } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                editorState.isPanningSceneView = false;
            }
            break;

        case SDL_EVENT_MOUSE_WHEEL: {
            const ImVec2 mousePos = ImGui::GetIO().MousePos;
            if (!IsInsideSceneViewport(mousePos.x, mousePos.y, editorState) || event.wheel.y == 0.0f) {
                break;
            }

            float worldX = 0.0f;
            float worldY = 0.0f;
            if (!MapScreenToScenePoint(mousePos.x, mousePos.y, editorState, worldX, worldY)) {
                break;
            }
        } else if (!ImGui::GetIO().WantCaptureMouse) {
            editorState.selectedObjectIndex = -1;
            StopSceneDrag(editorState);
        }
        }
    }

    if (leftMousePressed &&
        editorState.isDraggingSceneObject &&
        editorState.draggingObjectIndex >= 0 &&
        editorState.draggingObjectIndex < static_cast<int>(sceneState.objects.size())) {
        float sceneX = 0.0f;
        float sceneY = 0.0f;
        if (MapScreenToScenePoint(static_cast<float>(mouseX), static_cast<float>(mouseY), editorState, sceneX, sceneY, true)) {
            GameObject& object = sceneState.objects[editorState.draggingObjectIndex];
            object.position[0] = sceneX - editorState.sceneDragOffsetX;
            object.position[1] = sceneY - editorState.sceneDragOffsetY;
        }
    }

    if (!leftMousePressed && m_PreviousLeftMousePressed) {
        StopSceneDrag(editorState);
    }
    m_PreviousLeftMousePressed = leftMousePressed;

    if (editorState.draggingObjectIndex >= static_cast<int>(sceneState.objects.size())) {
        StopSceneDrag(editorState);
    }

    if (editorState.draggingObjectIndex < 0) {
        editorState.isDraggingSceneObject = false;
    }
}

bool InputManager::shouldQuit() const {
    return m_QuitRequested;
}
