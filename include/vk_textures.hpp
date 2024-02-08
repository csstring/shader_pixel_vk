#pragma once

#include "vk_types.hpp"
#include "vk_engine.hpp"

namespace vkutil {

	bool load_image_from_file(VulkanEngine& engine, const char* file, AllocatedImage& outImage);
	void blitCopy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent3D srcSize, VkExtent3D dstSize);
	void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent3D srcSize, VkExtent3D dstSize);
	void transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange);

}