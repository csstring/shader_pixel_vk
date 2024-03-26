#include "Cloud.hpp"
#include "vk_textures.hpp"
#include "vk_pipelines.hpp"
#include "vk_initializers.hpp"
#include "Camera.hpp"
#include "imgui.h"
//더블 이미지쓰지 말고 하나로 일단 해보자

void CloudScene::guiRender()
{
  ImGui::Begin("cloud controller");
	ImGui::SliderFloat("lightAbsorptionCoeff", &constants.lightAbsorptionCoeff, 0, 100);
	ImGui::SliderFloat("densityAbsorption", &constants.densityAbsorption, 0, 100);
	ImGui::SliderFloat("aniso", &constants.aniso, 0, 2);
	ImGui::SliderFloat3("lightDir", &constants.lightDir.x, -1, 1);
	ImGui::SliderFloat3("lightColor", &constants.lightColor.x, 0, 255);
	ImGui::SliderFloat3("uvwOffset", &constants.uvwOffset.x, 0, 100);
	ImGui::End();
}

void CloudScene::initialize(VulkanEngine* engine)
{
  _engine = engine;
	constants.lightAbsorptionCoeff = 20.0;
	constants.lightDir = glm::vec4(0,1,0,0);
	constants.densityAbsorption = 60.0;
	constants.lightColor = glm::vec4(1.0f) * 10.0f;
	constants.aniso = 0.3;
	// constants.uvwOffset = glm::vec4(0.f,0.f,0.f, 0.0f);
	constants.uvwOffset = glm::vec4(1.0f);
	constants.dt = 1.0f/ 720.0f;
	modelTrans = glm::vec3(0,40,-0);
	rot = glm::vec4(1,0,0, 3.14 / 2.0f);
	modelscale = glm::vec3(10, 10, 10);
	imageWidth = 128;
  imageHeight = 128;
  imageDepth = 128;
  init_commands();
  init_sync_structures();
  init_image_buffer();
  // initGenCloudPipelines();
  // initMakeLightTexturePipelines();

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

void CloudScene::uploadCubeMesh()
{
	Mesh cube{};
  float lineLength = 1.0f;

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
	constants.uvwOffset += glm::vec4(constants.dt/4.0f, 0.,constants.dt/4.0f,0.);
}

void CloudScene::draw(VkCommandBuffer cmd)
{
}

void CloudScene::loadSceneObject(VulkanEngine* engine)
{
  	auto cloudCube = loadGltf(engine,"./assets/models/", "cube.gltf", MaterialPass::Cloud, glm::scale(glm::vec3(0.2f)));//_cloud
	auto cloudDensity = loadComputeObj(engine, MaterialPass::CloudDensity); //_cloud
	auto cloudLighting = loadComputeObj(engine, MaterialPass::CloudLighting); // _cloud

	assert(cloudCube.has_value());
	assert(cloudDensity.has_value());
	assert(cloudLighting.has_value());

  	engine->loadedScenes["cloudCube"] = *cloudCube;
	engine->loadedComputeObj["cloudDensity"] = *cloudDensity;
	engine->loadedComputeObj["cloudLighting"] = *cloudLighting;
}