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

constexpr unsigned int FRAME_OVERLAP = 2;
class CloudScene;

struct GLTFMetallic_Roughness {
	MaterialPipeline opaquePipeline;
	MaterialPipeline transparentPipeline;
  MaterialPipeline world_INskyBoxPipeline;
  MaterialPipeline world_OutSkyBoxPipeline;

  MaterialPipeline reflectPipeline;
  MaterialPipeline stencilFillPipeline;
  MaterialPipeline cloudPipeline;
	VkDescriptorSetLayout materialLayout;

	struct MaterialConstants {
		glm::vec4 colorFactors;
		glm::vec4 metal_rough_factors;
		//padding, we need it anyway for uniform buffers
		glm::vec4 extra[14];
	};

	struct MaterialResources {
		AllocatedImage colorImage;
		VkSampler colorSampler;
		AllocatedImage metalRoughImage;
		VkSampler metalRoughSampler;
    AllocatedImage skyBoxImage;
		VkSampler skyBoxSampler;
		VkBuffer dataBuffer;
		uint32_t dataBufferOffset;
	};

	void build_pipelines(VulkanEngine* engine);
  void buildWorldSkyBoxpipelines(VulkanEngine* engine);
  void buildstencilFillpipelines(VulkanEngine* engine);
  void build_cloudPipelines(VulkanEngine* engine);
  // void buildReflectpipelines(VulkanEngine* engine);
  
  DescriptorWriter writer;
	MaterialInstance write_material(VulkanEngine* engine, MaterialPass pass, MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
};

struct MeshNode : public Node {

	std::shared_ptr<MeshAsset> mesh;

	virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
};


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
    void draw_objects(VkCommandBuffer cmd,RenderObject* first, int count);
    void init_descriptors();
    void init_imgui();
    void update_scene();

  private:  
    CloudScene* cloudScene;

  public :
    void init();
    void cleanup();
    void draw();
    void run();
    void load_images();
	  void upload_mesh(Mesh& mesh);
    GPUMeshBuffers uploadMeshBuffers(std::vector<uint32_t> indices, std::vector<Vertex> vertices);
    FrameData& get_current_frame(){return _frames[_frameNumber % FRAME_OVERLAP];}
    Material* create_material(VkPipeline pipeline, VkPipelineLayout layout,
    const std::string& name, 
    uint32_t layoutCount = 0, VkDescriptorSet descriptorSet = VK_NULL_HANDLE,
    uint32_t constantSize = 0, void* constantPtr = nullptr
    );
    Material* get_material(const std::string& name);
    AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedImage createCubeImage(ktxTexture* ktxTexture, VkFormat format, VkSampler* sampler);
    Mesh* get_mesh(const std::string& name);
    size_t pad_uniform_buffer_size(size_t originalSize);
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

    VkImageView _depthImageView;
    VkFormat _depthFormat;
	  AllocatedImage _depthImage;

    std::vector<RenderObject> _renderables;
    std::unordered_map<std::string,Material> _materials;
    std::unordered_map<std::string,Mesh> _meshes;
    
    UploadContext _uploadContext;
    std::unordered_map<std::string, Texture> _loadedTextures;

    DescriptorAllocatorGrowable globalDescriptorAllocator;
  	std::vector<std::shared_ptr<MeshAsset>> testMeshes;
    DefualtPushConstants defualtConstants;
    GPUSceneData sceneData;
    VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;
    MaterialInstance defaultData;
    GLTFMetallic_Roughness metalRoughMaterial;

    AllocatedImage _whiteImage;
    AllocatedImage _blackImage;
    AllocatedImage _greyImage;
    AllocatedImage _errorCheckerboardImage;
    AllocatedImage _vulkanBoxImage;
    AllocatedImage _spaceBoxImage;
    AllocatedImage _skySphereImage;

    VkSampler _defaultSamplerLinear;
	  VkSampler _defaultSamplerNearest;
    VkSampler _vulkanBoxSamplerLinear;
    VkSampler _spaceBoxSamplerLinear;

    DrawContext mainDrawContext;
    std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;
    std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;

    Portal _portalManager;
    CloudScene* _cloud;
};