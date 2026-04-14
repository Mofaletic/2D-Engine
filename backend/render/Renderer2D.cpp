#include "Renderer2D.h"

#include "Buffer.h"
#include "Material.h"
#include "Renderer.h"
#include "ShaderLibrary.h"
#include "Texture.h"
#include "VertexArray.h"
#include "../core/Instrumentor.h"
#include "../core/SceneState.h"
#include "../resource/ResourceManager.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stdexcept>

namespace {
struct Renderer2DStorage {
    Ref<VertexArray> QuadVertexArray;
    Ref<VertexBuffer> QuadVertexBuffer;
    Ref<IndexBuffer> QuadIndexBuffer;
    Ref<ShaderLibrary> ShaderLibraryInstance;
    Ref<Material> QuadMaterial;
    Ref<Texture2D> WhiteTexture;
    Matrix4 CameraViewProjection = Matrix4::Identity();
    bool Initialized = false;
};

Renderer2DStorage s_Renderer2DStorage;

std::filesystem::path GetRenderer2DShaderPath() {
    const std::filesystem::path projectRoot = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();

    const std::filesystem::path assetsPath = projectRoot / "assets" / "shaders" / "Renderer2D_Quad.glsl";
    if (std::filesystem::exists(assetsPath)) {
        return assetsPath;
    }

    const std::filesystem::path legacyAssetsPath = projectRoot / "asset" / "shaders" / "Renderer2D_Quad.glsl";
    if (std::filesystem::exists(legacyAssetsPath)) {
        return legacyAssetsPath;
    }

    throw std::runtime_error("Renderer2D shader asset was not found.");
}

Matrix4 CreateQuadTransform(const Vector3& position, const Vector2& size) {
    return Matrix4::Translation(position.x, position.y, position.z)
        * Matrix4::Scale(size.x, size.y, 1.0f);
}

void SubmitQuad(const Matrix4& transform, const Ref<Texture2D>& texture, const Vector4& tintColor, float tilingFactor) {
    PROFILE_SCOPE("Renderer2D::DrawQuadInternal");

    if (!s_Renderer2DStorage.Initialized || s_Renderer2DStorage.QuadMaterial == nullptr) {
        throw std::runtime_error("Renderer2D::Init must be called before DrawQuad.");
    }

    s_Renderer2DStorage.QuadMaterial->SetTexture(
        "u_Texture",
        texture != nullptr ? texture : s_Renderer2DStorage.WhiteTexture,
        0);
    s_Renderer2DStorage.QuadMaterial->SetFloat4("u_Color", tintColor);
    s_Renderer2DStorage.QuadMaterial->SetFloat("u_TilingFactor", tilingFactor);
    Renderer::Submit(s_Renderer2DStorage.QuadMaterial, s_Renderer2DStorage.QuadVertexArray, transform);
}
}

bool Renderer2D::init(GLFWwindow* window) {
    PROFILE_FUNCTION();

    m_Window = window;
    Renderer2D::Init();
    m_CameraInitialized = false;
    createSceneRenderTarget();
    m_InstanceInitialized = true;
    return true;
}

void Renderer2D::destroy() {
    PROFILE_FUNCTION();

    destroySceneRenderTarget();
    if (m_InstanceInitialized) {
        Renderer2D::Shutdown();
        m_TextureCache.clear();
        m_InstanceInitialized = false;
    }
}

void Renderer2D::clear() {
}

void Renderer2D::resizeSceneRenderTarget(int width, int height) {
    const int clampedWidth = std::max(width, 1);
    const int clampedHeight = std::max(height, 1);
    if (clampedWidth == m_SceneRenderTargetWidth && clampedHeight == m_SceneRenderTargetHeight) {
        return;
    }

    m_SceneRenderTargetWidth = clampedWidth;
    m_SceneRenderTargetHeight = clampedHeight;
    createSceneRenderTarget();
}

void Renderer2D::renderScene(const SceneState& sceneState, ResourceManager& resourceManager, float deltaSeconds) {
    PROFILE_FUNCTION();

    if (!m_InstanceInitialized || m_SceneFramebuffer == 0) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFramebuffer);
    glViewport(0, 0, m_SceneRenderTargetWidth, m_SceneRenderTargetHeight);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_CameraInitialized) {
        m_CameraPositionX = m_SceneRenderTargetWidth * 0.5f;
        m_CameraPositionY = m_SceneRenderTargetHeight * 0.5f;
        m_CameraInitialized = true;
    }

    updateCamera(deltaSeconds);

    const float halfWidth = (m_SceneRenderTargetWidth * 0.5f) * m_ZoomLevel;
    const float halfHeight = (m_SceneRenderTargetHeight * 0.5f) * m_ZoomLevel;
    m_SceneCamera.SetProjection(
        m_CameraPositionX - halfWidth,
        m_CameraPositionX + halfWidth,
        m_CameraPositionY - halfHeight,
        m_CameraPositionY + halfHeight);
    m_SceneCamera.SetPosition(m_CameraPositionX, m_CameraPositionY, 0.0f);
    m_SceneCamera.SetRotation(m_CameraRotation);

    BeginScene(m_SceneCamera);
    for (const GameObject& object : sceneState.objects) {
        const float width = 64.0f * std::max(object.scale[0], 0.0f);
        const float height = 64.0f * std::max(object.scale[1], 0.0f);

        Transform transform;
        transform.Translation = {
            object.position[0] + width * 0.5f,
            object.position[1] + height * 0.5f,
            0.0f
        };
        transform.Rotation = { 0.0f, 0.0f, object.rotation * 0.01745329252f };
        transform.Scale = { width, height, 1.0f };

        if (!object.texturePath.empty()) {
            const std::string resolvedPath = resourceManager.resolveTexturePath(object.texturePath);
            if (!resolvedPath.empty()) {
                DrawQuad(transform, GetOrLoadTexture(resolvedPath), { 1.0f, 1.0f, 1.0f, 1.0f });
                continue;
            }
        }

        DrawQuad(transform, { 0.85f, 0.3f, 0.2f, 1.0f });
    }
    EndScene();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

SceneViewportImage Renderer2D::getSceneViewportImage() const {
    SceneViewportImage image;
    image.Handle = reinterpret_cast<void*>(static_cast<intptr_t>(m_SceneColorAttachment));
    image.Width = static_cast<float>(m_SceneRenderTargetWidth);
    image.Height = static_cast<float>(m_SceneRenderTargetHeight);
    return image;
}

void Renderer2D::Init() {
    PROFILE_FUNCTION();

    auto shaderLibrary = CreateRef<ShaderLibrary>();
    shaderLibrary->Load("Renderer2D_Quad", GetRenderer2DShaderPath().string());
    Init(shaderLibrary);
}

void Renderer2D::Init(const Ref<ShaderLibrary>& shaderLibrary, const std::string& shaderName) {
    PROFILE_FUNCTION();

    if (s_Renderer2DStorage.Initialized) {
        return;
    }

    const float quadVertices[] = {
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f
    };

    const unsigned int quadIndices[] = { 0, 1, 2, 2, 3, 0 };
    const unsigned int whitePixel = 0xffffffff;

    s_Renderer2DStorage.QuadVertexArray = VertexArray::Create();
    s_Renderer2DStorage.QuadVertexBuffer = VertexBuffer::Create(quadVertices, sizeof(quadVertices));
    s_Renderer2DStorage.QuadVertexBuffer->SetLayout({
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float2, "a_TexCoord" }
    });
    s_Renderer2DStorage.QuadIndexBuffer = IndexBuffer::Create(quadIndices, 6);

    s_Renderer2DStorage.QuadVertexArray->AddVertexBuffer(s_Renderer2DStorage.QuadVertexBuffer);
    s_Renderer2DStorage.QuadVertexArray->SetIndexBuffer(s_Renderer2DStorage.QuadIndexBuffer);

    s_Renderer2DStorage.ShaderLibraryInstance = shaderLibrary;
    s_Renderer2DStorage.QuadMaterial = CreateRef<Material>(shaderLibrary->Get(shaderName));
    s_Renderer2DStorage.WhiteTexture = Texture2D::Create(1, 1, &whitePixel);
    s_Renderer2DStorage.QuadMaterial->SetTexture("u_Texture", s_Renderer2DStorage.WhiteTexture, 0);
    s_Renderer2DStorage.QuadMaterial->SetFloat4("u_Color", { 1.0f, 1.0f, 1.0f, 1.0f });
    s_Renderer2DStorage.QuadMaterial->SetFloat("u_TilingFactor", 1.0f);
    s_Renderer2DStorage.Initialized = true;
}

void Renderer2D::Shutdown() {
    PROFILE_FUNCTION();

    s_Renderer2DStorage.QuadMaterial.reset();
    s_Renderer2DStorage.ShaderLibraryInstance.reset();
    s_Renderer2DStorage.QuadIndexBuffer.reset();
    s_Renderer2DStorage.QuadVertexBuffer.reset();
    s_Renderer2DStorage.QuadVertexArray.reset();
    s_Renderer2DStorage.WhiteTexture.reset();
    s_Renderer2DStorage.CameraViewProjection = Matrix4::Identity();
    s_Renderer2DStorage.Initialized = false;
}

void Renderer2D::BeginScene(const OrthographicCamera& camera) {
    PROFILE_FUNCTION();

    s_Renderer2DStorage.CameraViewProjection = camera.GetViewProjectionMatrix();
    Renderer::BeginScene(camera);
}

void Renderer2D::EndScene() {
    PROFILE_FUNCTION();

    Renderer::EndScene();
}

void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Vector4& color) {
    DrawQuad(Vector3{ position.x, position.y, 0.0f }, size, color);
}

void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Vector4& color) {
    DrawQuad(CreateQuadTransform(position, size), color);
}

void Renderer2D::DrawQuad(const Matrix4& transform, const Vector4& color) {
    SubmitQuad(transform, s_Renderer2DStorage.WhiteTexture, color, 1.0f);
}

void Renderer2D::DrawQuad(const Transform& transform, const Vector4& color) {
    DrawQuad(transform.ToMatrix(), color);
}

void Renderer2D::DrawQuad(const Vector3& position, const Vector3& size, const Vector4& color) {
    DrawQuad(position, Vector2{ size.x, size.y }, color);
}

void Renderer2D::DrawQuad(const Vector2& position, const Vector2& size, const Ref<Texture2D>& texture) {
    DrawQuad(position, size, texture, { 1.0f, 1.0f, 1.0f, 1.0f });
}

void Renderer2D::DrawQuad(const Vector3& position, const Vector2& size, const Ref<Texture2D>& texture) {
    DrawQuad(position, size, texture, { 1.0f, 1.0f, 1.0f, 1.0f });
}

void Renderer2D::DrawQuad(const Matrix4& transform, const Ref<Texture2D>& texture) {
    DrawQuad(transform, texture, { 1.0f, 1.0f, 1.0f, 1.0f });
}

void Renderer2D::DrawQuad(
    const Vector2& position,
    const Vector2& size,
    const Ref<Texture2D>& texture,
    const Vector4& tintColor,
    float tilingFactor) {
    DrawQuad(Vector3{ position.x, position.y, 0.0f }, size, texture, tintColor, tilingFactor);
}

void Renderer2D::DrawQuad(
    const Vector3& position,
    const Vector2& size,
    const Ref<Texture2D>& texture,
    const Vector4& tintColor,
    float tilingFactor) {
    DrawQuad(CreateQuadTransform(position, size), texture, tintColor, tilingFactor);
}

void Renderer2D::DrawQuad(
    const Matrix4& transform,
    const Ref<Texture2D>& texture,
    const Vector4& tintColor,
    float tilingFactor) {
    SubmitQuad(transform, texture, tintColor, tilingFactor);
}

void Renderer2D::DrawQuad(
    const Transform& transform,
    const Ref<Texture2D>& texture,
    const Vector4& tintColor,
    float tilingFactor) {
    DrawQuad(transform.ToMatrix(), texture, tintColor, tilingFactor);
}

void Renderer2D::OnMouseScrolled(float xOffset, float yOffset) {
    static_cast<void>(xOffset);
    m_ZoomLevel -= yOffset * 0.1f;
    m_ZoomLevel = std::max(m_ZoomLevel, 0.25f);
}

void Renderer2D::createSceneRenderTarget() {
    destroySceneRenderTarget();

    glGenFramebuffers(1, &m_SceneFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFramebuffer);

    glGenTextures(1, &m_SceneColorAttachment);
    glBindTexture(GL_TEXTURE_2D, m_SceneColorAttachment);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        m_SceneRenderTargetWidth,
        m_SceneRenderTargetHeight,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SceneColorAttachment, 0);

    glGenRenderbuffers(1, &m_SceneDepthAttachment);
    glBindRenderbuffer(GL_RENDERBUFFER, m_SceneDepthAttachment);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_SceneRenderTargetWidth, m_SceneRenderTargetHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_SceneDepthAttachment);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        throw std::runtime_error("Scene framebuffer is incomplete.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer2D::destroySceneRenderTarget() {
    if (m_SceneDepthAttachment != 0) {
        glDeleteRenderbuffers(1, &m_SceneDepthAttachment);
        m_SceneDepthAttachment = 0;
    }

    if (m_SceneColorAttachment != 0) {
        glDeleteTextures(1, &m_SceneColorAttachment);
        m_SceneColorAttachment = 0;
    }

    if (m_SceneFramebuffer != 0) {
        glDeleteFramebuffers(1, &m_SceneFramebuffer);
        m_SceneFramebuffer = 0;
    }
}

void Renderer2D::updateCamera(float deltaSeconds) {
    if (m_Window == nullptr) {
        return;
    }

    const float moveSpeed = m_CameraTranslationSpeed * deltaSeconds;
    if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS) {
        m_CameraPositionX -= moveSpeed;
    }
    if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS) {
        m_CameraPositionX += moveSpeed;
    }
    if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS) {
        m_CameraPositionY += moveSpeed;
    }
    if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS) {
        m_CameraPositionY -= moveSpeed;
    }

    if (glfwGetKey(m_Window, GLFW_KEY_Q) == GLFW_PRESS) {
        m_CameraRotation += m_CameraRotationSpeed * deltaSeconds;
    }
    if (glfwGetKey(m_Window, GLFW_KEY_E) == GLFW_PRESS) {
        m_CameraRotation -= m_CameraRotationSpeed * deltaSeconds;
    }
}

Ref<Texture2D> Renderer2D::GetOrLoadTexture(const std::string& resolvedPath) {
    PROFILE_FUNCTION();

    const auto found = m_TextureCache.find(resolvedPath);
    if (found != m_TextureCache.end()) {
        return found->second;
    }

    auto texture = Texture2D::Create(resolvedPath);
    m_TextureCache.emplace(resolvedPath, texture);
    return texture;
}
