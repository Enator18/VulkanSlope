#define VMA_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <array>

#include "render_core.h"
#include "render_utils.h"
#include "VkBootstrap.h"
#include "vk_mem_alloc.h"
#include "pipeline_builder.h"
#include "mesh.h"

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

void Renderer::init(vkb::Instance vkbInstance, VkSurfaceKHR* surface, uint32_t width, uint32_t height)
{
	instance = vkbInstance.instance;
	this->surface = surface;

	messenger = vkbInstance.debug_messenger;
	
	vkb::PhysicalDeviceSelector selector{ vkbInstance };

	auto devRet = selector.set_surface(*surface)
		.set_minimum_version(1, 1)
		.prefer_gpu_device_type()
		.add_required_extension("VK_KHR_shader_draw_parameters")
		.select();
	physicalDevice = devRet.value();

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	device = deviceBuilder.build().value();

	graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = device;
	allocatorInfo.instance = instance;
	vmaCreateAllocator(&allocatorInfo, &allocator);

	createSwapchain(width, height);

	initCommands();

	initRenderpass();

	initSyncStructures();

	initDescriptors();

	VertexInputDescription inputDescription = Vertex::getInputDescription();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &globalSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	renderPipeline = buildRenderPipeline(device, renderPass, width, height, pipelineLayout, inputDescription);
};

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

	mainDeletionQueue.push_function([=]()
		{
			vkDestroyImageView(device, depthImageView, nullptr);
			vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
		});

	vkb::SwapchainBuilder swapchainBuilder{ physicalDevice, device, *surface };

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.build()
		.value();

	swapchain = vkbSwapchain.swapchain;
	swapchainImages = vkbSwapchain.get_images().value();
	swapchainImageViews = vkbSwapchain.get_image_views().value();

	swapchainImageFormat = vkbSwapchain.image_format;
};

void Renderer::cleanupSwapchain()
{
	for (VkImageView imageView : swapchainImageViews)
	{
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Renderer::updateSwapchain(uint32_t width, uint32_t height)
{
	cleanupSwapchain();
	createSwapchain(width, height);
}

void Renderer::initCommands()
{
	VkCommandPoolCreateInfo commandPoolInfo = {};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.queueFamilyIndex = graphicsQueueFamily;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

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

void Renderer::initDescriptors()
{
	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		frames[i].cameraBuffer = createBuffer(allocator, sizeof(Camera), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		frames[i].instanceBuffer = createBuffer(allocator, sizeof(glm::mat4) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		mainDeletionQueue.push_function([&]()
			{
				vmaDestroyBuffer(allocator, frames[i].cameraBuffer.buffer, frames[i].cameraBuffer.allocation);
				vmaDestroyBuffer(allocator, frames[i].instanceBuffer.buffer, frames[i].instanceBuffer.allocation);
			});
	}

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

	VkDescriptorSetLayoutCreateInfo setInfo = {};
	setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setInfo.pNext = nullptr;

	setInfo.bindingCount = 2;
	setInfo.flags = 0;
	setInfo.pBindings = &bindings[0];

	vkCreateDescriptorSetLayout(device, &setInfo, nullptr, &globalSetLayout);

	std::vector<VkDescriptorPoolSize> sizes =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, FRAME_OVERLAP },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, FRAME_OVERLAP }
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = 0;
	poolInfo.maxSets = FRAME_OVERLAP;
	poolInfo.poolSizeCount = (uint32_t)sizes.size();
	poolInfo.pPoolSizes = sizes.data();

	vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);

	mainDeletionQueue.push_function([&]()
		{
			vkDestroyDescriptorSetLayout(device, globalSetLayout, nullptr);
			vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		});

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.pNext = nullptr;
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &globalSetLayout;

		vkAllocateDescriptorSets(device, &allocInfo, &frames[i].globalDescriptor);

		VkDescriptorBufferInfo cameraBufferInfo;
		cameraBufferInfo.buffer = frames[i].cameraBuffer.buffer;
		cameraBufferInfo.offset = 0;
		cameraBufferInfo.range = sizeof(Camera);
		VkWriteDescriptorSet cameraSetWrite = {};
		cameraSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		cameraSetWrite.pNext = nullptr;
		cameraSetWrite.dstBinding = 0;
		cameraSetWrite.dstSet = frames[i].globalDescriptor;
		cameraSetWrite.descriptorCount = 1;
		cameraSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		cameraSetWrite.pBufferInfo = &cameraBufferInfo;

		VkDescriptorBufferInfo instanceBufferInfo;
		instanceBufferInfo.buffer = frames[i].instanceBuffer.buffer;
		instanceBufferInfo.offset = 0;
		instanceBufferInfo.range = sizeof(glm::mat4) * MAX_OBJECTS;
		VkWriteDescriptorSet instanceSetWrite = {};
		instanceSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		instanceSetWrite.pNext = nullptr;
		instanceSetWrite.dstBinding = 1;
		instanceSetWrite.dstSet = frames[i].globalDescriptor;
		instanceSetWrite.descriptorCount = 1;
		instanceSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		instanceSetWrite.pBufferInfo = &instanceBufferInfo;

		VkWriteDescriptorSet setWrites[] = { cameraSetWrite, instanceSetWrite };

		vkUpdateDescriptorSets(device, 2, &setWrites[0], 0, nullptr);
	}
}

void Renderer::uploadMesh(Mesh& mesh)
{
	mesh.upload(allocator, &mainDeletionQueue);
}

void Renderer::drawFrame(std::vector<MeshInstance>& instances, Camera camera)
{
	VkCommandBuffer& commandBuffer = getCurrentFrame().commandBuffer;

	VK_CHECK(vkWaitForFences(device, 1, &getCurrentFrame().renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(device, 1, &getCurrentFrame().renderFence));

	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1000000000, getCurrentFrame().presentSemaphore, nullptr, &swapchainImageIndex));

	VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));

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
	renderPassInfo.renderArea.extent = { width, height };

	VkClearValue clearValue;
	clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	VkClearValue depthClear;
	depthClear.depthStencil.depth = 1.f;

	VkClearValue clearValues[] = {clearValue, depthClear};

	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = &clearValues[0];

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	void* cameraData;
	vmaMapMemory(allocator, getCurrentFrame().cameraBuffer.allocation, &cameraData);
	memcpy(cameraData, &camera, sizeof(Camera));
	vmaUnmapMemory(allocator, getCurrentFrame().cameraBuffer.allocation);

	std::vector<glm::mat4> transforms;

	for (MeshInstance& instance : instances)
	{
		transforms.push_back(instance.transform);
	}

	void* instanceData;
	vmaMapMemory(allocator, getCurrentFrame().instanceBuffer.allocation, &instanceData);
	memcpy(instanceData, transforms.data(), sizeof(glm::mat4) * transforms.size());
	vmaUnmapMemory(allocator, getCurrentFrame().instanceBuffer.allocation);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &getCurrentFrame().globalDescriptor, 0, nullptr);

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

	//Draw Commands Here

	for (int i = 0; i < instances.size(); i++)
	{
		MeshInstance& instance = instances[i];

		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &instance.mesh->vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, instance.mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, instance.mesh->indices.size(), 1, 0, 0, i);
	}

	vkCmdEndRenderPass(commandBuffer);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit.pWaitDstStageMask = &waitStage;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &getCurrentFrame().presentSemaphore;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &getCurrentFrame().renderSemaphore;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &commandBuffer;

	VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, getCurrentFrame().renderFence));

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;

	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

	frameNumber += 1;
}

void Renderer::cleanup()
{
	vkDeviceWaitIdle(device);
	
	mainDeletionQueue.flush();

	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	vkDestroyPipeline(device, renderPipeline, nullptr);

	vkDestroyRenderPass(device, renderPass, nullptr);

	for (VkFramebuffer framebuffer : framebuffers)
	{
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	cleanupSwapchain();

	vkDestroyDevice(device, nullptr);
	vkb::destroy_debug_utils_messenger(instance, messenger);
};