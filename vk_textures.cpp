#include "vk_textures.hpp"
#include <iostream>
#include "vk_initializers.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool vkutil::load_image_from_file(VulkanEngine& engine, const char* file, AllocatedImage& outImage)
{
	// int texWidth, texHeight, texChannels;

	// stbi_uc* pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	// if (!pixels) {
	// 	std::cout << "Failed to load texture file " << file << std::endl;
	// 	return false;
	// }

  // void* pixel_ptr = pixels;
	// VkDeviceSize imageSize = texWidth * texHeight * 4;
	// VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;
	// AllocatedBuffer stagingBuffer = engine.create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	// void* data;
	// vmaMapMemory(engine._allocator, stagingBuffer.allocation, &data);
	// memcpy(data, pixel_ptr, imageSize);
	// vmaFlushAllocation(engine._allocator, stagingBuffer.allocation, 0, VK_WHOLE_SIZE);
	// vmaUnmapMemory(engine._allocator, stagingBuffer.allocation);
	// stbi_image_free(pixels);

	// VkExtent3D imageExtent;
	// imageExtent.width = static_cast<uint32_t>(texWidth);
	// imageExtent.height = static_cast<uint32_t>(texHeight);
	// imageExtent.depth = 1;

	// VkImageCreateInfo dimg_info = vkinit::image_create_info(image_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
	// AllocatedImage newImage;
	// VmaAllocationCreateInfo dimg_allocinfo = {};
	// dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	// vmaCreateImage(engine._allocator, &dimg_info, &dimg_allocinfo, &newImage._image, &newImage._allocation, nullptr);
	
	// engine.immediate_submit([&](VkCommandBuffer cmd) {
	// 	VkImageSubresourceRange range;
	// 	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	// 	range.baseMipLevel = 0;
	// 	range.levelCount = 1;
	// 	range.baseArrayLayer = 0;
	// 	range.layerCount = 1;

	// 	VkImageMemoryBarrier imageBarrier_toTransfer = {};
	// 	imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	// 	imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// 	imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	// 	imageBarrier_toTransfer.image = newImage._image;
	// 	imageBarrier_toTransfer.subresourceRange = range;

	// 	imageBarrier_toTransfer.srcAccessMask = 0;
	// 	imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	// 	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);
		
	// 	VkBufferImageCopy copyRegion = {};
	// 	copyRegion.bufferOffset = 0;
	// 	copyRegion.bufferRowLength = 0;
	// 	copyRegion.bufferImageHeight = 0;

	// 	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	// 	copyRegion.imageSubresource.mipLevel = 0;
	// 	copyRegion.imageSubresource.baseArrayLayer = 0;
	// 	copyRegion.imageSubresource.layerCount = 1;
	// 	copyRegion.imageExtent = imageExtent;

	// 	vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
		
	// 	VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;

	// 	imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	// 	imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// 	imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	// 	imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	// 	//barrier the image into the shader readable layout
	// 	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);	
	// });

	// engine._mainDeletionQueue.push_function([=]() {
	// 	vmaDestroyImage(engine._allocator, newImage._image, newImage._allocation);
	// });

	// vmaDestroyBuffer(engine._allocator, stagingBuffer.buffer, stagingBuffer.allocation);

	// std::cout << "Texture loaded successfully " << file << std::endl;

	// outImage = newImage;
	// return true;
}

void vkutil::blitCopy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent3D srcSize, VkExtent3D dstSize)
{
  VkImageBlit blitRegion{};
  
  // Setting up source coordinates
  blitRegion.srcOffsets[0] = {0, 0, 0};  // starting at the origin
  blitRegion.srcOffsets[1].x = srcSize.width;
  blitRegion.srcOffsets[1].y = srcSize.height;
  blitRegion.srcOffsets[1].z = srcSize.depth; // we're working with 2D images, so the Z dimension is 1

  // Setting up destination coordinates
  blitRegion.dstOffsets[0] = {0, 0, 0};  // starting at the origin
  blitRegion.dstOffsets[1].x = dstSize.width;
  blitRegion.dstOffsets[1].y = dstSize.height;
  blitRegion.dstOffsets[1].z = dstSize.depth; // we're working with 2D images, so the Z dimension is 1

  // Setting up the subresource layers for the blit operation
  blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blitRegion.srcSubresource.mipLevel = 0;
  blitRegion.srcSubresource.baseArrayLayer = 0;
  blitRegion.srcSubresource.layerCount = 1;

  blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blitRegion.dstSubresource.mipLevel = 0;
  blitRegion.dstSubresource.baseArrayLayer = 0;
  blitRegion.dstSubresource.layerCount = 1;

  // Issue the blit command
  vkCmdBlitImage(
    cmd,
    source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // source image and its layout
    destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // destination image and its layout
    1, &blitRegion, // blit region
    VK_FILTER_LINEAR // filter to use for resampling
  );
}

void vkutil::copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent3D srcSize, VkExtent3D dstSize)
{
  VkImageBlit blitRegion{};

  blitRegion.srcOffsets[0] = {0, 0, 0};
  blitRegion.srcOffsets[1] = {static_cast<int32_t>(srcSize.width), static_cast<int32_t>(srcSize.height), static_cast<int32_t>(srcSize.depth)};
  blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blitRegion.srcSubresource.mipLevel = 0;
  blitRegion.srcSubresource.baseArrayLayer = 0;
  blitRegion.srcSubresource.layerCount = 1;

  blitRegion.dstOffsets[0] = {0, 0, 0};
  blitRegion.dstOffsets[1] = {static_cast<int32_t>(dstSize.width), static_cast<int32_t>(dstSize.height), static_cast<int32_t>(srcSize.depth)};
  blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blitRegion.dstSubresource.mipLevel = 0;
  blitRegion.dstSubresource.baseArrayLayer = 0;
  blitRegion.dstSubresource.layerCount = 1;

    vkCmdBlitImage(
        cmd,
        source,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        destination,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &blitRegion,
        VK_FILTER_LINEAR
    );
}


void vkutil::transitionImageLayout(
  VkCommandBuffer cmd, VkImage image, VkFormat format, 
  VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange)
{
    VkImageMemoryBarrier imageBarrier{};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier.oldLayout = oldLayout;
    imageBarrier.newLayout = newLayout;
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.image = image;
    imageBarrier.subresourceRange = subresourceRange;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        imageBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
        imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    // Case 8: Color Attachment -> Present Src
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    // Case 10: Transfer Src -> Shader Read Only
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
      std::cerr << "old layout : " << oldLayout << "new layout : " << newLayout << std::endl;
      std::cerr<< "not support transition " << std::endl;
    }

    vkCmdPipelineBarrier(
        cmd,
        sourceStage, destinationStage,
        0, // Dependency flags
        0, nullptr, // Memory Barrier count + Pointer
        0, nullptr, // Buffer Memory Barrier count + Pointer
        1, &imageBarrier // Image Memory Barrier count + Pointer
    );
}
