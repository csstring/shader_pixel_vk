#pragma once
#include "vk_types.hpp"

namespace vkutil{
  bool load_shader_module(const char* filePath,VkDevice device,VkShaderModule* outShaderModule);
}

class PipelineBuilder {
  
  public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
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

};

