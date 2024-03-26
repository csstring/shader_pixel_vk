#include "EnvOffscreenRender.hpp"
#include "vk_engine.hpp"
#include "vk_textures.hpp"
#include "vk_initializers.hpp"

void EnvOffscreenRender::createCubeMap(VulkanEngine* engine)
{
	envCubeImage._imageFormat = offscreenImageFormat;
	envCubeImage._imageExtent = { offscreenImageSize, offscreenImageSize, 1 };

	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkImageCreateInfo img_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	img_info.imageType = VK_IMAGE_TYPE_2D;
	img_info.format = offscreenImageFormat;
	img_info.mipLevels = 1;
	img_info.samples = VK_SAMPLE_COUNT_1_BIT;
	img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	img_info.extent = { offscreenImageSize, offscreenImageSize, 1 };
	img_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	img_info.arrayLayers = 6;
	img_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VK_CHECK(vmaCreateImage(engine->_allocator, &img_info, &allocinfo, &envCubeImage._image, &envCubeImage._allocation, nullptr));

	VkImageViewCreateInfo imageViewInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	imageViewInfo.format = offscreenImageFormat;
	imageViewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	imageViewInfo.subresourceRange.layerCount = 6;
	imageViewInfo.subresourceRange.levelCount = 1;
	imageViewInfo.image = envCubeImage._image;

	VK_CHECK(vkCreateImageView(engine->_device, &imageViewInfo, nullptr, &envCubeImage._imageView));

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 6;


  	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.subresourceRange.layerCount = 1;
	imageViewInfo.image = envCubeImage._image;

  	for (uint32_t i = 0; i < 6; i++)
	{
    	imageViewInfo.subresourceRange.baseArrayLayer = i;
		VK_CHECK(vkCreateImageView(engine->_device, &imageViewInfo, nullptr, &envCubeMapFaceImageViews[i]));
	}

	engine->immediate_submit([&](VkCommandBuffer cmd) {
    vkutil::setImageLayout(
			cmd,
			envCubeImage._image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			subresourceRange);
	});


	VkSamplerCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.maxAnisotropy = 1.0f;
	info.magFilter = VK_FILTER_LINEAR;
	info.minFilter = VK_FILTER_LINEAR;
	info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	info.addressModeV = info.addressModeU;
	info.addressModeW = info.addressModeU;
	info.mipLodBias = 0.0f;
	info.compareOp = VK_COMPARE_OP_NEVER;
	info.minLod = 0.0f;
	info.maxLod = 1.0f;
	info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	VK_CHECK(vkCreateSampler(engine->_device, &info, nullptr, &sampler));
	_deletionQueue.push_function([=]() {
    engine->destroy_image(envCubeImage);
		vkDestroySampler(engine->_device, sampler, nullptr);
  });
}

void EnvOffscreenRender::makeOffscreenRenderpass(VulkanEngine* engine)
{
  VkAttachmentDescription osAttachments[2] = {};

  osAttachments[0].format = offscreenImageFormat;
  osAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  osAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  osAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  osAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  osAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  osAttachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  osAttachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // Depth attachment
  osAttachments[1].format = offscreenDepthFormat;
  osAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  osAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  osAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  osAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  osAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  osAttachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  osAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorReference = {};
  colorReference.attachment = 0;
  colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthReference = {};
  depthReference.attachment = 1;
  depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorReference;
  subpass.pDepthStencilAttachment = &depthReference;

  VkRenderPassCreateInfo renderPassCreateInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  renderPassCreateInfo.attachmentCount = 2;
  renderPassCreateInfo.pAttachments = osAttachments;
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpass;
  VK_CHECK(vkCreateRenderPass(engine->_device, &renderPassCreateInfo, nullptr, &offscreenRenderPass));
  _deletionQueue.push_function([=]() {
      vkDestroyRenderPass(engine->_device, offscreenRenderPass, nullptr);
  });
}

void EnvOffscreenRender::makeOffscreenFramebuffer(VulkanEngine* engine)
{
  depthImage._imageFormat = offscreenImageFormat;
	depthImage._imageExtent = { offscreenImageSize, offscreenImageSize, 1 };

	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkImageCreateInfo imageCreateInfo = vkinit::image_create_info(
    offscreenImageFormat, 
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    {offscreenImageSize,offscreenImageSize,1});
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  imageCreateInfo.format = offscreenDepthFormat;
  imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

  VK_CHECK(vmaCreateImage(engine->_allocator, &imageCreateInfo, &allocinfo, &depthImage._image, &depthImage._allocation, nullptr));

  VkImageViewCreateInfo depthStencilView = vkinit::imageview_create_info(
    offscreenDepthFormat,
    depthImage,
    VK_IMAGE_ASPECT_DEPTH_BIT
  );
  engine->immediate_submit([&](VkCommandBuffer cmd){
    vkutil::setImageLayout(
      cmd,
      depthImage._image,
      VK_IMAGE_ASPECT_DEPTH_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  });

  depthStencilView.image = depthImage._image;
  VK_CHECK(vkCreateImageView(engine->_device, &depthStencilView, nullptr, &depthImage._imageView));

  VkImageView attachments[2];
  attachments[1] = depthImage._imageView;

  VkFramebufferCreateInfo fbufCreateInfo = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
  fbufCreateInfo.renderPass = offscreenRenderPass;
  fbufCreateInfo.attachmentCount = 2;
  fbufCreateInfo.pAttachments = attachments;
  fbufCreateInfo.width = offscreenImageSize;
  fbufCreateInfo.height = offscreenImageSize;
  fbufCreateInfo.layers = 1;

  for (uint32_t i = 0; i < 6; i++)
  {
  	attachments[0] = envCubeMapFaceImageViews[i];
  	VK_CHECK(vkCreateFramebuffer(engine->_device, &fbufCreateInfo, nullptr, &frameBuffers[i]));
    _deletionQueue.push_function([=]() {
      vkDestroyFramebuffer(engine->_device, frameBuffers[i], nullptr);
			vkDestroyImageView(engine->_device, envCubeMapFaceImageViews[i], nullptr);
    });
  }

  _deletionQueue.push_function([=]() {
    engine->destroy_image(depthImage);
  });
}
void EnvOffscreenRender::renderCubeFace(VkCommandBuffer cmd, VulkanEngine* engine, VkDescriptorSet globalDescriptor)
{
  GPUDrawPushConstants pushConstants;
	pushConstants.proj = glm::perspective((float)(M_PI / 2.0f), 1.0f, 0.1f, 1024.0f);

	VkClearValue clearValue;
	clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

  VkClearValue depthClear;
	depthClear.depthStencil = { 1.0f, 0};//info의 max깊이값을 1.0으로 해놔서 이걸로 초기화

  VkClearValue clearValues[] = {clearValue, depthClear};

  for (const RenderObject& draw : engine->mainDrawContext.EnvSurfaces)
	{
		VkDeviceSize offset = 0;
		vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipeline);
		vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.material->pipeline->layout, 0,1, &globalDescriptor,0,nullptr );
		vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.material->pipeline->layout, 1,1, &draw.material->materialSet,0,nullptr );
		vkCmdBindIndexBuffer(cmd, draw.indexBuffer,0,VK_INDEX_TYPE_UINT32);
		vkCmdBindVertexBuffers(cmd, 0, 1, &draw.vertexBuffer, &offset);
		for (int faceIndex = 0; faceIndex < 6; faceIndex++){
			VkRenderPassBeginInfo rpInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
			rpInfo.renderPass = offscreenRenderPass;
			rpInfo.framebuffer = frameBuffers[faceIndex];
			rpInfo.renderArea.extent.width = offscreenImageSize;
			rpInfo.renderArea.extent.height = offscreenImageSize;
			rpInfo.clearValueCount = 2;
			rpInfo.pClearValues = clearValues;
			
			vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

			glm::mat4 viewMatrix = glm::mat4(1.0f);
			switch (faceIndex)
			{
			case 0: // POSITIVE_X
				viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				break;
			case 1:	// NEGATIVE_X
				viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				break;
			case 2:	// POSITIVE_Y
				viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				break;
			case 3:	// NEGATIVE_Y
				viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				break;
			case 4:	// POSITIVE_Z
				viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				break;
			case 5:	// NEGATIVE_Z
				viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				break;
			}
			pushConstants.view = viewMatrix;
			vkCmdPushConstants(cmd, draw.material->pipeline->layout ,VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,0, sizeof(GPUDrawPushConstants), &pushConstants);
			vkCmdDrawIndexed(cmd, draw.indexCount,1,draw.firstIndex,0,0);
			vkCmdEndRenderPass(cmd);
		}
	}
}

void EnvOffscreenRender::initialize(VulkanEngine* engine)
{
  createCubeMap(engine);
  makeOffscreenRenderpass(engine);
  makeOffscreenFramebuffer(engine);
}

void EnvOffscreenRender::loadSceneObject(VulkanEngine* engine)
{
  auto ocean = loadGltf(engine,"./assets/models/", "cube.gltf", MaterialPass::MainColor, glm::scale(glm::vec3(0.2f)));//envRender
	auto envoff = loadGltf(engine,"./assets/models/", "cube.gltf", MaterialPass::Offscreen, glm::scale(glm::vec3(0.2f))); //envRender
  auto World1_SkyBox = loadGltf(engine,"./assets/models/", "cube.gltf", MaterialPass::World1_SkyBox, glm::scale(glm::vec3(0.2f))); //vulkan skybox
	auto vulkanmodels = loadGltf(engine,"./assets/models/", "vulkanscenemodels.gltf", MaterialPass::Transparent);
	auto vulkanscenelogos = loadGltf(engine,"./assets/models/", "vulkanscenelogos.gltf", MaterialPass::Transparent);

  assert(ocean.has_value());
	assert(envoff.has_value());
	assert(World1_SkyBox.has_value());
	assert(vulkanmodels.has_value());
	assert(vulkanscenelogos.has_value());

  engine->loadedScenes["ocean"] = *ocean;
	engine->loadedScenes["envoff"] = *envoff;
	engine->loadedScenes["World1_SkyBox"] = *World1_SkyBox;
	engine->loadedScenes["vulkanmodels"] = *vulkanmodels;
	engine->loadedScenes["vulkanscenelogos"] = *vulkanscenelogos;
}

void EnvOffscreenRender::drawSceneObject(VulkanEngine* engine)
{
  glm::mat4 oceanTransForm = glm::translate(glm::vec3(0, -40, -0 )) * glm::scale(glm::mat4(1.0f), glm::vec3(200.0f));
  glm::mat4 skyBoxTransForm = glm::translate(glm::vec3(0, 40, 0 )) *glm::rotate(-80.0f, glm::vec3(1,0,0))*glm::scale(glm::mat4(1.0f), glm::vec3(250.0f));
	glm::mat4 vulkanModelTransForm = glm::translate(glm::vec3(0, 40, -100 )) * glm::scale(glm::mat4(1.0f), glm::vec3(20.0f));

  engine->loadedScenes["envoff"]->Draw(glm::mat4(1.0f), engine->mainDrawContext);
	engine->loadedScenes["ocean"]->Draw(glm::mat4(1.0f) * oceanTransForm, engine->mainDrawContext);

  engine->loadedScenes["World1_SkyBox"]->Draw(skyBoxTransForm, engine->mainDrawContext);
	engine->loadedScenes["vulkanmodels"]->Draw(vulkanModelTransForm ,engine->mainDrawContext);
	engine->loadedScenes["vulkanscenelogos"]->Draw(vulkanModelTransForm ,engine->mainDrawContext);
}