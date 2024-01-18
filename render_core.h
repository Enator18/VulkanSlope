#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <functional>
#include <deque>

#include "VkBootstrap.h"
#include "vk_mem_alloc.h"
#include "mesh.h"

constexpr unsigned int FRAME_OVERLAP = 1;

class Renderer
{
public:
	void init(vkb::Instance vkbInstance, VkSurfaceKHR* surface, uint32_t width, uint32_t height);
	void updateSwapchain(uint32_t width, uint32_t height);
	void uploadMesh(Mesh& mesh);
	void drawFrame(std::vector<MeshInstance>& instances);
	void cleanup();

private:
	uint32_t width;
	uint32_t height;
	VkInstance instance;
	VkSurfaceKHR* surface;
	VkDebugUtilsMessengerEXT messenger;
	vkb::PhysicalDevice physicalDevice;
	vkb::Device device;
	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;
	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;
	VkPipeline renderPipeline;
	VkPipelineLayout pipelineLayout;

	VmaAllocator allocator;
	DeletionQueue mainDeletionQueue;

	VkImageView depthImageView;
	AllocatedImage depthImage;
	VkFormat depthFormat;

	VkDescriptorSetLayout globalSetLayout;
	VkDescriptorPool descriptorPool;

	FrameData frames[FRAME_OVERLAP];
	uint32_t frameNumber = 0;

	FrameData& getCurrentFrame()
	{
		return frames[frameNumber % FRAME_OVERLAP];
	}

	void createSwapchain(uint32_t width, uint32_t height);
	void cleanupSwapchain();
	void initCommands();
	void initRenderpass();
	void initSyncStructures();
	void initDescriptors();
};
