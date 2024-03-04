#pragma once

#include "vk_engine.hpp"

class Particle
{
  private:
    VulkanEngine* _engine;
    uint32_t _curFrameIdx{0};

  public:
    uint32_t PARTICLE_COUNT;
    DeletionQueue _deletionQueue;
    ParticlePushConstants constant;
    AllocatedBuffer _particleBuffer;
    VkDescriptorSet particleDescriptor;
    VkDescriptorSetLayout _particleSetLayout;

    void initialize(VulkanEngine* engine);
    void load_particle();
    void update(float dt, uint32_t frameidx);
    void draw(VkCommandBuffer cmd);
    void guiRender();
	void init_descriptors();
    Particle(){};
    ~Particle(){
      _deletionQueue.flush();
    };
};