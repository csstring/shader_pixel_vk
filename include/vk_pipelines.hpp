#pragma once
#include "vk_types.hpp"

namespace vkutil{
  bool load_shader_module(const char* filePath,VkDevice device,VkShaderModule* outShaderModule);
}

class PipelineBuilder {
  private:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    std::vector<VkShaderModule> _shaders;
    
  public:
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
    VkPipeline build_compute_pipeline(VkDevice device);
    void loadShader(std::string ShaderPath, VkDevice device, VkShaderStageFlagBits shaderStage);
    void shaderFlush(VkDevice device);
};

