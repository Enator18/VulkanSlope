#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <deque>
#include <functional>
#include <array>

#include "vk_mem_alloc.h"

struct AllocatedBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct AllocatedImage
{
	VkImage image;
	VmaAllocation allocation;
};

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function)
	{
		deletors.push_back(function);
	}

	void flush()
	{
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
		{
			(*it)();
		}

		deletors.clear();
	}
};

struct VertexInputDescription
{
	VkVertexInputBindingDescription bindingDescription;
	std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions;
};

struct FrameData
{
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	
	VkSemaphore presentSemaphore, renderSemaphore;
	VkFence renderFence;

	AllocatedBuffer cameraBuffer;
	AllocatedBuffer instanceBuffer;

	VkDescriptorSet globalDescriptor;
};

struct Transform
{
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec3 rotation;
	glm::vec3 scale = glm::vec3(1, 1, 1);

	glm::mat4 getTransformMatrix()
	{
		return glm::translate(glm::scale(glm::mat4_cast(glm::quat(rotation)), scale), position);
	}
};

struct Camera
{
	glm::mat4 view;
	glm::mat4 proj;
};