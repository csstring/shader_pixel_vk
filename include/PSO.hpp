#pragma once
#include "vk_types.hpp"

class VulkanEngine;
struct GLTFMetallic;
class CloudScene;

class PSO
{
  private:
    void build_pipelines(VulkanEngine* engine, GLTFMetallic* metallic);
    void buildWorldSkyBoxpipelines(VulkanEngine* engine, GLTFMetallic* metallic);
    void buildstencilFillpipelines(VulkanEngine* engine, GLTFMetallic* metallic);
    void build_cloudPipelines(VulkanEngine* engine, GLTFMetallic* metallic);
    void buildEnvOffscreenPipelines(VulkanEngine* engine, GLTFMetallic* metallic);
    void buildJuliapipelines(VulkanEngine* engine, GLTFMetallic* metallic);
    void buildCloudDensityPipelines(VulkanEngine* engine);
    void buildCloudLightingPipelines(VulkanEngine* engine);
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

    //compute shader
    MaterialPipeline cloudDensityPipeline;
    MaterialPipeline cloudLightingPipeline;
};