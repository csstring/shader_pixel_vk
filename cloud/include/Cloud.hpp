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



    void makeLightTexture();
    void genCloud();

    void initRenderPipelines();
    void initGenCloudPipelines();
    void initMakeLightTexturePipelines();

    uint32_t dispatchSize = 4;
    uint32_t imageWidth = 128;
    uint32_t imageHeight = 128;
    uint32_t imageDepth = 128;

  public:
    VkSampler _defaultSamplerLinear;
    VkSampler _defualtSamplerNear;
    glm::vec3 modelTrans;
    glm::vec3  modelscale;
    uint32_t _curFrameIdx{0};
    AllocatedImage _cloudImageBuffer[2][2];
    CloudPushConstants constants;
    
    void initialize(VulkanEngine* engine);
    glm::mat4 getModelMatrix(){return glm::translate(modelTrans) * glm::scale(modelscale);};
    void uploadCubeMesh();
    void update(float dt);
    void draw(VkCommandBuffer cmd);
    void guiRender();
    void init_commands();
		void init_sync_structures();
    void init_image_buffer();
		void init_pipelines();
    void init_descriptors();//fix me

    CloudScene(){};
    ~CloudScene(){
      _deletionQueue.flush();
    };
};