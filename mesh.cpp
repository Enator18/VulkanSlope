#include <iostream>
#include "mesh.h"
#include "vk_mem_alloc.h"
#include "render_types.h"

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

void Mesh::upload(VmaAllocator allocator, DeletionQueue* deletionQueue)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = vertices.size() * sizeof(Vertex);
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &vertexBuffer.buffer, &vertexBuffer.allocation, nullptr));

	deletionQueue->push_function([=]()
	{
		vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.allocation);
	});

	void* data;
	vmaMapMemory(allocator, vertexBuffer.allocation, &data);

	memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));

	vmaUnmapMemory(allocator, vertexBuffer.allocation);
}