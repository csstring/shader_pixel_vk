#pragma once
#include "vk_types.hpp"
#include "vk_descriptors.hpp"

class GLTFMetallic {

  public :
	VkDescriptorSetLayout materialLayout;

	struct MaterialConstants {
		glm::vec4 colorFactors;
		glm::vec4 metal_rough_factors;
		glm::vec4 extra[14];
	};

	struct MaterialResources {
		AllocatedImage colorImage;
		VkSampler colorSampler;
		AllocatedImage metalRoughImage;
		VkSampler metalRoughSampler;
    AllocatedImage skyBoxImage;
		VkSampler skyBoxSampler;
		VkBuffer dataBuffer;
		uint32_t dataBufferOffset;
	};
  
  DescriptorWriter writer;
	MaterialInstance write_material(VulkanEngine* engine, MaterialPass pass, MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);
};