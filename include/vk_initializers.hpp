#pragma once

#include "vk_types.hpp"
#include "vk_mesh.hpp"

namespace vkinit {
  inline VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0)
  {
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;

    info.queueFamilyIndex = queueFamilyIndex;
    info.flags = flags;
    return info;
  }
	
  inline VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY)
  {
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = nullptr;

    info.commandPool = pool;
    info.commandBufferCount = count;
    info.level = level;
    return info;
  }

  inline VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule)
  {
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;

    //shader stage
    info.stage = stage;
    //module containing the code for this shader stage
    info.module = shaderModule;
    //the entry point of the shader
    info.pName = "main";
    return info;
  }

  inline VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info(VertexInputDescription& vertexDescription)
  {	
    VkPipelineVertexInputStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.pNext = nullptr;

    //no vertex bindings or attributes
    info.vertexBindingDescriptionCount = vertexDescription.bindings.size();
    info.vertexAttributeDescriptionCount = vertexDescription.attributes.size();
    info.pVertexAttributeDescriptions = vertexDescription.attributes.data();
    info.pVertexBindingDescriptions = vertexDescription.bindings.data();
    return info;
  }


  inline VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(
    VkPrimitiveTopology topology,
	  VkPipelineInputAssemblyStateCreateFlags flags,
	  VkBool32 primitiveRestartEnable)
  {
    VkPipelineInputAssemblyStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.pNext = nullptr;
    info.topology = topology;
    info.flags = flags;
    info.primitiveRestartEnable = primitiveRestartEnable;
    return info;
  }


  inline VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(
      VkPolygonMode polygonMode,
			VkCullModeFlags cullMode,
			VkFrontFace frontFace,
			VkPipelineRasterizationStateCreateFlags flags = 0)
  {
    VkPipelineRasterizationStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.pNext = nullptr;

    info.depthClampEnable = VK_FALSE;
    info.polygonMode = polygonMode;
    info.lineWidth = 1.0f;
    info.cullMode = cullMode;
    info.frontFace = frontFace;
    info.flags = flags;
    return info;
  }

  inline VkPipelineMultisampleStateCreateInfo multisampling_state_create_info(
    VkSampleCountFlagBits rasterizationSamples,
		VkPipelineMultisampleStateCreateFlags flags = 0)
  {
    VkPipelineMultisampleStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.pNext = nullptr;

    //multisampling defaulted to no multisampling (1 sample per pixel)
    info.rasterizationSamples = rasterizationSamples;
    info.flags = flags;

    return info;
  }

  inline VkPipelineColorBlendAttachmentState color_blend_attachment_state(
    VkColorComponentFlags colorWriteMask,
		VkBool32 blendEnable)
  {
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = colorWriteMask;
    colorBlendAttachment.blendEnable = blendEnable;
    return colorBlendAttachment;
  }

  inline VkPipelineLayoutCreateInfo pipeline_layout_create_info()
  {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;

    //empty defaults
    info.flags = 0;
    info.setLayoutCount = 0;
    info.pSetLayouts = nullptr;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;
    return info;
  }

  inline VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0)
  {
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;
    fenceCreateInfo.flags = flags;
    return fenceCreateInfo;
  }

  inline VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0)
  {
    VkSemaphoreCreateInfo semCreateInfo = {};
    semCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semCreateInfo.pNext = nullptr;
    semCreateInfo.flags = flags;
    return semCreateInfo;
  }

  inline VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent)
  {
    VkImageCreateInfo info = { };
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;

    info.imageType = extent.depth == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;

    info.format = format;
    info.extent = extent;

    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usageFlags;

    return info;
  }

  inline VkImageViewCreateInfo imageview_create_info(VkFormat format, AllocatedImage image, VkImageAspectFlags aspectFlags)
  {
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;

    info.viewType = image._imageExtent.depth == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_3D;
    info.image = image._image;
    info.format = format;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = aspectFlags;

    return info;
  }

  inline VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(
    VkBool32 depthTestEnable,
		VkBool32 depthWriteEnable,
		VkCompareOp depthCompareOp)
  {
    VkPipelineDepthStencilStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.depthTestEnable = depthTestEnable;
    info.depthWriteEnable = depthWriteEnable;
    info.depthCompareOp = depthCompareOp;
    info.front = {};
    info.back = {.compareOp = VK_COMPARE_OP_ALWAYS};
    return info;
  }

  inline VkDescriptorSetLayoutBinding descriptorset_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding)
  {
    VkDescriptorSetLayoutBinding setbind = {};
    setbind.binding = binding;
    setbind.descriptorCount = 1;
    setbind.descriptorType = type;
    setbind.pImmutableSamplers = nullptr;
    setbind.stageFlags = stageFlags;

    return setbind;
  }

  inline VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo , uint32_t binding)
  {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;

    write.dstBinding = binding;
    write.dstSet = dstSet;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = bufferInfo;

    return write;
  }

  inline VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0)
  {
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext = nullptr;

    info.pInheritanceInfo = nullptr;
    info.flags = flags;
    return info;
  }

	inline VkSubmitInfo submit_info(VkCommandBuffer* cmd)
  {
    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.pNext = nullptr;

    info.waitSemaphoreCount = 0;
    info.pWaitSemaphores = nullptr;
    info.pWaitDstStageMask = nullptr;
    info.commandBufferCount = 1;
    info.pCommandBuffers = cmd;
    info.signalSemaphoreCount = 0;
    info.pSignalSemaphores = nullptr;

    return info;
  }

  inline VkSamplerCreateInfo sampler_create_info(VkFilter filters, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT)
  {
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.pNext = nullptr;

    info.magFilter = filters;
    info.minFilter = filters;
    info.addressModeU = samplerAddressMode;
    info.addressModeV = samplerAddressMode;
    info.addressModeW = samplerAddressMode;

    return info;
  }

  inline VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding)
  {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;

    write.dstBinding = binding;
    write.dstSet = dstSet;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = imageInfo;

    return write;
  }

  inline VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask)
  {
    VkImageSubresourceRange subImage {};
    subImage.aspectMask = aspectMask;
    subImage.baseMipLevel = 0;
    subImage.levelCount = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return subImage;
  }

  inline VkPipelineColorBlendAttachmentState enable_blending_additive()
  {
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    return colorBlendAttachment;
  }

  inline VkPipelineColorBlendAttachmentState enable_blending_alphablend()
  {
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    return colorBlendAttachment;
  }

  inline VkViewport viewport_create_info(VkExtent2D extent)
  {
    VkViewport info = {};
    info.x = 0.0f;
    info.y = 0.0f;
    info.width = (float)extent.width;
    info.height = (float)extent.height;
    info.minDepth = 0.0f;
    info.maxDepth = 1.0f;
    return info;
  }

  inline VkRect2D scissor_create_info(VkExtent2D extent)
  {
    VkRect2D info = {};
    info.offset = { 0, 0 };
    info.extent = extent;
    return info;
  }

}