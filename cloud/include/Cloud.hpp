#pragma once

#include "vk_engine.hpp"
#include <glm/gtx/transform.hpp>
enum CLOUDTEXTUREID {
  CLOUDDENSITY,
  CLOUDLIGHT
};

class CloudScene
{
  private:
    VulkanEngine* _engine;

    DeletionQueue _deletionQueue;
    ComputeContext _computeContext;

    glm::vec4 rot;

  public:
    uint32_t dispatchSize = 4;
    uint32_t imageWidth = 128;
    uint32_t imageHeight = 128;
    uint32_t imageDepth = 128;
    VkDescriptorSetLayout cloudDensityLayout;
    VkDescriptorSetLayout cloudLightingLayout;

    VkDescriptorSet cloudDensityDescriptor;
    VkDescriptorSet cloudLightingDescriptor;

    VkSampler _defaultSamplerLinear;
    VkSampler _defualtSamplerNear;
    glm::vec3 modelTrans;
    glm::vec3  modelscale;
    uint32_t _curFrameIdx{0};
    AllocatedImage _cloudImageBuffer[2][2];
    CloudPushConstants constants;
    
    void initialize(VulkanEngine* engine);
    glm::mat4 getModelMatrix(){return glm::translate(modelTrans) * glm::rotate(glm::mat4(1.0f), rot.w, glm::vec3(rot)) * glm::scale(modelscale);};
    void loadSceneObject(VulkanEngine* engine);
    void drawSceneObject(VulkanEngine* engine);
    void update(float dt);
    void guiRender();
    void init_commands();
		void init_sync_structures();
    void init_image_buffer();
    void destroy(){_deletionQueue.flush();};
    CloudScene(){};
    ~CloudScene(){};
};