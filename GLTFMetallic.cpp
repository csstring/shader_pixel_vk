#include "GLTFMetallic.hpp"
#include "vk_engine.hpp"

MaterialInstance GLTFMetallic::write_material(VulkanEngine* engine, MaterialPass pass, MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)
{
	MaterialInstance matData;
	matData.passType = pass;
	switch (pass)
	{

	case MaterialPass::Transparent:
		matData.pipeline = &engine->_PSO.opaquePipeline;
		resources.skyBoxImage = engine->_vulkanBoxImage;
    resources.skyBoxSampler = engine->_vulkanBoxSamplerLinear;
		break;
	case MaterialPass::MainColor:
		matData.pipeline = &engine->_PSO.waterPipeline;
		resources.colorImage = engine->_cloud->_cloudImageBuffer[0][CLOUDTEXTUREID::CLOUDDENSITY];
    resources.colorSampler = engine->_defaultSamplerLinear;
		resources.metalRoughImage = engine->_cloud->_cloudImageBuffer[0][CLOUDTEXTUREID::CLOUDLIGHT];
		resources.metalRoughSampler = engine->_defaultSamplerLinear;
		// resources.skyBoxImage = engine->_vulkanBoxImage;
    // resources.skyBoxSampler = engine->_vulkanBoxSamplerLinear;
		resources.skyBoxImage = engine->envRender->envCubeImage;
    resources.skyBoxSampler = engine->envRender->sampler;
		matData.materialSet = descriptorAllocator.allocate(engine->_device, materialLayout);

		writer.clear();
		writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.write_image(1, resources.colorImage._imageView, resources.colorSampler, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.write_image(2, resources.metalRoughImage._imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.write_image(3, resources.skyBoxImage._imageView, resources.skyBoxSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.update_set(engine->_device, matData.materialSet);
		return matData;
		break;
	case MaterialPass::Offscreen:
		matData.pipeline = &engine->_PSO.envPipeline;
		resources.colorImage = engine->_cloud->_cloudImageBuffer[0][CLOUDTEXTUREID::CLOUDDENSITY];
    resources.colorSampler = engine->_defaultSamplerLinear;
		resources.metalRoughImage = engine->_cloud->_cloudImageBuffer[0][CLOUDTEXTUREID::CLOUDLIGHT];
		resources.metalRoughSampler = engine->_defaultSamplerLinear;
		resources.skyBoxImage = engine->_vulkanBoxImage;
    resources.skyBoxSampler = engine->_vulkanBoxSamplerLinear;
		matData.materialSet = descriptorAllocator.allocate(engine->_device, materialLayout);

		writer.clear();
		writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.write_image(1, resources.colorImage._imageView, resources.colorSampler, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.write_image(2, resources.metalRoughImage._imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.write_image(3, resources.skyBoxImage._imageView, resources.skyBoxSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.update_set(engine->_device, matData.materialSet);
		return matData;
		break;
	case MaterialPass::Reflect:
		matData.pipeline = &engine->_PSO.reflectPipeline;
		resources.skyBoxImage = engine->_vulkanBoxImage;
    resources.skyBoxSampler = engine->_vulkanBoxSamplerLinear;
		break;
	case MaterialPass::StencilFill_Zero:
		matData.pipeline = &engine->_PSO.stencilFill_Zero_Pipeline;
		resources.skyBoxImage = engine->_vulkanBoxImage;
    resources.skyBoxSampler = engine->_vulkanBoxSamplerLinear;
		break;
	case MaterialPass::StencilFill_One:
		matData.pipeline = &engine->_PSO.stencilFill_One_Pipeline;
		resources.skyBoxImage = engine->_vulkanBoxImage;
    resources.skyBoxSampler = engine->_vulkanBoxSamplerLinear;
		break;
	case MaterialPass::World1_SkyBox:
		matData.pipeline = &engine->_PSO.world_skyBoxPipeline;
		resources.skyBoxImage = engine->_vulkanBoxImage;
    resources.skyBoxSampler = engine->_vulkanBoxSamplerLinear;
		break;
	case MaterialPass::Cloud:
		matData.pipeline = &engine->_PSO.cloudPipeline;
		resources.colorImage = engine->_cloud->_cloudImageBuffer[0][CLOUDTEXTUREID::CLOUDDENSITY];
    resources.colorSampler = engine->_defaultSamplerLinear;
		resources.metalRoughImage = engine->_cloud->_cloudImageBuffer[0][CLOUDTEXTUREID::CLOUDLIGHT];
		resources.metalRoughSampler = engine->_defaultSamplerLinear;
		resources.skyBoxImage = engine->_vulkanBoxImage;
    resources.skyBoxSampler = engine->_vulkanBoxSamplerLinear;
		matData.materialSet = descriptorAllocator.allocate(engine->_device, materialLayout);

		writer.clear();
		writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.write_image(1, resources.colorImage._imageView, resources.colorSampler, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.write_image(2, resources.metalRoughImage._imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.write_image(3, resources.skyBoxImage._imageView, resources.skyBoxSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		writer.update_set(engine->_device, matData.materialSet);
		return matData;
	case MaterialPass::Julia:
		matData.pipeline = &engine->_PSO.juliaPipeline;
		resources.skyBoxImage = engine->_vulkanBoxImage;
    resources.skyBoxSampler = engine->_vulkanBoxSamplerLinear;
		break;
	default:
		break;
	}

	matData.materialSet = descriptorAllocator.allocate(engine->_device, materialLayout);

	writer.clear();
	writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.write_image(1, resources.colorImage._imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.write_image(2, resources.metalRoughImage._imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.write_image(3, resources.skyBoxImage._imageView, resources.skyBoxSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.update_set(engine->_device, matData.materialSet);

	return matData;
}
