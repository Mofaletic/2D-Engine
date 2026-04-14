#pragma once

#include "../core/Ref.h"
#include "GraphicsAPI.h"
#include "Material.h"
#include "Math.h"

class OrthographicCamera;
class VertexArray;

class Renderer {
public:
    static void Init(GraphicsAPI api);
    static void Shutdown();
    static void BeginScene(const OrthographicCamera& camera);
    static void EndScene();
    static void OnWindowResize(unsigned int width, unsigned int height);
    static void Submit(
        const Ref<Material>& material,
        const Ref<VertexArray>& vertexArray,
        const Matrix4& transform
    );
    static void Submit(
        const Ref<Material>& material,
        const Ref<VertexArray>& vertexArray,
        const Transform& transform = Transform{}
    );
};
