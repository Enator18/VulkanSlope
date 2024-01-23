#pragma once
#include <vulkan/vulkan.h>
#include <vector>

#include "vk_mem_alloc.h"
#include "render_types.h"

AllocatedImage createImage(VmaAllocator allocator, VkDevice, VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, VmaMemoryUsage memUsage, VkMemoryPropertyFlags memFlags);
VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
uint32_t findMemoryType(VkPhysicalDeviceMemoryProperties memProperties, uint32_t typeFilter, VkMemoryPropertyFlags properties);
VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer);
VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);

VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo(bool pDepthTest, bool pDepthWrite, VkCompareOp compareOp);

AllocatedBuffer createBuffer(VmaAllocator allocator, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags allocFlags, VkMemoryPropertyFlags requiredFlags);