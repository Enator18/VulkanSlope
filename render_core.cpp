#define VMA_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <array>
#include <memory>

#include "render_core.h"
#include "render_utils.h"
#include "VkBootstrap.h"
#include "vk_mem_alloc.h"
#include "pipeline_builder.h"
#include "mesh.h"
#include "entity.h"

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

//Initialize base renderer structures
void Renderer::init(vkb::Instance vkbInstance, VkSurfaceKHR* surface, uint32_t width, uint32_t height)
{
	//Create vulkan instance and window surface
	instance = vkbInstance.instance;
	this->surface = surface;

	messenger = vkbInstance.debug_messenger;
	
	//Initialize the GPU device
	vkb::PhysicalDeviceSelector selector{ vkbInstance };

	VkPhysicalDeviceVulkan13Features features13 =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.synchronization2 = true,
	};

	auto devRet = selector.set_surface(*surface)
		.set_minimum_version(1, 3)
		.prefer_gpu_device_type()
		.add_required_extension("VK_KHR_shader_draw_parameters")
		.set_required_features_13(features13)
		.select();
	physicalDevice = devRet.value();

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	device = deviceBuilder.build().value();

	graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();

	//Create the memory allocator
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = device;
	allocatorInfo.instance = instance;
	vmaCreateAllocator(&allocatorInfo, &allocator);

	//Init functions for all sub-sections of the renderer
	createSwapchain(width, height);
	initCommands();
	initRenderpass();
	initFramebuffers();
	initSyncStructures();
	initDescriptors();
	
	//Create render pipeline
	VertexInputDescription inputDescription = Vertex::getInputDescription();

	std::vector<VkDescriptorSetLayout> setLayouts = {globalSetLayout, textureSetLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 2;
	pipelineLayoutInfo.pSetLayouts = &setLayouts[0];
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	renderPipeline = buildRenderPipeline(device, renderPass, width, height, pipelineLayout, inputDescription);

	//Create Error Texture
	uint32_t black = 0xFF000000;
	uint32_t magenta = 0xFFFF00FF;
	std::array<uint32_t, 16 * 16> pixels;
	for (int x = 0; x < 16; x++)
	{
		for (int y = 0; y < 16; y++)
		{
			pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}

	errorTexture = createImage(pixels.data(), allocator, device, mainCommandPool, graphicsQueue, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VkExtent3D{16, 16, 1}, VMA_MEMORY_USAGE_GPU_ONLY, 0);

	VkSamplerCreateInfo samplerInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	vkCreateSampler(device, &samplerInfo, nullptr, &defaultSampler);

	errorTexView = createImageView(device, errorTexture.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

	mainDeletionQueue.push_function([=]() {
		vmaDestroyImage(allocator, errorTexture.image, errorTexture.allocation);
		vkDestroySampler(device, defaultSampler, nullptr);
		vkDestroyImageView(device, errorTexView, nullptr);
		});
};

//Create swapchain and depth image and fetch swapchain images and image views
void Renderer::createSwapchain(uint32_t width, uint32_t height)
{
	this->width = width;
	this->height = height;

	VkExtent3D depthImageExtent =
	{
		width,
		height,
		1
	};

	depthFormat = VK_FORMAT_D32_SFLOAT;

	depthImage = createImage(allocator, device, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent, VMA_MEMORY_USAGE_GPU_ONLY, VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	VkImageViewCreateInfo depthViewInfo = imageViewCreateInfo(depthFormat, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

	VK_CHECK(vkCreateImageView(device, &depthViewInfo, nullptr, &depthImageView));

	vkb::SwapchainBuilder swapchainBuilder{ physicalDevice, device, *surface };

	swapchain = swapchainBuilder
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
		.set_desired_extent(width, height)
		.build()
		.value();

	swapchainImages = swapchain.get_images().value();
	swapchainImageViews = swapchain.get_image_views().value(); 

	swapchainImageFormat = swapchain.image_format;
};

//Callback function for when the window is resized
void Renderer::onResized(uint32_t width, uint32_t height)
{
	this->width = width;
	this->height = height;
	resized = true;
}

//Delete swapchain
void Renderer::cleanupSwapchain()
{
	vkDestroyImageView(device, depthImageView, nullptr);
	vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
	
	for (VkFramebuffer framebuffer : framebuffers)
	{
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	for (VkImageView imageView : swapchainImageViews)
	{
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain, nullptr);
}

//Delete and recreate the swapchain for when the window is resized. If minimized, wait.
void Renderer::updateSwapchain()
{
	if (width == 0 || height == 0)
	{
		return;
	}
	vkDeviceWaitIdle(device);

	cleanupSwapchain();

	createSwapchain(width, height);
	initFramebuffers();
}

//Create command buffer and command pool
void Renderer::initCommands()
{
	VkCommandPoolCreateInfo commandPoolInfo = {};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.queueFamilyIndex = graphicsQueueFamily;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &mainCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool");
	}

	mainDeletionQueue.push_function([=]()
		{
			vkDestroyCommandPool(device, mainCommandPool, nullptr);
		});

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].commandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command pool");
		}

		VkCommandBufferAllocateInfo cmdAllocInfo = {};
		cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAllocInfo.pNext = nullptr;
		cmdAllocInfo.commandPool = frames[i].commandPool;
		cmdAllocInfo.commandBufferCount = 1;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		if (vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command pool");
		}

		mainDeletionQueue.push_function([=]()
			{
				vkDestroyCommandPool(device, frames[i].commandPool, nullptr);
			});
	}
}

//Create renderpass
void Renderer::initRenderpass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = &attachments[0];
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass");
	}
}

//Create the framebuffers and attach the swapchain and depth image views
void Renderer::initFramebuffers()
{
	VkFramebufferCreateInfo fbInfo = {};
	fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbInfo.pNext = nullptr;
	fbInfo.renderPass = renderPass;
	fbInfo.attachmentCount = 1;
	fbInfo.width = width;
	fbInfo.height = height;
	fbInfo.layers = 1;

	const uint32_t swapchainImageCount = swapchainImages.size();
	framebuffers = std::vector<VkFramebuffer>(swapchainImageCount);

	for (int i = 0; i < swapchainImageCount; i++)
	{
		VkImageView attachments[2];
		attachments[0] = swapchainImageViews[i];
		attachments[1] = depthImageView;

		fbInfo.pAttachments = attachments;
		fbInfo.attachmentCount = 2;
		if (vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer");
		}
	}
}

//Create the fences and semaphores
void Renderer::initSyncStructures()
{
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		if (vkCreateFence(device, &fenceCreateInfo, nullptr, &frames[i].renderFence) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create fence");
		}

		if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].renderSemaphore) != VK_SUCCESS || vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &frames[i].presentSemaphore) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create semaphores");
		}

		mainDeletionQueue.push_function([=]()
			{
				vkDestroyFence(device, frames[i].renderFence, nullptr);
				vkDestroySemaphore(device, frames[i].renderSemaphore, nullptr);
				vkDestroySemaphore(device, frames[i].presentSemaphore, nullptr);
			});
	}
}

//Create the camera buffer and instance buffer and set up their descriptor set layout
void Renderer::initDescriptors()
{
	//Create buffers
	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		frames[i].cameraBuffer = createBuffer(allocator, sizeof(Camera), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		frames[i].instanceBuffer = createBuffer(allocator, sizeof(glm::mat4) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		mainDeletionQueue.push_function([&, i]()
			{
				vmaDestroyBuffer(allocator, frames[i].cameraBuffer.buffer, frames[i].cameraBuffer.allocation);
				vmaDestroyBuffer(allocator, frames[i].instanceBuffer.buffer, frames[i].instanceBuffer.allocation);
			});
	}

	//Create bindings
	VkDescriptorSetLayoutBinding cameraBufferBinding = {};
	cameraBufferBinding.binding = 0;
	cameraBufferBinding.descriptorCount = 1;
	cameraBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	cameraBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding instanceBufferBinding = {};
	instanceBufferBinding.binding = 1;
	instanceBufferBinding.descriptorCount = 1;
	instanceBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

	instanceBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding bindings[] = {cameraBufferBinding, instanceBufferBinding};

	VkDescriptorSetLayoutBinding textureBinding = {};
	textureBinding.binding = 0;
	textureBinding.descriptorCount = 1;
	textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//Create descriptor set layout
	VkDescriptorSetLayoutCreateInfo setInfo = {};
	setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setInfo.pNext = nullptr;

	setInfo.bindingCount = 2;
	setInfo.flags = 0;
	setInfo.pBindings = &bindings[0];

	VkDescriptorSetLayoutCreateInfo texSetInfo = {};
	texSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	texSetInfo.pNext = nullptr;

	texSetInfo.bindingCount = 1;
	texSetInfo.flags = 0;
	texSetInfo.pBindings = &textureBinding;

	vkCreateDescriptorSetLayout(device, &setInfo, nullptr, &globalSetLayout);
	vkCreateDescriptorSetLayout(device, &texSetInfo, nullptr, &textureSetLayout);

	mainDeletionQueue.push_function([&]()
		{
			vkDestroyDescriptorSetLayout(device, globalSetLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, textureSetLayout, nullptr);
		});

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		std::vector<DescriptorAllocator::PoolSizeRatio> frameSizes =
		{
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
		};

		frames[i].descriptorAllocator = DescriptorAllocator{};
		frames[i].descriptorAllocator.init(device, 1000, frameSizes);

		mainDeletionQueue.push_function([&, i]()
		{
			frames[i].descriptorAllocator.destroyPools(device);
		});
	}
}

void Renderer::uploadMesh(Mesh& mesh)
{
	mesh.upload(allocator);
}

void Renderer::deleteMesh(Mesh& mesh)
{
	vkDeviceWaitIdle(device);
	vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
	vmaDestroyBuffer(allocator, mesh.indexBuffer.buffer, mesh.indexBuffer.allocation);
}

TextureImage Renderer::uploadTexture(std::vector<uint32_t> pixels, uint32_t width, uint32_t height)
{
	AllocatedImage texture = createImage(pixels.data(), allocator, device, mainCommandPool, graphicsQueue, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VkExtent3D{ width, height, 1 }, VMA_MEMORY_USAGE_GPU_ONLY, 0);

	VkImageView textureView = createImageView(device, texture.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

	TextureImage textureImage = { texture, textureView };

	return textureImage;
}

void Renderer::deleteTexture(TextureImage& texture)
{
	vkDestroyImageView(device, texture.textureView, nullptr);
	vmaDestroyImage(allocator, texture.texture.image, texture.texture.allocation);
}

//Main draw function. Called every frame.
void Renderer::drawFrame(std::vector<std::unique_ptr<Entity>>& scene, Camera camera)
{
	//Set up commands
	VkCommandBuffer& commandBuffer = getCurrentFrame().commandBuffer;

	//Synchronize and get images
	VK_CHECK(vkWaitForFences(device, 1, &getCurrentFrame().renderFence, true, 1000000000));

	getCurrentFrame().descriptorAllocator.clearPools(device);

	uint32_t swapchainImageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapchain, 1000000000, getCurrentFrame().presentSemaphore, nullptr, &swapchainImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		updateSwapchain();
		resized = false;
		return;
	}

	VK_CHECK(vkResetFences(device, 1, &getCurrentFrame().renderFence));

	VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));

	//Begin commands and renderpass
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffers[swapchainImageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapchain.extent;

	VkClearValue clearValue;
	clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	VkClearValue depthClear;
	depthClear.depthStencil.depth = 1.f;

	VkClearValue clearValues[] = { clearValue, depthClear };

	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = &clearValues[0];

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	//Upload camera and instance matrices to the GPU
	void* cameraData;
	vmaMapMemory(allocator, getCurrentFrame().cameraBuffer.allocation, &cameraData);
	memcpy(cameraData, &camera, sizeof(Camera));
	vmaUnmapMemory(allocator, getCurrentFrame().cameraBuffer.allocation);

	std::vector<glm::mat4> transforms;

	for (std::unique_ptr<Entity> &entity : scene)
	{
		transforms.push_back(entity->transform.getTransformMatrix());
	}

	void* instanceData;
	vmaMapMemory(allocator, getCurrentFrame().instanceBuffer.allocation, &instanceData);
	memcpy(instanceData, transforms.data(), sizeof(glm::mat4) * transforms.size());
	vmaUnmapMemory(allocator, getCurrentFrame().instanceBuffer.allocation);

	//Create descriptor set
	VkDescriptorSet globalDescriptor = getCurrentFrame().descriptorAllocator.allocate(device, globalSetLayout);

	DescriptorWriter writer = DescriptorWriter{};
	writer.writeBuffer(0, getCurrentFrame().cameraBuffer.buffer, sizeof(Camera), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.writeBuffer(1, getCurrentFrame().instanceBuffer.buffer, sizeof(MeshInstance) * MAX_OBJECTS, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	writer.updateSet(device, globalDescriptor);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &globalDescriptor, 0, nullptr);

	//Set up window settings
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(width);
	viewport.height = static_cast<float>(height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent.width = width;
	scissor.extent.height = height;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPipeline);

	//Main draw loop
	for (int i = 0; i < scene.size(); i++)
	{
		MeshInstance& instance = scene[i]->mesh;

		VkDescriptorSet texDescriptor = getCurrentFrame().descriptorAllocator.allocate(device, textureSetLayout);

		DescriptorWriter texWriter = DescriptorWriter{};
		texWriter.writeImage(0, instance.texture->textureView, defaultSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		texWriter.updateSet(device, texDescriptor);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &texDescriptor, 0, nullptr);

		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &instance.mesh->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, instance.mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, instance.mesh->indices.size(), 1, 0, 0, i);
	}

	//End renderpass and commands
	vkCmdEndRenderPass(commandBuffer);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;

	//Submit commands
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit.pWaitDstStageMask = &waitStage;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &getCurrentFrame().presentSemaphore;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &getCurrentFrame().renderSemaphore;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &commandBuffer;

	VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, getCurrentFrame().renderFence));

	//Draw to screen
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;

	presentInfo.pSwapchains = &swapchain.swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	result = vkQueuePresentKHR(graphicsQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || resized)
	{
		updateSwapchain();
		resized = false;
	}

	frameNumber += 1;
}

//Delete everything
void Renderer::cleanup()
{
	vkDeviceWaitIdle(device);
	
	mainDeletionQueue.flush();

	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	vkDestroyPipeline(device, renderPipeline, nullptr);

	vkDestroyRenderPass(device, renderPass, nullptr);

	cleanupSwapchain();

	vmaDestroyAllocator(allocator);

	vkDestroyDevice(device, nullptr);
	vkb::destroy_debug_utils_messenger(instance, messenger);
};