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
    void renderCubeFace(uint32_t faceIndex, VkCommandBuffer commandBuffer,VulkanEngine* engine);

  public:
		VkSampler sampler;
    AllocatedImage envCubeImage;
    AllocatedImage depthImage;
    uint32_t offscreenImageSize;
    std::array<VkFramebuffer, 6> frameBuffers;
    VkRenderPass offscreenRenderPass;
    EnvOffscreenRender();
    ~EnvOffscreenRender();
    void initialize(VulkanEngine* engine);
};

