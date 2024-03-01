#pragma once

#include "vk_types.hpp"

struct MeshAsset;

class IRenderable {

    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
};

struct Node : public IRenderable {

    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children;

    glm::mat4 localTransform;
    glm::mat4 worldTransform;

    void refreshTransform(const glm::mat4& parentMatrix);
    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx);
};

struct MeshNode : public Node {

	std::shared_ptr<MeshAsset> mesh;
  virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
};
