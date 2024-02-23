#include "Node.hpp"
#include "vk_loader.hpp"

void Node::refreshTransform(const glm::mat4& parentMatrix)
{
  worldTransform = parentMatrix * localTransform;
  for (auto c : children) {
    c->refreshTransform(worldTransform);
  }
}

void Node::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
  for (auto& c : children) {
    c->Draw(topMatrix, ctx);
  }
}

void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
    glm::mat4 nodeMatrix = topMatrix * worldTransform;

    for (auto& s : mesh->surfaces) {
      RenderObject def;
      def.indexCount = s.count;
      def.firstIndex = s.startIndex;
      def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
      def.vertexBuffer = mesh->meshBuffers.vertexBuffer.buffer;
      def.material = &s.material->data;

      def.transform = nodeMatrix;
      if (s.material->data.passType == MaterialPass::Offscreen)
        ctx.EnvSurfaces.push_back(def);
      else
        ctx.OpaqueSurfaces.push_back(def);
    }
    Node::Draw(topMatrix, ctx);
}