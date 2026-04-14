#include "OpenGLTexture.h"

#include <glad/glad.h>

#include <wincodec.h>

#include <stdexcept>
#include <string>
#include <vector>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

#ifndef _WIN32
#error OpenGLTexture2D currently uses WIC and is implemented for Windows builds only.
#endif

#include <objbase.h>
#include <stdexcept>

namespace {
std::wstring ToWideString(const std::string& value) {
    if (value.empty()) {
        return {};
    }

    const int count = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (count <= 0) {
        throw std::runtime_error("Failed to convert texture path to wide string.");
    }

    std::wstring result(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), count);
    result.pop_back();
    return result;
}

struct DecodedImage {
    unsigned int Width = 0;
    unsigned int Height = 0;
    std::vector<unsigned char> Pixels;
};

DecodedImage LoadImageWIC(const std::string& path) {
    const HRESULT initResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool shouldUninitialize = SUCCEEDED(initResult);

    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;

    auto releaseAll = [&]() {
        if (converter != nullptr) converter->Release();
        if (frame != nullptr) frame->Release();
        if (decoder != nullptr) decoder->Release();
        if (factory != nullptr) factory->Release();
        if (shouldUninitialize) CoUninitialize();
    };

    if (FAILED(CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory)))) {
        releaseAll();
        throw std::runtime_error("Failed to create WIC imaging factory.");
    }

    const std::wstring widePath = ToWideString(path);
    if (FAILED(factory->CreateDecoderFromFilename(
        widePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder))) {
        releaseAll();
        throw std::runtime_error("Failed to open texture file: " + path);
    }

    if (FAILED(decoder->GetFrame(0, &frame))) {
        releaseAll();
        throw std::runtime_error("Failed to decode texture frame: " + path);
    }

    if (FAILED(factory->CreateFormatConverter(&converter))) {
        releaseAll();
        throw std::runtime_error("Failed to create WIC format converter.");
    }

    if (FAILED(converter->Initialize(
        frame,
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom))) {
        releaseAll();
        throw std::runtime_error("Failed to convert texture to RGBA: " + path);
    }

    UINT width = 0;
    UINT height = 0;
    if (FAILED(converter->GetSize(&width, &height))) {
        releaseAll();
        throw std::runtime_error("Failed to read texture size: " + path);
    }

    DecodedImage image;
    image.Width = static_cast<unsigned int>(width);
    image.Height = static_cast<unsigned int>(height);
    image.Pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);

    const UINT stride = width * 4;
    const UINT bufferSize = stride * height;
    if (FAILED(converter->CopyPixels(nullptr, stride, bufferSize, image.Pixels.data()))) {
        releaseAll();
        throw std::runtime_error("Failed to copy texture pixels: " + path);
    }

    releaseAll();
    return image;
}
}

OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
    : m_Path(path) {
    const DecodedImage image = LoadImageWIC(path);
    m_Width = image.Width;
    m_Height = image.Height;

    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        static_cast<int>(m_Width),
        static_cast<int>(m_Height),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image.Pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
}

OpenGLTexture2D::OpenGLTexture2D(unsigned int width, unsigned int height, const void* data)
    : m_Width(width),
      m_Height(height) {
    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        static_cast<int>(m_Width),
        static_cast<int>(m_Height),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data);
}

OpenGLTexture2D::~OpenGLTexture2D() {
    glDeleteTextures(1, &m_RendererID);
}

unsigned int OpenGLTexture2D::GetWidth() const {
    return m_Width;
}

unsigned int OpenGLTexture2D::GetHeight() const {
    return m_Height;
}

void OpenGLTexture2D::Bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
}
