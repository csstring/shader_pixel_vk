#include "vk_engine.hpp"
#include "SDL.h"
#include "SDL_vulkan.h"
#include "vk_initializers.hpp"
#include "vk_types.hpp"
#include "VkBootstrap.h"
#include <chrono>
#include <thread>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "vk_pipelines.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include <glm/gtx/transform.hpp>
#include <algorithm>
#include "vk_textures.hpp"
#include <random>
#include "Camera.hpp"
#include "SDL_Event.hpp"
#include "Cloud.hpp"
#include "PSO.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "Particle.hpp"

void VulkanEngine::init()
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

	_window = SDL_CreateWindow(
		"Vulkan Engine", //window title
		SDL_WINDOWPOS_UNDEFINED, //window position x (don't care)
		SDL_WINDOWPOS_UNDEFINED, //window position y (don't care)
		_windowExtent.width,  //window width in pixels
		_windowExtent.height, //window height in pixels
		window_flags 
	);

  init_vulkan();
	init_swapchain();
  init_commands();
  init_default_renderpass();
	init_framebuffers();
  init_sync_structures();

	envRender = new EnvOffscreenRender();
	envRender->initialize(this);
	init_descriptors();
	_cloud = new CloudScene();
	_cloud->initialize(this);
	_particle = new Particle();
	_particle->initialize(this);
  	init_pipelines();
	load_meshes();
	loadCubeMap(this, "./assets/textures/", "cubemap_vulkan.ktx", VK_FORMAT_R8G8B8A8_UNORM, &_vulkanBoxSamplerLinear, &_vulkanBoxImage);

	init_imgui();
	_isInitialized = true;
  	init_scene();
	_portalManager.initialize(glm::scale(glm::mat4(1.0f), glm::vec3(10.0f)), glm::mat4(1.0f) ,glm::translate(glm::vec3(0, 50, -4 )), PortalState::In_World0);
  // auto FlightHelmetFile = loadGltf(this,"./assets/models/FlightHelmet/glTF/", "FlightHelmet.gltf");
  // assert(FlightHelmetFile.has_value());
  // loadedScenes["FlightHelmet"] = *FlightHelmetFile;

	auto sphere = loadGltf(this,"./assets/models/", "sphere.gltf", MaterialPass::MainColor);
	auto World1_SkyBox = loadGltf(this,"./assets/models/", "cube.gltf", MaterialPass::World1_SkyBox, glm::scale(glm::vec3(0.2f)));
	auto julia = loadGltf(this,"./assets/models/", "cube.gltf", MaterialPass::Julia, glm::scale(glm::vec3(0.2f)));
	auto cloudCube = loadGltf(this,"./assets/models/", "cube.gltf", MaterialPass::Cloud, glm::scale(glm::vec3(0.2f)));
	auto sand = loadGltf(this,"./assets/models/", "cube.gltf", MaterialPass::MainColor, glm::scale(glm::vec3(0.2f)));
	auto envoff = loadGltf(this,"./assets/models/", "cube.gltf", MaterialPass::Offscreen, glm::scale(glm::vec3(0.2f)));

	auto vulkanmodels = loadGltf(this,"./assets/models/", "vulkanscenemodels.gltf", MaterialPass::Transparent);
	auto vulkanscenelogos = loadGltf(this,"./assets/models/", "vulkanscenelogos.gltf", MaterialPass::Transparent);
	auto StencilFill_Zero = loadGltf(this,"./assets/models/", "plane_z.gltf", MaterialPass::StencilFill_Zero);
	auto StencilFill_One = loadGltf(this,"./assets/models/", "plane_z.gltf", MaterialPass::StencilFill_One);


	auto cloudDensity = loadComputeObj(this, MaterialPass::CloudDensity);
	auto cloudLighting = loadComputeObj(this, MaterialPass::CloudLighting);

	auto particleCube = loadGltf(this,"./assets/models/", "cube.gltf", MaterialPass::ParticleRender, glm::scale(glm::vec3(0.2f)));
	auto particleComp = loadComputeObj(this, MaterialPass::ParticleComp);
	assert(World1_SkyBox.has_value());
	assert(sphere.has_value());
	assert(sand.has_value());
	assert(StencilFill_Zero.has_value());
	assert(StencilFill_One.has_value());
	assert(cloudCube.has_value());
	assert(envoff.has_value());
	assert(julia.has_value());
	assert(cloudDensity.has_value());
	assert(cloudLighting.has_value());
	assert(vulkanmodels.has_value());
	assert(vulkanscenelogos.has_value());
	assert(particleComp.has_value());
	assert(particleCube.has_value());

	loadedScenes["World1_SkyBox"] = *World1_SkyBox;
	loadedScenes["sphere"] = *sphere;
	loadedScenes["sand"] = *sand;
	loadedScenes["StencilFill_Zero"] = *StencilFill_Zero;
	loadedScenes["StencilFill_One"] = *StencilFill_One;
	loadedScenes["cloudCube"] = *cloudCube;
	loadedScenes["envoff"] = *envoff;
	loadedScenes["julia"] = *julia;
	loadedScenes["vulkanmodels"] = *vulkanmodels;
	loadedScenes["vulkanscenelogos"] = *vulkanscenelogos;
	loadedComputeObj["cloudDensity"] = *cloudDensity;
	loadedComputeObj["cloudLighting"] = *cloudLighting;
	loadedComputeObj["particleComp"] = *particleComp;
	loadedScenes["particleCube"] = *particleCube;
}

void VulkanEngine::init_vulkan()
{
  vkb::InstanceBuilder builder;

	//make the Vulkan instance, with basic debug features
	auto inst_ret = builder.set_app_name("Example Vulkan Application")
		.request_validation_layers(true)
		.require_api_version(1, 3, 0)
		.use_default_debug_messenger()
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	//store the instance
	_instance = vkb_inst.instance;
	//store the debug messenger
	_debug_messenger = vkb_inst.debug_messenger;
  SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 2)
		.set_surface(_surface)
		.select()
		.value();

	//create the final Vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	VkPhysicalDeviceShaderDrawParametersFeatures shader_draw_parameters_features = {};
  shader_draw_parameters_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
  shader_draw_parameters_features.pNext = nullptr;
  shader_draw_parameters_features.shaderDrawParameters = VK_TRUE;
  vkb::Device vkbDevice = deviceBuilder.add_pNext(&shader_draw_parameters_features).build().value();

	// Get the VkDevice handle used in the rest of a Vulkan application
	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;

  _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
	// _computeQueue = vkbDevice.get_queue(vkb::QueueType::compute).value();
	// _computeQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::compute).value();

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = _chosenGPU;
  allocatorInfo.device = _device;
  allocatorInfo.instance = _instance;
  vmaCreateAllocator(&allocatorInfo, &_allocator);

	_gpuProperties = vkbDevice.physical_device.properties;
	std::cout << "The GPU has a minimum buffer alignment of " << _gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;
}

void VulkanEngine::init_swapchain()
{
  vkb::SwapchainBuilder swapchainBuilder{_chosenGPU,_device,_surface };

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
		.set_desired_extent(_windowExtent.width, _windowExtent.height)
		.build()
		.value();

	//store swapchain and its related images
	_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();

	_swapchainImageFormat = vkbSwapchain.image_format;
  _mainDeletionQueue.push_function([=]() {
		vkDestroySwapchainKHR(_device, _swapchain, nullptr);
	});

  VkExtent3D depthImageExtent = {
      _windowExtent.width,
      _windowExtent.height,
      1
  };
	//hardcoding the depth format to 32 bit float
	_depthImage._imageFormat = VK_FORMAT_D24_UNORM_S8_UINT;

	//the depth image will be an image with the format we selected and Depth Attachment usage flag
	VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthImage._imageFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

	//for the depth image, we want to allocate it from GPU local memory
	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//allocate and create the image
	vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &_depthImage._image, &_depthImage._allocation, nullptr);

	_depthImage._imageExtent = depthImageExtent;
	VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthImage._imageFormat, _depthImage, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

	VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImage._imageView));

	//add to deletion queues
	_mainDeletionQueue.push_function([=]() {
		destroy_image(_depthImage);
	});
}

void VulkanEngine::init_commands()
{
  VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	for (int i = 0; i < FRAME_OVERLAP; i++) {
		VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
		_mainDeletionQueue.push_function([=]() {
			vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
		});
	}

	VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily);
	VK_CHECK(vkCreateCommandPool(_device, &uploadCommandPoolInfo, nullptr, &_uploadContext._commandPool));
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_uploadContext._commandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_uploadContext._commandBuffer));

	_mainDeletionQueue.push_function([=](){
		vkDestroyCommandPool(_device, _uploadContext._commandPool, nullptr);
	});

}

void VulkanEngine::init_default_renderpass()
{
  VkAttachmentDescription color_attachment = {};
	//the attachment will have the format needed by the swapchain
	color_attachment.format = _swapchainImageFormat;
	//1 sample, we won't be doing MSAA
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// we Clear when this attachment is loaded
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// we keep the attachment stored when the renderpass ends
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	//we don't care about stencil
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

	//we don't know or care about the starting layout of the attachment
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	//after the renderpass ends, the image has to be on a layout ready for display
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  VkAttachmentReference color_attachment_ref = {};
	//attachment number will index into the pAttachments array in the parent renderpass itself
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  
  VkAttachmentDescription depth_attachment = {};
    // Depth attachment
  depth_attachment.flags = 0;
  depth_attachment.format = _depthImage._imageFormat;
  depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_ref = {};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//we are going to create 1 subpass, which is the minimum you can do
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
  subpass.pDepthStencilAttachment = &depth_attachment_ref;

  VkAttachmentDescription attachments[2] = {color_attachment, depth_attachment};
  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkSubpassDependency depth_dependency = {};
  depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  depth_dependency.dstSubpass = 0;
  depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  depth_dependency.srcAccessMask = 0;
  depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  
  VkSubpassDependency dependencies[2] = { dependency, depth_dependency };
  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.dependencyCount = 2;
  render_pass_info.pDependencies = &dependencies[0];
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	//connect the color attachment to the info
	render_pass_info.attachmentCount = 2;
	render_pass_info.pAttachments = &attachments[0];
	//connect the subpass to the info
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_renderPass));

  _mainDeletionQueue.push_function([=]() {
		vkDestroyRenderPass(_device, _renderPass, nullptr);
  });
}

void VulkanEngine::init_framebuffers()
{
	//create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
	VkFramebufferCreateInfo fb_info = {};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.pNext = nullptr;

	fb_info.renderPass = _renderPass;
	fb_info.attachmentCount = 1;
	fb_info.width = _windowExtent.width;
	fb_info.height = _windowExtent.height;
	fb_info.layers = 1;

	//grab how many images we have in the swapchain
	const uint32_t swapchain_imagecount = _swapchainImages.size();
	_framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

	//create framebuffers for each of the swapchain image views
	for (int i = 0; i < swapchain_imagecount; i++) {
    VkImageView attachments[2];
    attachments[0] = _swapchainImageViews[i];
	  attachments[1] = _depthImage._imageView;
		fb_info.pAttachments = attachments;
    fb_info.attachmentCount = 2;
		VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]));
    
    _mainDeletionQueue.push_function([=]() {
			vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
			vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
    });
  }
}

void VulkanEngine::init_sync_structures()
{
	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();
	for (int i = 0; i < FRAME_OVERLAP; i++) {     
    VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._presentSemaphore));
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
    _mainDeletionQueue.push_function([=]() {
      vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
      vkDestroySemaphore(_device, _frames[i]._presentSemaphore, nullptr);
      vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
    });
	}

	VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fence_create_info();
	VK_CHECK(vkCreateFence(_device, &uploadFenceCreateInfo, nullptr, &_uploadContext._uploadFence));
	_mainDeletionQueue.push_function([=](){
		vkDestroyFence(_device, _uploadContext._uploadFence, nullptr);
	});

}

void VulkanEngine::init_descriptors()
{
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 2.f },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2.f }
    };
	globalDescriptorAllocator.init(_device, 10, sizes);
	{
		DescriptorLayoutBuilder builder;
		builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		_gpuSceneDataDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	_mainDeletionQueue.push_function([&]() {
		vkDestroyDescriptorSetLayout(_device, _gpuSceneDataDescriptorLayout, nullptr);
		globalDescriptorAllocator.destroy_pools(_device);
  });

	for (int i =0; i < FRAME_OVERLAP; ++i){
		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = { 
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
		};

		_frames[i]._frameDescriptors = DescriptorAllocatorGrowable{};
		_frames[i]._frameDescriptors.init(_device, 1000, frame_sizes);
	
		_mainDeletionQueue.push_function([&, i]() {
			_frames[i]._frameDescriptors.destroy_pools(_device);
		});
	}
	DescriptorLayoutBuilder layoutBuilder;
  layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.add_binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

  metalRoughMaterial.materialLayout = layoutBuilder.build(_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	_mainDeletionQueue.push_function([&]() {
		vkDestroyDescriptorSetLayout(_device, metalRoughMaterial.materialLayout, nullptr);
  });
}

void VulkanEngine::init_pipelines()
{
	_PSO.buildPipeLine(this);
}

//------------------run-------------
void VulkanEngine::run()
{
	SDL_Event e;
	bool bQuit = false;
	Camera& camera = Camera::getInstance();
	//main loop
	while (!bQuit)
	{
		vkutil::SDL_event_process(&e, camera, bQuit);
		update_scene();
		camera.update();
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame(_window);

		ImGui::NewFrame();
// b.WaterTurbulence c.WaterAbsorption d.color
    ImGui::Begin("Scene controller");
  	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Text("carmera pos x : %f y : %f z : %f", camera._cameraPos.x, camera._cameraPos.y, camera._cameraPos.z);
		ImGui::SliderFloat3("light pos", &sceneData.sunlightDirection.x, -1, 1);
		ImGui::SliderFloat3("light color", &sceneData.sunlightColor.x, 0, 1);
		ImGui::SliderFloat("light power", &sceneData.sunlightColor.w, 0, 100);
		ImGui::SliderFloat("WaterTurbulence", &sceneData.waterData.y, -20, 20);
		ImGui::SliderFloat("WaterAbsorption", &sceneData.waterData.z, 0, 1);
		ImGui::SliderFloat("Water wave", &sceneData.waterData.w, 0, 15);
		ImGui::End();
		_cloud->guiRender();
		// _cloud->update(1.0f/120.f);
		draw();
	}
}
//------------------draw-------------

void VulkanEngine::draw()
{
	VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

	get_current_frame()._deletionQueue.flush();
	get_current_frame()._frameDescriptors.clear_pools(_device);

  uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._presentSemaphore, nullptr, &swapchainImageIndex));
  VK_CHECK(vkResetCommandBuffer(get_current_frame()._mainCommandBuffer, 0));
	

  VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;
	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo = {};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = nullptr;

	cmdBeginInfo.pInheritanceInfo = nullptr;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// cloudScene->update(1./120., _frameNumber % 2);
	ImGui::Render();
  draw_objects(cmd, swapchainImageIndex);
	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));

  VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit.pWaitDstStageMask = &waitStage;

	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &get_current_frame()._presentSemaphore;

	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &get_current_frame()._renderSemaphore;

	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));
  
  VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;

	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

	_frameNumber++;
}

void VulkanEngine::cleanup()
{	
	if (_isInitialized) {
		vkDeviceWaitIdle(_device);
    	loadedScenes.clear();
		delete _cloud;
		delete envRender;
		delete _particle;
		for (int i =0; i < FRAME_OVERLAP; ++i){
    	vkWaitForFences(_device, 1, &_frames[i]._renderFence, true, 1000000000);
			_frames[i]._deletionQueue.flush();
		}
    _mainDeletionQueue.flush();

    vmaDestroyAllocator(_allocator);
		vkDestroyDevice(_device, nullptr);
		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
		vkDestroyInstance(_instance, nullptr);
		SDL_DestroyWindow(_window);
	}
}

void VulkanEngine::load_meshes()
{ 
	uint32_t white = 0xFFFFFFFF;
	uint32_t magenta = 0xFF00FFFF;

	std::array<uint32_t, 16 *16 > pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : white;
		}
	}
	_errorCheckerboardImage = create_image(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT);

	VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerLinear);

	GLTFMetallic::MaterialResources materialResources;
	//default the material textures
	materialResources.colorImage = _errorCheckerboardImage;
	materialResources.colorSampler = _defaultSamplerLinear;
	materialResources.metalRoughImage = _errorCheckerboardImage;
	materialResources.metalRoughSampler = _defaultSamplerLinear;

	//set the uniform buffer for the material data
	AllocatedBuffer materialConstants = create_buffer(sizeof(GLTFMetallic::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	//write the buffer
	GLTFMetallic::MaterialConstants* sceneUniformData = (GLTFMetallic::MaterialConstants*)materialConstants.allocation->GetMappedData();
	sceneUniformData->colorFactors = glm::vec4{1,1,1,1};
	sceneUniformData->metal_rough_factors = glm::vec4{1,0.5,0,0};

	_mainDeletionQueue.push_function([=]() {
		destroy_buffer(materialConstants);
		destroy_image(_errorCheckerboardImage);
		vkDestroySampler(_device, _defaultSamplerLinear, nullptr);
	});

	materialResources.dataBuffer = materialConstants.buffer;
	materialResources.dataBufferOffset = 0;
}

void VulkanEngine::upload_mesh(Mesh& mesh)
{
	const size_t bufferSize = mesh._vertices.size() * sizeof(Vertex);

	mesh._vertexBuffer = create_buffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ,
		VMA_MEMORY_USAGE_GPU_ONLY);
	AllocatedBuffer staging = create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data;
	vmaMapMemory(_allocator, staging.allocation, &data);
	memcpy(data, mesh._vertices.data(), bufferSize);
	vmaFlushAllocation(_allocator, staging.allocation, 0, VK_WHOLE_SIZE);
  vmaUnmapMemory(_allocator, staging.allocation);

	immediate_submit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{ 0 };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = bufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, mesh._vertexBuffer.buffer, 1, &vertexCopy);
	});

	_mainDeletionQueue.push_function([=]() {
	vmaDestroyBuffer(_allocator, mesh._vertexBuffer.buffer, mesh._vertexBuffer.allocation);
	});
	vmaDestroyBuffer(_allocator, staging.buffer, staging.allocation);
}

GPUMeshBuffers VulkanEngine::uploadMeshBuffers(std::vector<uint32_t> indices, std::vector<Vertex> vertices)
{
	const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	const size_t indexBufferSize = indices.size() * sizeof(uint32_t);
	GPUMeshBuffers newSurface;

	newSurface.vertexBuffer = create_buffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ,
		VMA_MEMORY_USAGE_GPU_ONLY);
	newSurface.indexBuffer = create_buffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	AllocatedBuffer staging = create_buffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data;

	vmaMapMemory(_allocator, staging.allocation, &data);
	memcpy(data, vertices.data(), vertexBufferSize);
  vmaUnmapMemory(_allocator, staging.allocation);

	vmaMapMemory(_allocator, staging.allocation, &data);
	memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);
  vmaUnmapMemory(_allocator, staging.allocation);

	vmaFlushAllocation(_allocator, staging.allocation, 0, VK_WHOLE_SIZE);
	immediate_submit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{ 0 };
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

		VkBufferCopy indexCopy{ 0 };
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size = indexBufferSize;

		vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
	});

	vmaDestroyBuffer(_allocator, staging.buffer, staging.allocation);

	return newSurface;
}
Material* VulkanEngine::create_material(VkPipeline pipeline, VkPipelineLayout layout, 
const std::string& name, uint32_t layoutCount, VkDescriptorSet descriptorSet, 
uint32_t constantSize, void* constantPtr)
{
	Material mat;
	mat.setLayoutCount = layoutCount;
	mat.textureSet = descriptorSet;
	mat.pipeline = pipeline;
	mat.pipelineLayout = layout;
	mat.constantSize = constantSize;
	mat.constant = constantPtr;
	_materials[name] = mat;
	return &_materials[name];
}

Material* VulkanEngine::get_material(const std::string& name)
{
	//search for the object, and return nullptr if not found
	auto it = _materials.find(name);
	if (it == _materials.end()) {
		return nullptr;
	}
	else {
		return &(*it).second;
	}
}

Mesh* VulkanEngine::get_mesh(const std::string& name)
{
	auto it = _meshes.find(name);
	if (it == _meshes.end()) {
		return nullptr;
	}
	else {
		return &(*it).second;
	}
}

void VulkanEngine::init_scene()
{
	sceneData.sunlightDirection = glm::vec4(1.f, 1.f, -1.f, 1.0f);
	sceneData.sunlightColor = glm::vec4(0.0f);
	sceneData.waterData.y = 1.43;
	sceneData.waterData.z = 0.28;
	sceneData.waterData.w = 1;
}

void VulkanEngine::computeCloud(VkCommandBuffer cmd)
{
	for (const ComputeObject* draw : mainDrawContext.computeObj)
	{
		if (draw->material.passType == MaterialPass::CloudDensity || draw->material.passType == MaterialPass::CloudLighting){
			vkCmdPushConstants(cmd, draw->material.pipeline->layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CloudPushConstants), &_cloud->constants);
		} else if (draw->material.passType == MaterialPass::ParticleComp){
			vkCmdPushConstants(cmd, draw->material.pipeline->layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ParticlePushConstants), &_particle->constant);
		}
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, draw->material.pipeline->pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, draw->material.pipeline->layout, 0, 1, &draw->material.materialSet, 0, nullptr);
		vkCmdDispatch(cmd, draw->dispatchX, draw->dispatchY, draw->dispatchZ);
	}
}

void VulkanEngine::draw_objects(VkCommandBuffer cmd, uint32_t swapchainImageIndex)
{
	computeCloud(cmd);

	AllocatedBuffer gpuSceneDataBuffer = create_buffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	Camera& _camera = Camera::getInstance();
	//add it to the deletion queue of this frame so it gets deleted once its been used
	get_current_frame()._deletionQueue.push_function([=]() {
    destroy_buffer(gpuSceneDataBuffer);
	});

	VkDescriptorSet globalDescriptor = get_current_frame()._frameDescriptors.allocate(_device, _gpuSceneDataDescriptorLayout);

	DescriptorWriter writer;
	writer.write_buffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.update_set(_device, globalDescriptor);

	void* data;
	vmaMapMemory(_allocator, gpuSceneDataBuffer.allocation, &data);
	memcpy(data, &sceneData, sizeof(GPUSceneData));
	vmaFlushAllocation(_allocator, gpuSceneDataBuffer.allocation, 0, VK_WHOLE_SIZE);
	vmaUnmapMemory(_allocator, gpuSceneDataBuffer.allocation);

	GPUDrawPushConstants pushConstants;
	pushConstants.proj = glm::perspective((float)(M_PI / 2.0f), 1.0f, 0.1f, 1024.0f);

	VkClearValue clearValue;
	clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

  VkClearValue depthClear;
	depthClear.depthStencil = { 1.0f, 0};//info의 max깊이값을 1.0으로 해놔서 이걸로 초기화

  VkClearValue clearValues[] = {clearValue, depthClear};
	for (const RenderObject& draw : mainDrawContext.EnvSurfaces)
	{
		VkDeviceSize offset = 0;
		vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipeline);
		vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.material->pipeline->layout, 0,1, &globalDescriptor,0,nullptr );
		vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.material->pipeline->layout, 1,1, &draw.material->materialSet,0,nullptr );
		vkCmdBindIndexBuffer(cmd, draw.indexBuffer,0,VK_INDEX_TYPE_UINT32);
		vkCmdBindVertexBuffers(cmd, 0, 1, &draw.vertexBuffer, &offset);
		for (int faceIndex = 0; faceIndex < 6; faceIndex++){
			VkRenderPassBeginInfo rpInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
			// Reuse render pass from example pass
			rpInfo.renderPass = envRender->offscreenRenderPass;
			rpInfo.framebuffer = envRender->frameBuffers[faceIndex];
			rpInfo.renderArea.extent.width = envRender->offscreenImageSize;
			rpInfo.renderArea.extent.height = envRender->offscreenImageSize;
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
	//start the main renderpass.
	depthClear.depthStencil = { 1.0f, _portalManager.getWorldIndex()};//info의 max깊이값을 1.0으로 해놔서 이걸로 초기화
  	clearValues[1] = depthClear;
	VkRenderPassBeginInfo rpInfo = {};
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.pNext = nullptr;

	rpInfo.renderPass = _renderPass;
	rpInfo.renderArea.offset.x = 0;
	rpInfo.renderArea.offset.y = 0;
	rpInfo.renderArea.extent = _windowExtent;
	rpInfo.framebuffer = _framebuffers[swapchainImageIndex];

	//connect clear values
	rpInfo.clearValueCount = 2;
	rpInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
	pushConstants.proj = _camera.getProjection();
	pushConstants.proj[1][1] *= -1;
	for (const RenderObject& draw : mainDrawContext.OpaqueSurfaces)
	{
		pushConstants.view = _camera._view;
		VkDeviceSize offset = 0;
		vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipeline);
		vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.material->pipeline->layout, 0,1, &globalDescriptor,0,nullptr );
		vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.material->pipeline->layout, 1,1, &draw.material->materialSet,0,nullptr );
		vkCmdBindIndexBuffer(cmd, draw.indexBuffer,0,VK_INDEX_TYPE_UINT32);
		vkCmdBindVertexBuffers(cmd, 0, 1, &draw.vertexBuffer, &offset);

		pushConstants.worldMatrix = draw.transform;
		pushConstants.normal = glm::inverseTranspose(pushConstants.view * draw.transform);
		vkCmdPushConstants(cmd, draw.material->pipeline->layout ,VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,0, sizeof(GPUDrawPushConstants), &pushConstants);
		if (draw.material->passType == MaterialPass::ParticleRender){
			vkCmdDrawIndexed(cmd, draw.indexCount,_particle->PARTICLE_COUNT,draw.firstIndex,0,0);
		} else {
			vkCmdDrawIndexed(cmd, draw.indexCount,1,draw.firstIndex,0,0);
		}

	}
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

  vkCmdEndRenderPass(cmd);

	//mirror
	// for (const RenderObject& draw : mainDrawContext.OpaqueSurfaces)
	// {
	// 	if (draw.material->passType != MaterialPass::SkyBoxInStencil) continue;
	// 	VkDeviceSize offset = 0;
	// 	vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipeline);
	// 	vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.material->pipeline->layout, 0,1, &globalDescriptor,0,nullptr );
	// 	vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,draw.material->pipeline->layout, 1,1, &draw.material->materialSet,0,nullptr );
	// 	vkCmdBindIndexBuffer(cmd, draw.indexBuffer,0,VK_INDEX_TYPE_UINT32);
	// 	vkCmdBindVertexBuffers(cmd, 0, 1, &draw.vertexBuffer, &offset);
		
	// 	GPUDrawPushConstants pushConstants;
	// 	pushConstants.worldMatrix = draw.transform;
	// 	pushConstants.view = _camera.getMirrorView(glm::vec3(0,0,1), glm::vec3(draw.transform * glm::vec4(0,0,0,1)));
	// 	pushConstants.proj = _camera.getProjection();
	// 	pushConstants.proj[1][1] *= -1;
	// 	vkCmdPushConstants(cmd, draw.material->pipeline->layout ,VK_SHADER_STAGE_VERTEX_BIT,0, sizeof(GPUDrawPushConstants), &pushConstants);
	// 	vkCmdDrawIndexed(cmd, draw.indexCount,1,draw.firstIndex,0,0);
	// }
}

AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	//allocate vertex buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;

	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;


	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;
	vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	AllocatedBuffer newBuffer;

	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo,
		&newBuffer.buffer,
		&newBuffer.allocation,
		&newBuffer.info));

	return newBuffer;
}

void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VkCommandBuffer cmd = _uploadContext._commandBuffer;
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));
	VkSubmitInfo submit = vkinit::submit_info(&cmd);

	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _uploadContext._uploadFence));

	vkWaitForFences(_device, 1, &_uploadContext._uploadFence, true, 9999999999);
	vkResetFences(_device, 1, &_uploadContext._uploadFence);
	vkResetCommandPool(_device, _uploadContext._commandPool, 0);
}

void VulkanEngine::load_images()
{

}

void VulkanEngine::init_imgui()
{
	//1: create descriptor pool for IMGUI
	// the size of the pool is very oversize, but it's copied from imgui demo itself.
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 40 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 40 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 40 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 40 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 40 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 40 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 40 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 40 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 40 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 40 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 40 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 40;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;

	VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));


	// 2: initialize imgui library

	//this initializes the core structures of imgui
	ImGui::CreateContext();
	//this initializes imgui for SDL
	ImGui_ImplSDL2_InitForVulkan(_window);

	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = _instance;
	init_info.PhysicalDevice = _chosenGPU;
	init_info.Device = _device;
	init_info.Queue = _graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info, _renderPass);

	//execute a gpu command to upload imgui font textures
	immediate_submit([&](VkCommandBuffer cmd) {
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
		});

	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	//add the destroy the imgui created structures
	_mainDeletionQueue.push_function([=]() {

		vkDestroyDescriptorPool(_device, imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
		});
}

AllocatedImage VulkanEngine::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	AllocatedImage newImage;
	newImage._imageFormat = format;
	newImage._imageExtent = size;

	VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
	if (mipmapped) {
		img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	VK_CHECK(vmaCreateImage(_allocator, &img_info, &allocinfo, &newImage._image, &newImage._allocation, nullptr));

	// if the format is a depth format, we will need to have it use the correct
	// aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build a image-view for the image
	VkImageViewCreateInfo view_info = vkinit::imageview_create_info(format, newImage, aspectFlag);
	view_info.subresourceRange.levelCount = img_info.mipLevels;

	VK_CHECK(vkCreateImageView(_device, &view_info, nullptr, &newImage._imageView));

	return newImage;
}

AllocatedImage VulkanEngine::create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
{
	size_t data_size = size.depth * size.width * size.height * 4;
	AllocatedBuffer uploadbuffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	memcpy(uploadbuffer.info.pMappedData, data, data_size);
	vmaFlushAllocation(_allocator, uploadbuffer.allocation, 0, VK_WHOLE_SIZE);
	
	AllocatedImage new_image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

	immediate_submit([&](VkCommandBuffer cmd) {
		vkutil::transitionImageLayout(cmd, new_image._image, new_image._imageFormat,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT));

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
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT));
		});
	destroy_buffer(uploadbuffer);
	return new_image;
}

AllocatedImage VulkanEngine::createCubeImage(ktxTexture* ktxTexture, VkFormat format, VkSampler* sampler)
{
	ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
	ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);
	AllocatedBuffer uploadbuffer = create_buffer(ktxTextureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	memcpy(uploadbuffer.info.pMappedData, ktxTextureData, ktxTextureSize);
	vmaFlushAllocation(_allocator, uploadbuffer.allocation, 0, VK_WHOLE_SIZE);

	AllocatedImage newImage;
	newImage._imageFormat = format;
	newImage._imageExtent = { ktxTexture->baseWidth, ktxTexture->baseHeight, 1 };

	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkImageCreateInfo img_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	img_info.imageType = VK_IMAGE_TYPE_2D;
	img_info.format = format;
	img_info.mipLevels = ktxTexture->numLevels;
	img_info.samples = VK_SAMPLE_COUNT_1_BIT;
	img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	img_info.extent = { ktxTexture->baseWidth, ktxTexture->baseHeight, 1 };
	img_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	img_info.arrayLayers = 6;
	img_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VK_CHECK(vmaCreateImage(_allocator, &img_info, &allocinfo, &newImage._image, &newImage._allocation, nullptr));

	VkImageViewCreateInfo imageViewInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	imageViewInfo.format = format;
	imageViewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	imageViewInfo.subresourceRange.layerCount = 6;
	imageViewInfo.subresourceRange.levelCount = ktxTexture->numLevels;
	imageViewInfo.image = newImage._image;

	VK_CHECK(vkCreateImageView(_device, &imageViewInfo, nullptr, &newImage._imageView));

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = ktxTexture->numLevels;
	subresourceRange.layerCount = 6;

	std::vector<VkBufferImageCopy> bufferCopyRegions;
	uint32_t offset = 0;
	for (uint32_t face = 0; face < 6; face++)
	{
		for (uint32_t level = 0; level < ktxTexture->numLevels; level++)
		{
			// Calculate offset into staging buffer for the current mip level and face
			ktx_size_t offset;
			KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);
			assert(ret == KTX_SUCCESS);
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = level;
			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> level;
			bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> level;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;
			bufferCopyRegions.push_back(bufferCopyRegion);
		}
	}

	immediate_submit([&](VkCommandBuffer cmd) {
		vkutil::transitionImageLayout(cmd, newImage._image, newImage._imageFormat,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

		// copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, newImage._image, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		static_cast<uint32_t>(bufferCopyRegions.size()),
		bufferCopyRegions.data());

		vkutil::transitionImageLayout(cmd, newImage._image,newImage._imageFormat, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			subresourceRange);
	});
	destroy_buffer(uploadbuffer);

	VkSamplerCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.maxAnisotropy = 1.0f;
	info.magFilter = VK_FILTER_LINEAR;
	info.minFilter = VK_FILTER_LINEAR;
	info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.addressModeV = info.addressModeU;
	info.addressModeW = info.addressModeU;
	info.mipLodBias = 0.0f;
	info.compareOp = VK_COMPARE_OP_NEVER;
	info.minLod = 0.0f;
	info.maxLod = static_cast<float>(ktxTexture->numLevels);
	info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	info.maxAnisotropy = 1.0f;

	VK_CHECK(vkCreateSampler(_device, &info, nullptr, sampler));
	_mainDeletionQueue.push_function([=]() {
		vkDestroySampler(_device, *sampler, nullptr);
  });

	return newImage;
}

void VulkanEngine::destroy_buffer(const AllocatedBuffer& buffer)
{
	vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
}

void VulkanEngine::update_scene()
{
	static auto start = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	Camera& _camera = Camera::getInstance();
	mainDrawContext.OpaqueSurfaces.clear();
	mainDrawContext.EnvSurfaces.clear();
	mainDrawContext.computeObj.clear();

	// _portalManager.update();
	glm::mat4 s = glm::translate(glm::vec3(0, 40, 0 )) *glm::rotate(-80.0f, glm::vec3(1,0,0))*glm::scale(glm::mat4(1.0f), glm::vec3(250.0f));
	glm::mat4 sandTransForm = glm::translate(glm::vec3(0, -40, -0 )) * glm::scale(glm::mat4(1.0f), glm::vec3(200.0f));
	glm::mat4 sprTransForm = glm::translate(glm::vec3(0, 40, -100 )) * glm::scale(glm::mat4(1.0f), glm::vec3(20.0f));
	glm::mat4 sandTransForm1 = glm::translate(glm::vec3(0, -40, -20 )) * glm::scale(glm::mat4(1.0f), glm::vec3(20.0f));
	// switch (_portalManager.getPortalState())
	// {
	// case PortalState::In_World0:
	// 	loadedScenes["StencilFill_One"]->Draw(_portalManager.getModelTransForm(), mainDrawContext);
	// 	break;
	// case PortalState::In_World1:
	// 	loadedScenes["StencilFill_Zero"]->Draw(_portalManager.getModelTransForm(), mainDrawContext);
	// 	// loadedScenes["julia"]->Draw(glm::scale(glm::mat4(1.0f), glm::vec3(40.0f)) ,mainDrawContext);
	// 	break;
	// default:
	// 	break;
	// }
	// loadedScenes["World1_SkyBox"]->Draw(s, mainDrawContext);
	// loadedScenes["vulkanmodels"]->Draw(sprTransForm ,mainDrawContext);
	// loadedScenes["vulkanscenelogos"]->Draw(sprTransForm ,mainDrawContext);
	std::random_device _rd;
	_particle->constant.cursor = _camera.getWorldCursorPos(_windowExtent.width, _windowExtent.height);
	if (_camera._clickOn){
		_particle->constant.cursor.w = 1;
	} else {
		_particle->constant.cursor.w = 0;
	}
	_particle->constant.seed = _rd();
	std::chrono::duration<float> elapsed = now - start;
	sceneData.viewPos = glm::vec4(_camera._cameraPos,1);
	static int i =0;
	int t = (int)(std::ceil(elapsed.count()));
	if (t >= i)
	{
		i++;
	}
	mainDrawContext.computeObj.push_back(loadedComputeObj["particleComp"].get());
	loadedScenes["particleCube"]->Draw(glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)), mainDrawContext);
		_cloud->update(1.0f / 720.0f);
		mainDrawContext.computeObj.push_back(loadedComputeObj["cloudDensity"].get());
		mainDrawContext.computeObj.push_back(loadedComputeObj["cloudLighting"].get());
		loadedScenes["envoff"]->Draw(glm::mat4(1.0f), mainDrawContext);
		loadedScenes["sand"]->Draw(glm::mat4(1.0f) * sandTransForm, mainDrawContext); // cloudCube
	// loadedScenes["cloudCube"]->Draw(sandTransForm1, mainDrawContext);

	// sceneData.viewPos = _camera._view * glm::vec4(_camera._cameraPos, 1.0f);
	sceneData.waterData.x = elapsed.count();
	sceneData.cloudData.x = _cloud->constants.densityAbsorption;
	sceneData.cloudData.y = _cloud->constants.aniso;
}

void VulkanEngine::destroy_image(const AllocatedImage& img)
{
  vkDestroyImageView(_device, img._imageView, nullptr);
  vmaDestroyImage(_allocator, img._image, img._allocation);
}
