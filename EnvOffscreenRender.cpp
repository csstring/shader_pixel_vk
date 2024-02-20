#include "EnvOffscreenRender.hpp"
#include "vk_engine.hpp"
#include "vk_textures.hpp"
#include "vk_initializers.hpp"

EnvOffscreenRender::EnvOffscreenRender()
{
  offscreenImageSize = 1024;
}

EnvOffscreenRender::~EnvOffscreenRender()
{
  _deletionQueue.flush();
}

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

  //이거 무슨 코드인지 모르겠네
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
void EnvOffscreenRender::renderCubeFace(uint32_t faceIndex, VkCommandBuffer commandBuffer,VulkanEngine* engine)
{

}

void EnvOffscreenRender::initialize(VulkanEngine* engine)
{
  createCubeMap(engine);
  makeOffscreenRenderpass(engine);
  makeOffscreenFramebuffer(engine);
}