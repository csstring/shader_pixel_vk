#pragma once
#include "vk_types.hpp"

class VulkanEngine;
struct GLTFMetallic;
class CloudScene;
class Particle;
class PSO
{
  private:
    void build_pipelines(VulkanEngine* engine, GLTFMetallic* metallic);
    void buildWorldSkyBoxpipelines(VulkanEngine* engine, GLTFMetallic* metallic);
    void buildstencilFillpipelines(VulkanEngine* engine, GLTFMetallic* metallic);
    void build_cloudPipelines(VulkanEngine* engine, GLTFMetallic* metallic);
    void buildEnvOffscreenPipelines(VulkanEngine* engine, GLTFMetallic* metallic);
    void buildJuliapipelines(VulkanEngine* engine, GLTFMetallic* metallic);
    void buildCloudDensityPipelines(VulkanEngine* engine, CloudScene* cloud);
    void buildCloudLightingPipelines(VulkanEngine* engine, CloudScene* cloud);
    void buildParticleRenderPipeLines(VulkanEngine* engine, GLTFMetallic* metallic);
    void buildParticleUpdatePipelines(VulkanEngine* engine, Particle* particle);

  public:
    PSO(){};
    ~PSO(){};
    void buildPipeLine(VulkanEngine* engine);

    MaterialPipeline opaquePipeline;
    MaterialPipeline transparentPipeline;
    MaterialPipeline world_skyBoxPipeline;

    MaterialPipeline reflectPipeline;
    MaterialPipeline stencilFill_Zero_Pipeline;
    MaterialPipeline stencilFill_One_Pipeline;
    MaterialPipeline cloudPipeline;
    MaterialPipeline waterPipeline;
    MaterialPipeline envPipeline;
    MaterialPipeline juliaPipeline;
    MaterialPipeline particlePipeline;

    //compute shader
    MaterialPipeline cloudDensityPipeline;
    MaterialPipeline cloudLightingPipeline;
    MaterialPipeline particleCompPipeline;
};