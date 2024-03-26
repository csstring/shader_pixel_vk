#pragma once
#include "vk_types.hpp"
#include "vk_descriptors.hpp"
#include "vk_mesh.hpp"
#include <glm/glm.hpp>
#include <unordered_map>
#include "vk_loader.hpp"  
#include "ktx.h"
#include "ktxvulkan.h"
#include "Portal.hpp"
#include "Cloud.hpp"
#include "EnvOffscreenRender.hpp"
#include "PSO.hpp"
#include "GLTFMetallic.hpp"

constexpr unsigned int FRAME_OVERLAP = 2;

struct FrameData {
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;	

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	AllocatedBuffer objectBuffer;
	VkDescriptorSet objectDescriptor;

	DeletionQueue _deletionQueue;
	DescriptorAllocatorGrowable _frameDescriptors;
};

class VulkanEngine
{
  private:
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_default_renderpass();
	  void init_framebuffers();
    void init_sync_structures();
    void init_pipelines();
    void init_scene();
    void load_meshes();
    void draw_objects(VkCommandBuffer cmd, uint32_t swapchainImageIndex);
    void computeShaderCalc(VkCommandBuffer cmd);
    void init_descriptors();
    void init_imgui();
    void update_scene();
    void loadSceneObject();
    void GUIRender();
    void OpaqueRender(VkCommandBuffer cmd,VkRenderPassBeginInfo rpInfo, GPUDrawPushConstants& pushConstants, VkDescriptorSet globalDescriptor);
    void TransparentRender(VkCommandBuffer cmd,VkRenderPassBeginInfo rpInfo, GPUDrawPushConstants& pushConstants, VkDescriptorSet globalDescriptor);
  
  private:  
    
  public :
    void init();
    void cleanup();
    void draw();
    void run();
    GPUMeshBuffers uploadMeshBuffers(std::vector<uint32_t> indices, std::vector<Vertex> vertices);
    FrameData& get_current_frame(){return _frames[_frameNumber % FRAME_OVERLAP];}
    AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedImage createCubeImage(ktxTexture* ktxTexture, VkFormat format, VkSampler* sampler);
    
    AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);
    void destroy_buffer(const AllocatedBuffer& buffer);
    void destroy_image(const AllocatedImage& img);

  public :
    bool _isInitialized{ false };
	  int _frameNumber {0};
	  VkExtent2D _windowExtent{ 1920 , 1080 };
    struct SDL_Window* _window{ nullptr };

    DeletionQueue _mainDeletionQueue;
    VkInstance _instance; // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
    VkPhysicalDevice _chosenGPU; // GPU chosen as the default device
    VkDevice _device; // Vulkan device for commands
    VkSurfaceKHR _surface; // Vulkan window surface
    VmaAllocator _allocator;
    VkPhysicalDeviceProperties _gpuProperties;

    FrameData _frames[FRAME_OVERLAP];

    VkSwapchainKHR _swapchain;
    VkFormat _swapchainImageFormat;
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;

    VkQueue _graphicsQueue; //queue we will submit to
    uint32_t _graphicsQueueFamily; //family of that queue
    
    VkRenderPass _renderPass;
	  std::vector<VkFramebuffer> _framebuffers;

	  AllocatedImage _depthImage;

    std::unordered_map<std::string,Material> _materials;
    std::unordered_map<std::string,Mesh> _meshes;
    
    UploadContext _uploadContext;

    DescriptorAllocatorGrowable globalDescriptorAllocator;
    GPUSceneData sceneData;
    VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;
    GLTFMetallic metalRoughMaterial;

    AllocatedImage _errorCheckerboardImage;
    AllocatedImage _vulkanBoxImage;

    VkSampler _defaultSamplerLinear;
    VkSampler _vulkanBoxSamplerLinear;

    DrawContext mainDrawContext;
    std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;
    std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;
    std::unordered_map<std::string, std::shared_ptr<ComputeObject>> loadedComputeObj;
    std::unique_ptr<Portal> _portalManager;
    std::unique_ptr<CloudScene> _cloud;
    std::unique_ptr<EnvOffscreenRender> envRender;
    PSO _PSO;
};