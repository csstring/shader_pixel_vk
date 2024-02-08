#pragma once

#include "vk_types.hpp"
#include <vector>
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

struct alignas(16) Vertex {

  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
  glm::vec3 color;
  
  static VertexInputDescription get_vertex_description();
};

struct Mesh {
	std::vector<Vertex> _vertices;
	AllocatedBuffer _vertexBuffer;
  bool load_from_obj(const char* filename);
};

struct GPUMeshBuffers {
    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
};
