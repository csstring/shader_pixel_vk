#include "PSO.hpp"
#include "vk_pipelines.hpp"
#include "vk_engine.hpp"
#include "vk_initializers.hpp"
#include "Cloud.hpp"

void PSO::buildWorldSkyBoxpipelines(VulkanEngine* engine, GLTFMetallic* metallic)
{
	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayout layouts[] = { 
			engine->_gpuSceneDataDescriptorLayout,
      metallic->materialLayout };

	VkPipelineLayoutCreateInfo skybox_layout_info = vkinit::pipeline_layout_create_info();
	skybox_layout_info.setLayoutCount = 2;
	skybox_layout_info.pSetLayouts = layouts;
	skybox_layout_info.pPushConstantRanges = &matrixRange;
	skybox_layout_info.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(engine->_device, &skybox_layout_info, nullptr, &newLayout));

  	world_skyBoxPipeline.layout = newLayout;
	VertexInputDescription vertexDescription = Vertex::get_vertex_description();
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.loadShader("./spv/skybox.vert.spv", engine->_device, VK_SHADER_STAGE_VERTEX_BIT);
	pipelineBuilder.loadShader("./spv/skybox.frag.spv", engine->_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info(vertexDescription);
	pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,0, false);
	pipelineBuilder._viewport = vkinit::viewport_create_info(engine->_windowExtent);
	pipelineBuilder._scissor = vkinit::scissor_create_info(engine->_windowExtent);
	pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	pipelineBuilder._multisampling = vkinit::multisampling_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);
	pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state(0xf, VK_FALSE);
	pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
	pipelineBuilder._pipelineLayout = newLayout;
	
	pipelineBuilder._depthStencil.stencilTestEnable = VK_TRUE;
	pipelineBuilder._depthStencil.back.compareMask = 0xff;
	pipelineBuilder._depthStencil.back.writeMask = 0xff;
	pipelineBuilder._depthStencil.back.reference = 1;
	pipelineBuilder._depthStencil.back.compareOp = VK_COMPARE_OP_EQUAL;
	pipelineBuilder._depthStencil.back.failOp = VK_STENCIL_OP_KEEP;
	pipelineBuilder._depthStencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
	pipelineBuilder._depthStencil.back.passOp = VK_STENCIL_OP_KEEP;
	pipelineBuilder._depthStencil.front = pipelineBuilder._depthStencil.back;
  world_skyBoxPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device, engine->_renderPass);

	pipelineBuilder.shaderFlush(engine->_device);

	engine->_mainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(engine->_device, newLayout, nullptr);
		vkDestroyPipeline(engine->_device, world_skyBoxPipeline.pipeline, nullptr);
  });
}

void PSO::buildEnvOffscreenPipelines(VulkanEngine* engine, GLTFMetallic* metallic)
{
	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayout layouts[] = { 
			engine->_gpuSceneDataDescriptorLayout,
      metallic->materialLayout };

	VkPipelineLayoutCreateInfo env_layout_info = vkinit::pipeline_layout_create_info();
	env_layout_info.setLayoutCount = 2;
	env_layout_info.pSetLayouts = layouts;
	env_layout_info.pPushConstantRanges = &matrixRange;
	env_layout_info.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(engine->_device, &env_layout_info, nullptr, &newLayout));

  envPipeline.layout = newLayout;
	VertexInputDescription vertexDescription = Vertex::get_vertex_description();
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.loadShader("./spv/offscreen.vert.spv", engine->_device, VK_SHADER_STAGE_VERTEX_BIT);
	pipelineBuilder.loadShader("./spv/offscreen.frag.spv", engine->_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info(vertexDescription);
	pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,0, false);
	pipelineBuilder._viewport = vkinit::viewport_create_info({engine->envRender->offscreenImageSize,engine->envRender->offscreenImageSize});
	pipelineBuilder._scissor = vkinit::scissor_create_info({engine->envRender->offscreenImageSize,engine->envRender->offscreenImageSize});
	pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	pipelineBuilder._multisampling = vkinit::multisampling_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);
	pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state(0xf, VK_FALSE);
	pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
	pipelineBuilder._pipelineLayout = newLayout;
	
  envPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device, engine->envRender->offscreenRenderPass);

	pipelineBuilder.shaderFlush(engine->_device);

	engine->_mainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(engine->_device, newLayout, nullptr);
		vkDestroyPipeline(engine->_device, envPipeline.pipeline, nullptr);
  });
}

void PSO::buildstencilFillpipelines(VulkanEngine* engine, GLTFMetallic* metallic)
{
	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayout layouts[] = { 
			engine->_gpuSceneDataDescriptorLayout,
      metallic->materialLayout };

	VkPipelineLayoutCreateInfo stencilFill_layout_info = vkinit::pipeline_layout_create_info();
	stencilFill_layout_info.setLayoutCount = 2;
	stencilFill_layout_info.pSetLayouts = layouts;
	stencilFill_layout_info.pPushConstantRanges = &matrixRange;
	stencilFill_layout_info.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(engine->_device, &stencilFill_layout_info, nullptr, &newLayout));

	stencilFill_Zero_Pipeline.layout = newLayout;
	stencilFill_One_Pipeline.layout = newLayout;

	VertexInputDescription vertexDescription = Vertex::get_vertex_description();
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.loadShader("./spv/stencilFill.vert.spv", engine->_device, VK_SHADER_STAGE_VERTEX_BIT);
	pipelineBuilder.loadShader("./spv/stencilFill.frag.spv", engine->_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info(vertexDescription);
	pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,0, false);
	pipelineBuilder._viewport = vkinit::viewport_create_info(engine->_windowExtent);
	pipelineBuilder._scissor = vkinit::scissor_create_info(engine->_windowExtent);
	pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	pipelineBuilder._multisampling = vkinit::multisampling_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);
	pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state(0xf, VK_FALSE);
	pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(false, false, VK_COMPARE_OP_LESS_OR_EQUAL);
	pipelineBuilder._pipelineLayout = newLayout;

	pipelineBuilder._depthStencil.stencilTestEnable = VK_TRUE;
	pipelineBuilder._depthStencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
	pipelineBuilder._depthStencil.back.failOp = VK_STENCIL_OP_REPLACE;
	pipelineBuilder._depthStencil.back.depthFailOp = VK_STENCIL_OP_REPLACE;
	pipelineBuilder._depthStencil.back.passOp = VK_STENCIL_OP_REPLACE;
	pipelineBuilder._depthStencil.back.compareMask = 0xff;
	pipelineBuilder._depthStencil.back.writeMask = 0xff;
	pipelineBuilder._depthStencil.back.reference = 0;
	pipelineBuilder._depthStencil.front = pipelineBuilder._depthStencil.back;

  stencilFill_Zero_Pipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device, engine->_renderPass);

	pipelineBuilder._depthStencil.back.reference = 1;
	pipelineBuilder._depthStencil.front = pipelineBuilder._depthStencil.back;
	stencilFill_One_Pipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device, engine->_renderPass);
	pipelineBuilder.shaderFlush(engine->_device);

	engine->_mainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(engine->_device, newLayout, nullptr);
		vkDestroyPipeline(engine->_device, stencilFill_Zero_Pipeline.pipeline, nullptr);
		vkDestroyPipeline(engine->_device, stencilFill_One_Pipeline.pipeline, nullptr);
  });
}

void PSO::build_cloudPipelines(VulkanEngine* engine, GLTFMetallic* metallic)
{
	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayout layouts[] = { 
			engine->_gpuSceneDataDescriptorLayout,
      metallic->materialLayout };

	VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
	mesh_layout_info.setLayoutCount = 2;
	mesh_layout_info.pSetLayouts = layouts;
	mesh_layout_info.pPushConstantRanges = &matrixRange;
	mesh_layout_info.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &newLayout));

  cloudPipeline.layout = newLayout;

	VertexInputDescription vertexDescription = Vertex::get_vertex_description();
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.loadShader("./spv/cloud.vert.spv", engine->_device, VK_SHADER_STAGE_VERTEX_BIT);
	pipelineBuilder.loadShader("./spv/cloud.frag.spv", engine->_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info(vertexDescription);
	pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,0, false);
	pipelineBuilder._viewport = vkinit::viewport_create_info(engine->_windowExtent);
	pipelineBuilder._scissor= vkinit::scissor_create_info(engine->_windowExtent);
	pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	pipelineBuilder._multisampling = vkinit::multisampling_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);
	pipelineBuilder._colorBlendAttachment = vkinit::enable_blending_additive();
	pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
	pipelineBuilder._pipelineLayout = newLayout;

  cloudPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device, engine->_renderPass);
	pipelineBuilder.shaderFlush(engine->_device);

	engine->_mainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(engine->_device, newLayout, nullptr);
		vkDestroyPipeline(engine->_device, cloudPipeline.pipeline, nullptr);
  });
}

void PSO::buildJuliapipelines(VulkanEngine* engine, GLTFMetallic* metallic)
{
	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayout layouts[] = { 
			engine->_gpuSceneDataDescriptorLayout,
      metallic->materialLayout };

	VkPipelineLayoutCreateInfo skybox_layout_info = vkinit::pipeline_layout_create_info();
	skybox_layout_info.setLayoutCount = 2;
	skybox_layout_info.pSetLayouts = layouts;
	skybox_layout_info.pPushConstantRanges = &matrixRange;
	skybox_layout_info.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(engine->_device, &skybox_layout_info, nullptr, &newLayout));

  	juliaPipeline.layout = newLayout;
	VertexInputDescription vertexDescription = Vertex::get_vertex_description();
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.loadShader("./spv/julia.vert.spv", engine->_device, VK_SHADER_STAGE_VERTEX_BIT);
	pipelineBuilder.loadShader("./spv/julia.frag.spv", engine->_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info(vertexDescription);
	pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,0, false);
	pipelineBuilder._viewport = vkinit::viewport_create_info(engine->_windowExtent);
	pipelineBuilder._scissor = vkinit::scissor_create_info(engine->_windowExtent);
	pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	pipelineBuilder._multisampling = vkinit::multisampling_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);
	pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state(0xf, VK_FALSE);
	pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
	pipelineBuilder._pipelineLayout = newLayout;
	pipelineBuilder._depthStencil.stencilTestEnable = VK_TRUE;
	pipelineBuilder._depthStencil.back.compareMask = 0xff;
	pipelineBuilder._depthStencil.back.writeMask = 0xff;
	pipelineBuilder._depthStencil.back.reference = 1;
	pipelineBuilder._depthStencil.back.compareOp = VK_COMPARE_OP_EQUAL;
	pipelineBuilder._depthStencil.back.failOp = VK_STENCIL_OP_KEEP;
	pipelineBuilder._depthStencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
	pipelineBuilder._depthStencil.back.passOp = VK_STENCIL_OP_KEEP;
	pipelineBuilder._depthStencil.front = pipelineBuilder._depthStencil.back;

  	juliaPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device, engine->_renderPass);
	pipelineBuilder.shaderFlush(engine->_device);

	engine->_mainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(engine->_device, newLayout, nullptr);
		vkDestroyPipeline(engine->_device, juliaPipeline.pipeline, nullptr);
  });
}

void PSO::build_pipelines(VulkanEngine* engine, GLTFMetallic* metallic)
{
	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayout layouts[] = { 
			engine->_gpuSceneDataDescriptorLayout,
      metallic->materialLayout };

	VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
	mesh_layout_info.setLayoutCount = 2;
	mesh_layout_info.pSetLayouts = layouts;
	mesh_layout_info.pPushConstantRanges = &matrixRange;
	mesh_layout_info.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(engine->_device, &mesh_layout_info, nullptr, &newLayout));

  	waterPipeline.layout = newLayout;
  	transparentPipeline.layout = newLayout;
	opaquePipeline.layout = newLayout;

	VertexInputDescription vertexDescription = Vertex::get_vertex_description();
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.loadShader("./spv/sdfPrac.vert.spv", engine->_device, VK_SHADER_STAGE_VERTEX_BIT);
	pipelineBuilder.loadShader("./spv/sdfPrac.frag.spv", engine->_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info(vertexDescription);
	pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,0, false);
	pipelineBuilder._viewport = vkinit::viewport_create_info(engine->_windowExtent);
	pipelineBuilder._scissor= vkinit::scissor_create_info(engine->_windowExtent);
	pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	pipelineBuilder._multisampling = vkinit::multisampling_state_create_info(VK_SAMPLE_COUNT_1_BIT, 0);
	pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state(0xf, VK_FALSE);
	pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
	pipelineBuilder._pipelineLayout = newLayout;
	pipelineBuilder._depthStencil.stencilTestEnable = VK_TRUE;
	pipelineBuilder._depthStencil.back.compareMask = 0xff;
	pipelineBuilder._depthStencil.back.writeMask = 0xff;
	pipelineBuilder._depthStencil.back.reference = 0;
	pipelineBuilder._depthStencil.back.compareOp = VK_COMPARE_OP_EQUAL;
	pipelineBuilder._depthStencil.back.failOp = VK_STENCIL_OP_KEEP;
	pipelineBuilder._depthStencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
	pipelineBuilder._depthStencil.back.passOp = VK_STENCIL_OP_KEEP;
	pipelineBuilder._depthStencil.front = pipelineBuilder._depthStencil.back;
  	waterPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device, engine->_renderPass);
	pipelineBuilder.shaderFlush(engine->_device);

	pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	pipelineBuilder.loadShader("./spv/mesh.vert.spv", engine->_device, VK_SHADER_STAGE_VERTEX_BIT);
	pipelineBuilder.loadShader("./spv/mesh.frag.spv", engine->_device, VK_SHADER_STAGE_FRAGMENT_BIT);
	pipelineBuilder._depthStencil.back.reference = 1;
	pipelineBuilder._depthStencil.front = pipelineBuilder._depthStencil.back;
	opaquePipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device, engine->_renderPass);

	pipelineBuilder._colorBlendAttachment = vkinit::enable_blending_additive();
	pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, false, VK_COMPARE_OP_LESS_OR_EQUAL);
	transparentPipeline.pipeline = pipelineBuilder.build_pipeline(engine->_device, engine->_renderPass);
	
	pipelineBuilder.shaderFlush(engine->_device);
	engine->_mainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(engine->_device, newLayout, nullptr);
		vkDestroyPipeline(engine->_device, waterPipeline.pipeline, nullptr);
		vkDestroyPipeline(engine->_device, opaquePipeline.pipeline, nullptr);
		vkDestroyPipeline(engine->_device, transparentPipeline.pipeline, nullptr);
  });
}

void PSO::buildCloudDensityPipelines(VulkanEngine* engine)
{
	
	DescriptorLayoutBuilder builder;
  	builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
  	engine->_cloud->cloudDensityLayout = builder.build(engine->_device, VK_SHADER_STAGE_COMPUTE_BIT);
	engine->_cloud->cloudDensityDescriptor = engine->globalDescriptorAllocator.allocate(engine->_device, engine->_cloud->cloudDensityLayout);

	engine->_mainDeletionQueue.push_function([=]() {
    vkDestroyDescriptorSetLayout(engine->_device,  engine->_cloud->cloudDensityLayout, nullptr);
  });

	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(CloudPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayout layouts[] = { 
			engine->_cloud->cloudDensityLayout
	};

	VkPipelineLayoutCreateInfo cloudDensity = vkinit::pipeline_layout_create_info();
	cloudDensity.setLayoutCount = 1;
	cloudDensity.pSetLayouts = layouts;
	cloudDensity.pPushConstantRanges = &matrixRange;
	cloudDensity.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(engine->_device, &cloudDensity, nullptr, &newLayout));

  cloudDensityPipeline.layout = newLayout;
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.loadShader("./spv/cloudDensity.comp.spv", engine->_device, VK_SHADER_STAGE_COMPUTE_BIT);
	pipelineBuilder._pipelineLayout = newLayout;
	cloudDensityPipeline.pipeline = pipelineBuilder.build_compute_pipeline(engine->_device);
	pipelineBuilder.shaderFlush(engine->_device);

	engine->_mainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(engine->_device, newLayout, nullptr);
		vkDestroyPipeline(engine->_device, cloudDensityPipeline.pipeline, nullptr);
  });
}

void PSO::buildCloudLightingPipelines(VulkanEngine* engine)
{
	
	DescriptorLayoutBuilder builder;
    builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	builder.add_binding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    engine->_cloud->cloudLightingLayout = builder.build(engine->_device, VK_SHADER_STAGE_COMPUTE_BIT);
	engine->_cloud->cloudLightingDescriptor = engine->globalDescriptorAllocator.allocate(engine->_device, engine->_cloud->cloudLightingLayout);
	engine->_mainDeletionQueue.push_function([=]() {
        vkDestroyDescriptorSetLayout(engine->_device,  engine->_cloud->cloudLightingLayout, nullptr);
  	});

	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(CloudPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayout layouts[] = { 
			engine->_cloud->cloudLightingLayout
	};

	VkPipelineLayoutCreateInfo cloudDensity = vkinit::pipeline_layout_create_info();
	cloudDensity.setLayoutCount = 1;
	cloudDensity.pSetLayouts = layouts;
	cloudDensity.pPushConstantRanges = &matrixRange;
	cloudDensity.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(engine->_device, &cloudDensity, nullptr, &newLayout));

  cloudLightingPipeline.layout = newLayout;
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.loadShader("./spv/cloudLighting.comp.spv", engine->_device, VK_SHADER_STAGE_COMPUTE_BIT);
	pipelineBuilder._pipelineLayout = newLayout;

	cloudLightingPipeline.pipeline = pipelineBuilder.build_compute_pipeline(engine->_device);
	
	pipelineBuilder.shaderFlush(engine->_device);

	engine->_mainDeletionQueue.push_function([=]() {
		vkDestroyPipelineLayout(engine->_device, newLayout, nullptr);
		vkDestroyPipeline(engine->_device, cloudLightingPipeline.pipeline, nullptr);
  });
}

void PSO::buildPipeLine(VulkanEngine* engine)
{
  build_pipelines(engine,&(engine->metalRoughMaterial));
	buildWorldSkyBoxpipelines(engine,&(engine->metalRoughMaterial));
	buildstencilFillpipelines(engine,&(engine->metalRoughMaterial));
	build_cloudPipelines(engine,&(engine->metalRoughMaterial));
	buildEnvOffscreenPipelines(engine,&(engine->metalRoughMaterial));
	buildJuliapipelines(engine,&(engine->metalRoughMaterial));
	buildCloudDensityPipelines(engine);
	buildCloudLightingPipelines(engine);
}