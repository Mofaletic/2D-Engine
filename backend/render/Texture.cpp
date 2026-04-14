#include "Texture.h"

#include "../platform/opengl/OpenGLTexture.h"

#include <stdexcept>

Ref<Texture2D> Texture2D::Create(const std::string& path) {
    switch (RendererAPI::GetAPI()) {
    case GraphicsAPI::OpenGL:
        return CreateRef<OpenGLTexture2D>(path);
    case GraphicsAPI::Vulkan:
    case GraphicsAPI::DX12:
    case GraphicsAPI::None:
    default:
        throw std::runtime_error("Texture2D backend is not implemented yet.");
    }
}

Ref<Texture2D> Texture2D::Create(unsigned int width, unsigned int height, const void* data) {
    switch (RendererAPI::GetAPI()) {
    case GraphicsAPI::OpenGL:
        return CreateRef<OpenGLTexture2D>(width, height, data);
    case GraphicsAPI::Vulkan:
    case GraphicsAPI::DX12:
    case GraphicsAPI::None:
    default:
        throw std::runtime_error("Texture2D backend is not implemented yet.");
    }
}
