#pragma once

#include <string>
#include <unordered_map>

#include "../core/Ref.h"
#include "Math.h"
#include "OrthographicCamera.h"
#include "SceneViewportImage.h"

struct GLFWwindow;
struct SceneState;
class ResourceManager;
class ShaderLibrary;
class Texture2D;

class Renderer2D {
public:
    bool init(GLFWwindow* window);
    void destroy();
    void clear();
    void resizeSceneRenderTarget(int width, int height);
    void renderScene(const SceneState& sceneState, ResourceManager& resourceManager, float deltaSeconds);
    SceneViewportImage getSceneViewportImage() const;
    void OnMouseScrolled(float xOffset, float yOffset);

    static void Init();
    static void Init(const Ref<ShaderLibrary>& shaderLibrary, const std::string& shaderName = "Renderer2D_Quad");
    static void Shutdown();
    static void BeginScene(const OrthographicCamera& camera);
    static void EndScene();

    static void DrawQuad(const Vector2& position, const Vector2& size, const Vector4& color);
    static void DrawQuad(const Vector3& position, const Vector2& size, const Vector4& color);
    static void DrawQuad(const Matrix4& transform, const Vector4& color);
    static void DrawQuad(const Transform& transform, const Vector4& color);
    static void DrawQuad(const Vector3& position, const Vector3& size, const Vector4& color);

    static void DrawQuad(const Vector2& position, const Vector2& size, const Ref<Texture2D>& texture);
    static void DrawQuad(const Vector3& position, const Vector2& size, const Ref<Texture2D>& texture);
    static void DrawQuad(const Matrix4& transform, const Ref<Texture2D>& texture);

    static void DrawQuad(
        const Vector2& position,
        const Vector2& size,
        const Ref<Texture2D>& texture,
        const Vector4& tintColor,
        float tilingFactor = 1.0f);
    static void DrawQuad(
        const Vector3& position,
        const Vector2& size,
        const Ref<Texture2D>& texture,
        const Vector4& tintColor,
        float tilingFactor = 1.0f);
    static void DrawQuad(
        const Matrix4& transform,
        const Ref<Texture2D>& texture,
        const Vector4& tintColor,
        float tilingFactor = 1.0f);
    static void DrawQuad(
        const Transform& transform,
        const Ref<Texture2D>& texture,
        const Vector4& tintColor,
        float tilingFactor = 1.0f);

private:
    void createSceneRenderTarget();
    void destroySceneRenderTarget();
    void updateCamera(float deltaSeconds);
    Ref<Texture2D> GetOrLoadTexture(const std::string& resolvedPath);

private:
    GLFWwindow* m_Window = nullptr;
    OrthographicCamera m_SceneCamera{ 0.0f, 1.0f, 1.0f, 0.0f };
    std::unordered_map<std::string, Ref<Texture2D>> m_TextureCache;
    unsigned int m_SceneFramebuffer = 0;
    unsigned int m_SceneColorAttachment = 0;
    unsigned int m_SceneDepthAttachment = 0;
    int m_SceneRenderTargetWidth = 1;
    int m_SceneRenderTargetHeight = 1;
    bool m_InstanceInitialized = false;
    bool m_CameraInitialized = false;
    float m_CameraPositionX = 0.0f;
    float m_CameraPositionY = 0.0f;
    float m_CameraRotation = 0.0f;
    float m_ZoomLevel = 1.0f;
    float m_CameraTranslationSpeed = 260.0f;
    float m_CameraRotationSpeed = 1.5f;
};
