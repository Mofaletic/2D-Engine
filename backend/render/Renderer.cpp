#include "Renderer.h"

#include "Material.h"
#include "OrthographicCamera.h"
#include "RenderCommand.h"
#include "VertexArray.h"
#include "../core/Instrumentor.h"

namespace {
struct SceneData {
    Matrix4 ViewProjectionMatrix = Matrix4::Identity();
};

SceneData s_SceneData;
}

void Renderer::Init(GraphicsAPI api) {
    RenderCommand::Init(api);
}

void Renderer::Shutdown() {
}

void Renderer::BeginScene(const OrthographicCamera& camera) {
    PROFILE_FUNCTION();
    s_SceneData.ViewProjectionMatrix = camera.GetViewProjectionMatrix();
}

void Renderer::EndScene() {
    PROFILE_FUNCTION();
}

void Renderer::OnWindowResize(unsigned int width, unsigned int height) {
    if (width == 0 || height == 0) {
        return;
    }

    RenderCommand::SetViewport(0, 0, width, height);
}

void Renderer::Submit(
    const Ref<Material>& material,
    const Ref<VertexArray>& vertexArray,
    const Matrix4& transform) {
    PROFILE_FUNCTION();
    material->SetMat4("u_ViewProjection", s_SceneData.ViewProjectionMatrix);
    material->SetMat4("u_Transform", transform);
    material->Bind();
    vertexArray->Bind();
    RenderCommand::DrawIndexed(*vertexArray);
}

void Renderer::Submit(
    const Ref<Material>& material,
    const Ref<VertexArray>& vertexArray,
    const Transform& transform) {
    Submit(material, vertexArray, transform.ToMatrix());
}
