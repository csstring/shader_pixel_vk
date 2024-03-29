#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vma/vk_mem_alloc.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <iostream>

#define VK_CHECK(result) \
    if (result != VK_SUCCESS) { \
        std::cerr << "Vulkan error: " << result << std::endl; \
        exit(-1); \
    }

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct AllocatedImage {
    VkImage _image;
    VmaAllocation _allocation;
		VkImageView _imageView;
		VkExtent3D _imageExtent;
  	VkFormat _imageFormat;
};

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct DeletionQueue
{
  std::deque<std::function<void()>> deletors;

  void push_function(std::function<void()>&& functions){
    deletors.push_back(functions);
  }

  void flush(){
    for (auto it = deletors.rbegin(); it != deletors.rend(); ++it){
      (*it)();
    }
    deletors.clear();
  }
};

struct VertexInputDescription {

	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;

	VkPipelineVertexInputStateCreateFlags flags = 0;
};


struct Material {
	uint32_t constantSize{0};
	void* constant{VK_NULL_HANDLE};
	uint32_t setLayoutCount{0};
	VkDescriptorSet textureSet{VK_NULL_HANDLE};
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

enum class MaterialPass :uint8_t {
    MainColor,
    Transparent,
    World1_SkyBox,
    Cloud,
    Reflect,
    StencilFill_Zero,
    StencilFill_One,
    Offscreen,
    OffscreenBox,
    Julia,
    CloudDensity,
    CloudLighting,
    Other
};

enum class PortalState :uint8_t {
  Only_World0,
  Only_World1,
  In_World0,
  In_World1
};

struct MaterialPipeline {
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

struct MaterialInstance {
    MaterialPipeline* pipeline;
    VkDescriptorSet materialSet;
    MaterialPass passType;
};

struct RenderObject {
	uint32_t indexCount;
	uint32_t firstIndex;
	VkBuffer indexBuffer;
	VkBuffer vertexBuffer;

	MaterialInstance* material;

	glm::mat4 transform;
};

struct ComputeObject {
  MaterialInstance material;
  uint32_t dispatchX, dispatchY, dispatchZ;
};

struct alignas(16) GPUSceneData {
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; 
    glm::vec4 sunlightColor;// w for sun power
    glm::vec4 viewPos;
    glm::vec4 waterData;
    glm::vec4 cloudData;
};

struct ComputeContext {
	VkSemaphore _computeSemaphore;
	VkFence _computeFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

struct GPUObjectData{
	glm::mat4 modelMatrix;
	glm::vec4 modelColor;
};

struct UploadContext {
	VkFence _uploadFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

struct Texture {
	AllocatedImage image;
	VkImageView imageView;
	VkExtent3D imageExtent;
  VkFormat imageFormat;
};

struct alignas(16) CloudPushConstants {
	glm::mat4 worldMatrix;
  glm::mat4 view;
  glm::mat4 proj;
	glm::vec4 uvwOffset;
  glm::vec4 lightDir;
  glm::vec4 lightColor;
  float lightAbsorptionCoeff;
  float densityAbsorption;
  float aniso;
	float dt;
};

struct alignas(16) DefualtPushConstants {
	glm::vec4 lightPos;
	glm::vec4 camPos;
};

struct GPUDrawPushConstants {
  glm::mat4 worldMatrix;
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 normal;
};

class VulkanEngine;
class Scene
{
  public:
    virtual void initialize(VulkanEngine* engine) = 0;
    virtual void update(float dt, uint32_t frameidx) = 0;
    virtual void draw(VkCommandBuffer cmd) = 0;
    virtual void guiRender() = 0;
		virtual void init_sync_structures() = 0;
		virtual void init_descriptors() = 0;
		virtual void init_pipelines() = 0;
    virtual ~Scene(){};
};

struct DrawContext {
    std::vector<RenderObject> OpaqueSurfaces;
    std::vector<RenderObject> TransparentSurfaces;
    std::vector<RenderObject> EnvSurfaces;
    std::vector<ComputeObject*> computeObj;
};
