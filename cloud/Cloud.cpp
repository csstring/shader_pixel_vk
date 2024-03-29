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
			vkutil::setImageLayout(cmd, _cloudImageBuffer[0][i]._image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL);
			vkutil::setImageLayout(cmd, _cloudImageBuffer[1][i]._image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL);
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

void CloudScene::update(float dt)
{
	constants.uvwOffset += glm::vec4(constants.dt/4.0f, 0.,constants.dt/4.0f,0.);
}

void CloudScene::drawSceneObject(VulkanEngine* engine)
{
	// glm::mat4 sandTransForm1 = glm::translate(glm::vec3(0, -40, -20 )) * glm::scale(glm::mat4(1.0f), glm::vec3(20.0f));
	// engine->loadedScenes["cloudCube"]->Draw(sandTransForm1, engine->mainDrawContext);

	engine->mainDrawContext.computeObj.push_back(engine->loadedComputeObj["cloudDensity"].get());
	engine->mainDrawContext.computeObj.push_back(engine->loadedComputeObj["cloudLighting"].get());
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