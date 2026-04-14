#pragma once

#include <string>

#include "../core/Ref.h"
#include "RendererAPI.h"

class Texture {
public:
    virtual ~Texture() = default;

    virtual unsigned int GetWidth() const = 0;
    virtual unsigned int GetHeight() const = 0;
    virtual void Bind(unsigned int slot = 0) const = 0;
};

class Texture2D : public Texture {
public:
    static Ref<Texture2D> Create(const std::string& path);
    static Ref<Texture2D> Create(unsigned int width, unsigned int height, const void* data);
};
