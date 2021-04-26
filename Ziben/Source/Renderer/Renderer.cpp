#include "Renderer.hpp"

namespace Ziben {

    glm::mat4 Renderer::s_ViewProjectionMatrix = glm::mat4(1.0f);

    void Renderer::Init() {
        RenderCommand::Init();
    }

    void Renderer::BeginScene(Camera& camera) {
        s_ViewProjectionMatrix = camera.GetViewProjectionMatrix();
    }

    void Renderer::EndScene() {

    }

    void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform) {
        Shader::Bind(shader);
        shader->SetUniform("u_ViewProjectionMatrix", s_ViewProjectionMatrix);
        shader->SetUniform("u_Transform", transform);

        VertexArray::Bind(vertexArray);
        RenderCommand::DrawIndexed(vertexArray);
    }

} // namespace Ziben