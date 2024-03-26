#pragma once
#include "vk_types.hpp"
#include <array>

class VulkanEngine;

class EnvOffscreenRender
{
  private:
    std::array<VkImageView, 6> envCubeMapFaceImageViews{};
    VkFormat offscreenImageFormat{ VK_FORMAT_R32G32B32A32_SFLOAT };
    VkFormat offscreenDepthFormat{ VK_FORMAT_D32_SFLOAT };
    DeletionQueue _deletionQueue;

    void createCubeMap(VulkanEngine* engine);
    void makeOffscreenRenderpass(VulkanEngine* engine);
    void makeOffscreenFramebuffer(VulkanEngine* engine);

  public:
		VkSampler sampler;
    AllocatedImage envCubeImage;
    AllocatedImage depthImage;
    uint32_t offscreenImageSize;
    std::array<VkFramebuffer, 6> frameBuffers;
    VkRenderPass offscreenRenderPass;
    void initialize(VulkanEngine* engine);
    void destroy(){_deletionQueue.flush();};
    void renderCubeFace(VkCommandBuffer cmd,VulkanEngine* engine, VkDescriptorSet globalDescriptor);
    void loadSceneObject(VulkanEngine* engine);
    void drawSceneObject(VulkanEngine* engine);
    ~EnvOffscreenRender(){};
    EnvOffscreenRender(){offscreenImageSize = 1024 / 8;};
};

