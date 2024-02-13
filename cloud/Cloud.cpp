#include "Cloud.hpp"
#include "vk_textures.hpp"
#include "vk_pipelines.hpp"
#include "vk_initializers.hpp"
#include "Camera.hpp"
#include "imgui.h"
//더블 이미지쓰지 말고 하나로 일단 해보자

void CloudScene::guiRender()
{
  ImGui::Begin("Scene controller");
	ImGui::SliderFloat("lightAbsorptionCoeff", &constants.lightAbsorptionCoeff, 0, 100);
	ImGui::SliderFloat("densityAbsorption", &constants.densityAbsorption, 0, 100);
	ImGui::SliderFloat("aniso", &constants.aniso, 0, 2);
	ImGui::SliderFloat3("lightDir", &constants.lightDir.x, -1, 1);
	ImGui::SliderFloat3("lightColor", &constants.lightColor.x, 0, 255);
	ImGui::SliderFloat3("uvwOffset", &constants.uvwOffset.x, 0, 100);
	ImGui::SliderFloat3("model translate", &modelTrans.x, -400, 400);
	ImGui::SliderFloat3("model scale", &modelscale.x, 0, 500);
	ImGui::End();
}

void CloudScene::initialize(VulkanEngine* engine)
{
  _engine = engine;
	constants.lightAbsorptionCoeff = 10.0;
	constants.lightDir = glm::vec4(0,1,0,0);
	constants.densityAbsorption = 10.0;
	constants.lightColor = glm::vec4(1.0f) * 40.0f;
	constants.aniso = 0.3;
	// constants.uvwOffset = glm::vec4(0.f,0.f,0.f, 0.0f);
	constants.uvwOffset = glm::vec4(1.0f);
	constants.dt = 1.0f/ 120.0f;
	modelTrans = glm::vec3(0,-0,0);
	// modelscale = 25.0f;
	modelscale = glm::vec3(10.0f);
	imageWidth = 128*1;
  imageHeight = 128*1;
  imageDepth = 128*1;
  init_commands();
  init_sync_structures();
  init_image_buffer();
  initGenCloudPipelines();
  initMakeLightTexturePipelines();

}

void CloudScene::initRenderPipelines()
{
	// VkDescriptorSetLayout descriptorSetLayout;
  // VkDescriptorSet descriptorSetFirst, descriptorSetSecond;
	// //init descriptor
	// {
	// 	DescriptorLayoutBuilder builder;
  //   builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	// 	builder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  //   descriptorSetLayout = builder.build(_engine->_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	// }
	// descriptorSetFirst = _engine->globalDescriptorAllocator.allocate(_engine->_device, descriptorSetLayout);
	// descriptorSetSecond = _engine->globalDescriptorAllocator.allocate(_engine->_device, descriptorSetLayout);
		
	// {
  //   DescriptorWriter writer;	
	// 	writer.write_image(0, _cloudImageBuffer[0][CLOUDTEXTUREID::CLOUDDENSITY]._imageView, _defaultSamplerLinear , VK_IMAGE_LAYOUT_GENERAL,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	// 	writer.write_image(1, _cloudImageBuffer[0][CLOUDTEXTUREID::CLOUDLIGHT]._imageView, _defaultSamplerLinear , VK_IMAGE_LAYOUT_GENERAL,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  //   writer.update_set(_engine->_device, descriptorSetFirst);
  // }

	// {
  //   DescriptorWriter writer;	
	// 	writer.write_image(0, _cloudImageBuffer[1][CLOUDTEXTUREID::CLOUDDENSITY]._imageView, _defaultSamplerLinear , VK_IMAGE_LAYOUT_GENERAL,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	// 	writer.write_image(1, _cloudImageBuffer[1][CLOUDTEXTUREID::CLOUDLIGHT]._imageView, _defaultSamplerLinear , VK_IMAGE_LAYOUT_GENERAL,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  //   writer.update_set(_engine->_device, descriptorSetSecond);
  // }
	// _deletionQueue.push_function([=]() {
  //       vkDestroyDescriptorSetLayout(_engine->_device, descriptorSetLayout, nullptr);
  // });
	// //init pipeline
	// {
	// PipelineBuilder pipelineBuilder;
  // pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
  // pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
	// //input assembly is the configuration for drawing triangle lists, strips, or individual points.
	// //we are just going to draw triangle list
	// pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	// //build viewport and scissor from the swapchain extents
	// pipelineBuilder._viewport.x = 0.0f;
	// pipelineBuilder._viewport.y = 0.0f;
	// pipelineBuilder._viewport.width = (float)_engine->_windowExtent.width;
	// pipelineBuilder._viewport.height = (float)_engine->_windowExtent.height;
	// pipelineBuilder._viewport.minDepth = 0.0f;
	// pipelineBuilder._viewport.maxDepth = 1.0f;

	// pipelineBuilder._scissor.offset = { 0, 0 };
	// pipelineBuilder._scissor.extent = _engine->_windowExtent;

	// pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
	// pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
	// pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();

  // VkPipelineLayoutCreateInfo textured_pipeline_layout_info = vkinit::pipeline_layout_create_info();

	// VkDescriptorSetLayout texturedSetLayouts[] = {_engine->_gpuSceneDataDescriptorLayout, descriptorSetLayout };//fix me
  // VkPushConstantRange push_constant;
	// push_constant.offset = 0;
	// push_constant.size = sizeof(CloudPushConstants);
	// push_constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
	// textured_pipeline_layout_info.pPushConstantRanges = &push_constant;
	// textured_pipeline_layout_info.pushConstantRangeCount = 1;

	// textured_pipeline_layout_info.setLayoutCount = 3;//fix me
	// textured_pipeline_layout_info.pSetLayouts = texturedSetLayouts;
	// VkPipelineLayout texturedPipeLayout;
	// VK_CHECK(vkCreatePipelineLayout(_engine->_device, &textured_pipeline_layout_info, nullptr, &texturedPipeLayout));
	
	// VertexInputDescription vertexDescription = Vertex::get_vertex_description();
	// pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	// pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

	// pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	// pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

  // VkShaderModule fragmentShader;
  // VkShaderModule vertShader;

	// if (!vkutil::load_shader_module("./spv/cloud.vert.spv", _engine->_device, &vertShader)){
	// 	std::cout << "Error when building the triangle vertex shader module" << std::endl;
	// }
	// else {
	// 	std::cout << "Red Triangle vertex shader successfully loaded" << std::endl;
	// }

	// if (!vkutil::load_shader_module("./spv/cloud.frag.spv", _engine->_device, &fragmentShader))
	// {
	// 	std::cout << "Error when building the default_lit fragment shader module" << std::endl;
	// }
	// else {
	// 	std::cout << "Red Triangle default_lit shader successfully loaded" << std::endl;
	// }

	// pipelineBuilder._shaderStages.push_back(
	// 	vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertShader));
	// pipelineBuilder._shaderStages.push_back(
	// 	vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
	// pipelineBuilder._pipelineLayout = texturedPipeLayout;
	// VkPipeline texPipeline = pipelineBuilder.build_pipeline(_engine->_device, _engine->_renderPass);
  // //일단 한개만 
	// _engine->create_material(texPipeline, texturedPipeLayout, "cloudRenderPipe", 1, descriptorSetFirst, 
	// sizeof(CloudPushConstants), (void*)&constants);

	// vkDestroyShaderModule(_engine->_device, fragmentShader, nullptr);
  // vkDestroyShaderModule(_engine->_device, vertShader, nullptr);

 	// _deletionQueue.push_function([=]() {
	// 	vkDestroyPipeline(_engine->_device, texPipeline, nullptr);
	// 	vkDestroyPipelineLayout(_engine->_device, texturedPipeLayout, nullptr);
  // });
	// }
}

void CloudScene::initGenCloudPipelines()
{
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	{
		DescriptorLayoutBuilder builder;
    builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    descriptorSetLayout = builder.build(_engine->_device, VK_SHADER_STAGE_COMPUTE_BIT);
	}
	descriptorSet = _engine->globalDescriptorAllocator.allocate(_engine->_device, descriptorSetLayout);
	{
    DescriptorWriter writer;	
		writer.write_image(0, _cloudImageBuffer[0][CLOUDTEXTUREID::CLOUDDENSITY]._imageView, nullptr , VK_IMAGE_LAYOUT_GENERAL,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.update_set(_engine->_device, descriptorSet);
  }
	_deletionQueue.push_function([=]() {
        vkDestroyDescriptorSetLayout(_engine->_device, descriptorSetLayout, nullptr);
  });
	{
	VkShaderModule compShader;
	if (!vkutil::load_shader_module("./spv/cloudDensity.comp.spv", _engine->_device, &compShader)){
		std::cout << "Error when building the cloudDensity comp shader module" << std::endl;
	}
	VkPipeline cloudDensityPipeline;
	VkPipelineLayout cloudDensityPipeLayout;

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShader;
	computeShaderStageInfo.pName = "main";

	VkDescriptorSetLayout SetLayouts[] = { descriptorSetLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = SetLayouts;

	VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = sizeof(CloudPushConstants);
	push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineLayoutInfo.pPushConstantRanges = &push_constant;
	pipelineLayoutInfo.pushConstantRangeCount = 1;

	if (vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &cloudDensityPipeLayout) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = cloudDensityPipeLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &cloudDensityPipeline) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline!");
	}
	_engine->create_material(cloudDensityPipeline, cloudDensityPipeLayout, "MAKECLOUD", pipelineLayoutInfo.setLayoutCount, descriptorSet);
	vkDestroyShaderModule(_engine->_device, compShader, nullptr);
	_deletionQueue.push_function([=]() {
    vkDestroyPipeline(_engine->_device, cloudDensityPipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, cloudDensityPipeLayout, nullptr);
  });
	}
}

void CloudScene::initMakeLightTexturePipelines()
{
  VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	{
		DescriptorLayoutBuilder builder;
    builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    descriptorSetLayout = builder.build(_engine->_device, VK_SHADER_STAGE_COMPUTE_BIT);
	}
	descriptorSet = _engine->globalDescriptorAllocator.allocate(_engine->_device, descriptorSetLayout);
	{
    DescriptorWriter writer;	
		writer.write_image(0, _cloudImageBuffer[0][CLOUDTEXTUREID::CLOUDDENSITY]._imageView, _defaultSamplerLinear , VK_IMAGE_LAYOUT_GENERAL,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.write_image(1, _cloudImageBuffer[0][CLOUDTEXTUREID::CLOUDLIGHT]._imageView, nullptr , VK_IMAGE_LAYOUT_GENERAL,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.update_set(_engine->_device, descriptorSet);
  }
	_deletionQueue.push_function([=]() {
        vkDestroyDescriptorSetLayout(_engine->_device, descriptorSetLayout, nullptr);
  });
  {
  VkShaderModule compShader;
	if (!vkutil::load_shader_module("./spv/cloudLighting.comp.spv", _engine->_device, &compShader)){
		std::cout << "Error when building the cloudLighting comp shader module" << std::endl;
	}
	VkPipeline cloudLightingPipeline;
	VkPipelineLayout cloudLightingPipeLayout;

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = compShader;
	computeShaderStageInfo.pName = "main";

	VkDescriptorSetLayout SetLayouts[] = { descriptorSetLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = SetLayouts;

	VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = sizeof(CloudPushConstants);
	push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineLayoutInfo.pPushConstantRanges = &push_constant;
	pipelineLayoutInfo.pushConstantRangeCount = 1;

	if (vkCreatePipelineLayout(_engine->_device, &pipelineLayoutInfo, nullptr, &cloudLightingPipeLayout) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline layout!");
	}

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = cloudLightingPipeLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(_engine->_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &cloudLightingPipeline) != VK_SUCCESS) {
	    throw std::runtime_error("failed to create compute pipeline!");
	}
	_engine->create_material(cloudLightingPipeline, cloudLightingPipeLayout, "MAKELIGHTTEXTURE", pipelineLayoutInfo.setLayoutCount, descriptorSet);
	vkDestroyShaderModule(_engine->_device, compShader, nullptr);
	_deletionQueue.push_function([=]() {
    vkDestroyPipeline(_engine->_device, cloudLightingPipeline, nullptr);
		vkDestroyPipelineLayout(_engine->_device, cloudLightingPipeLayout, nullptr);
  });
	}
}

void CloudScene::init_pipelines()
{
}

void CloudScene::uploadCubeMesh()
{
	Mesh cube{};
  float lineLength = 1.0f;
  // cube._vertices = {
  //   // Front face
  //   {{-lineLength, -lineLength, lineLength}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
  //   {{lineLength, -lineLength, lineLength}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
  //   {{lineLength, lineLength, lineLength}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
  //   {{lineLength, lineLength, lineLength}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
  //   {{-lineLength, lineLength, lineLength}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
  //   {{-lineLength, -lineLength, lineLength}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},

  //   // Back face
  //   {{-lineLength, -lineLength, -lineLength}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
  //   {{lineLength, -lineLength, -lineLength}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
  //   {{lineLength, lineLength, -lineLength}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
  //   {{lineLength, lineLength, -lineLength}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
  //   {{-lineLength, lineLength, -lineLength}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
  //   {{-lineLength, -lineLength, -lineLength}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

  //   // Left face
  //   {{-lineLength, -lineLength, -lineLength}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
  //   {{-lineLength, lineLength, lineLength}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
  //   {{-lineLength, lineLength, -lineLength}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
  //   {{-lineLength, lineLength, lineLength}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
  //   {{-lineLength, -lineLength, -lineLength}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
  //   {{-lineLength, -lineLength, lineLength}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

  //   // Right face
  //   {{lineLength, -lineLength, -lineLength}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
  //   {{lineLength, lineLength, -lineLength}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
  //   {{lineLength, lineLength, lineLength}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
  //   {{lineLength, lineLength, lineLength}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
  //   {{lineLength, -lineLength, lineLength}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
  //   {{lineLength, -lineLength, -lineLength}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

  //   // Top face
  //   {{-lineLength, lineLength, -lineLength}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
  //   {{-lineLength, lineLength, lineLength}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
  //   {{lineLength, lineLength, lineLength}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
  //   {{lineLength, lineLength, lineLength}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
  //   {{lineLength, lineLength, -lineLength}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
  //   {{-lineLength, lineLength, -lineLength}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

  //   // Bottom face
  //   {{-lineLength, -lineLength, -lineLength}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
  //   {{lineLength, -lineLength, -lineLength}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
  //   {{lineLength, -lineLength, lineLength}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
  //   {{lineLength, -lineLength, lineLength}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
  //   {{-lineLength, -lineLength, lineLength}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
  //   {{-lineLength, -lineLength, -lineLength}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}
  // };
  _engine->upload_mesh(cube);
	_engine->_meshes["cube_cloud"] = cube;
}

void CloudScene::init_commands()
{
	VkCommandPoolCreateInfo computePoolInfo = vkinit::command_pool_create_info(_engine->_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  VK_CHECK(vkCreateCommandPool(_engine->_device, &computePoolInfo, nullptr, &_computeContext._commandPool));
  VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_computeContext._commandPool, 1);
  VK_CHECK(vkAllocateCommandBuffers(_engine->_device, &cmdAllocInfo, &_computeContext._commandBuffer));
  _deletionQueue.push_function([=]() {
  	vkDestroyCommandPool(_engine->_device, _computeContext._commandPool, nullptr);
  });
}

void CloudScene::init_sync_structures()
{
  VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();  
  VK_CHECK(vkCreateFence(_engine->_device, &fenceCreateInfo, nullptr, &_computeContext._computeFence));
  VK_CHECK(vkCreateSemaphore(_engine->_device, &semaphoreCreateInfo, nullptr, &_computeContext._computeSemaphore));
  _deletionQueue.push_function([=]() {
    vkDestroyFence(_engine->_device, _computeContext._computeFence, nullptr);
    vkDestroySemaphore(_engine->_device, _computeContext._computeSemaphore, nullptr);
  });
}

void CloudScene::init_image_buffer()
{
	for (int i = 0; i < 2; ++i){
		_cloudImageBuffer[0][i] = _engine->create_image(VkExtent3D{ imageWidth, imageHeight, imageDepth }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT, false);
		_cloudImageBuffer[1][i] = _engine->create_image(VkExtent3D{ imageWidth, imageHeight, imageDepth }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT, false);
		_engine->immediate_submit([=](VkCommandBuffer cmd){
			vkutil::transitionImageLayout(cmd, _cloudImageBuffer[0][i]._image,VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT));
			vkutil::transitionImageLayout(cmd, _cloudImageBuffer[1][i]._image,VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT));
		});
	}

	VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;
	VK_CHECK(vkCreateSampler(_engine->_device, &sampl, nullptr, &_defaultSamplerLinear));
	
	sampl.magFilter = VK_FILTER_NEAREST;
	sampl.minFilter = VK_FILTER_NEAREST;
	vkCreateSampler(_engine->_device, &sampl, nullptr, &_defualtSamplerNear);

	
	_deletionQueue.push_function([=]() {
		vkDestroySampler(_engine->_device, _defaultSamplerLinear, nullptr);
		vkDestroySampler(_engine->_device, _defualtSamplerNear, nullptr);

		for (int i =0; i < 2;++i){
			vkDestroyImageView(_engine->_device, _cloudImageBuffer[0][i]._imageView, nullptr);
			vkDestroyImageView(_engine->_device, _cloudImageBuffer[1][i]._imageView, nullptr);
			vmaDestroyImage(_engine->_allocator, _cloudImageBuffer[0][i]._image, _cloudImageBuffer[0][i]._allocation);
			vmaDestroyImage(_engine->_allocator, _cloudImageBuffer[1][i]._image, _cloudImageBuffer[1][i]._allocation);
		}
	});
}

void CloudScene::init_descriptors(){}

void CloudScene::makeLightTexture()
{
  Material* computMaterial = _engine->get_material("MAKELIGHTTEXTURE");
	VkCommandBuffer cmd = _engine->get_current_frame()._mainCommandBuffer;
  vkCmdPushConstants(cmd, computMaterial->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CloudPushConstants), &constants);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computMaterial->pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computMaterial->pipelineLayout, 0, 1, &computMaterial->textureSet, 0, nullptr);
  vkCmdDispatch(cmd, imageWidth/dispatchSize, imageHeight/dispatchSize, imageDepth / dispatchSize);
}

void CloudScene::genCloud()
{
  Material* computMaterial = _engine->get_material("MAKECLOUD");
	VkCommandBuffer cmd = _engine->get_current_frame()._mainCommandBuffer;

  vkCmdPushConstants(cmd, computMaterial->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CloudPushConstants), &constants);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computMaterial->pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computMaterial->pipelineLayout, 0, 1, &computMaterial->textureSet, 0, nullptr);
  vkCmdDispatch(cmd, imageWidth/dispatchSize, imageHeight/dispatchSize, imageDepth / dispatchSize);
}

void CloudScene::update(float dt)
{
	Camera cam = Camera::getInstance();
	static int color = 0;
	constants.uvwOffset += glm::vec4(constants.dt/4.0f, 0.,constants.dt/4.0f,0.);
	if (color++ == 0){
	}
  genCloud();
	makeLightTexture();
}

void CloudScene::draw(VkCommandBuffer cmd)
{
}
/*
User
You
AllocatedImage VulkanEngine::create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	size_t data_size = size.depth * size.width * size.height * 4;
	AllocatedBuffer uploadbuffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	AllocatedBuffer readbackBuffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);

	memcpy(uploadbuffer.info.pMappedData, data, data_size);
	
	AllocatedImage new_image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

	immediate_submit([&](VkCommandBuffer cmd) {
		vkutil::transitionImageLayout(cmd, new_image._image, new_image._imageFormat,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = size;
		copyRegion.imageOffset = {0, 0, 0};
		// copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, new_image._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			&copyRegion);

		vkutil::transitionImageLayout(cmd, new_image._image,new_image._imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			vkutil::transitionImageLayout(cmd, new_image._image, new_image._imageFormat,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0; // Tightly packed
    region.bufferImageHeight = 0; // Tightly packed
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = size;

    vkCmdCopyImageToBuffer(cmd, new_image._image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readbackBuffer.buffer, 1, &region);

    // Transition back if necessary
    vkutil::transitionImageLayout(cmd, new_image._image, new_image._imageFormat,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		});
	std::cerr << *(uint32_t*)uploadbuffer.info.pMappedData << std::endl;
	destroy_buffer(uploadbuffer);
	void* mappedData;
	vmaMapMemory(_allocator, readbackBuffer.allocation, &mappedData);
	// Assuming the image format is VK_FORMAT_R8G8B8A8_UNORM and you're reading the entire image
	uint32_t* pixels = static_cast<uint32_t*>(mappedData);
	for (size_t i = 0; i < size.width * size.height; ++i) {
			std::cout << std::hex << pixels[i] << " ";
			if ((i + 1) % size.width == 0) std::cout << std::endl; // New line per image row
	}
	vmaUnmapMemory(_allocator, readbackBuffer.allocation);
	destroy_buffer(readbackBuffer);
	return new_image;
}*/