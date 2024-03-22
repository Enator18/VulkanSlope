#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <functional>
#include <deque>

#include "VkBootstrap.h"
#include "vk_mem_alloc.h"
#include "mesh.h"
#include "entity.h"

constexpr unsigned int FRAME_OVERLAP = 2;
constexpr unsigned int MAX_OBJECTS = 10000;

class Renderer
{
public:
	void init(vkb::Instance vkbInstance, VkSurfaceKHR* surface, uint32_t width, uint32_t height);
	void uploadMesh(Mesh& mesh);
	void deleteMesh(Mesh& mesh);
	uint32_t uploadTexture(std::vector<uint32_t> pixels, uint32_t width, uint32_t height);
	void drawFrame(std::vector<std::unique_ptr<Entity>>& scene, Camera camera);
	void onResized(uint32_t width, uint32_t height);
	void cleanup();

private:
	uint32_t width;
	uint32_t height;
	bool resized;
	VkInstance instance;
	VkSurfaceKHR* surface;
	VkDebugUtilsMessengerEXT messenger;
	vkb::PhysicalDevice physicalDevice;
	vkb::Device device;
	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;
	vkb::Swapchain swapchain;
	VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;
	VkPipeline renderPipeline;
	VkPipelineLayout pipelineLayout;

	VkCommandPool mainCommandPool;

	VmaAllocator allocator;
	DeletionQueue mainDeletionQueue;

	VkImageView depthImageView;
	AllocatedImage depthImage;
	VkFormat depthFormat;

	VkDescriptorSetLayout globalSetLayout;
	VkDescriptorPool descriptorPool;

	FrameData frames[FRAME_OVERLAP];
	uint32_t frameNumber = 0;

	VkImageView errorTexView;
	AllocatedImage errorTexture;

	VkSampler defaultSampler;

	VkDescriptorSetLayout textureSetLayout;

	std::vector<TextureImage> textures;

	FrameData& getCurrentFrame()
	{
		return frames[frameNumber % FRAME_OVERLAP];
	}

	void createSwapchain(uint32_t width, uint32_t height);
	void updateSwapchain();
	void cleanupSwapchain();
	void initCommands();
	void initRenderpass();
	void initFramebuffers();
	void initSyncStructures();
	void initDescriptors();
};
