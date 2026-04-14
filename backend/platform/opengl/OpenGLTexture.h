#pragma once

#include "../../render/Texture.h"

#include <string>

class OpenGLTexture2D : public Texture2D {
public:
    explicit OpenGLTexture2D(const std::string& path);
    OpenGLTexture2D(unsigned int width, unsigned int height, const void* data);
    ~OpenGLTexture2D() override;

    unsigned int GetWidth() const override;
    unsigned int GetHeight() const override;
    void Bind(unsigned int slot = 0) const override;

private:
    unsigned int m_RendererID = 0;
    unsigned int m_Width = 0;
    unsigned int m_Height = 0;
    std::string m_Path;
};
