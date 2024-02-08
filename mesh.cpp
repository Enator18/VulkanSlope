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

void Mesh::upload(VmaAllocator allocator)
{
	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.size = vertices.size() * sizeof(Vertex);
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VK_CHECK(vmaCreateBuffer(allocator, &vertexBufferInfo, &vmaAllocInfo, &vertexBuffer.buffer, &vertexBuffer.allocation, nullptr));

	void* vertexData;
	vmaMapMemory(allocator, vertexBuffer.allocation, &vertexData);

	memcpy(vertexData, vertices.data(), vertices.size() * sizeof(Vertex));

	vmaUnmapMemory(allocator, vertexBuffer.allocation);

	VkBufferCreateInfo indexBufferInfo = {};
	indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	indexBufferInfo.size = indices.size() * sizeof(uint32_t);
	indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	VK_CHECK(vmaCreateBuffer(allocator, &indexBufferInfo, &vmaAllocInfo, &indexBuffer.buffer, &indexBuffer.allocation, nullptr));

	void* indexData;
	vmaMapMemory(allocator, indexBuffer.allocation, &indexData);

	memcpy(indexData, indices.data(), indices.size() * sizeof(uint32_t));

	vmaUnmapMemory(allocator, indexBuffer.allocation);

	/*deletionQueue->push_function([&, allocator]()
	{
		vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.allocation);
		vmaDestroyBuffer(allocator, indexBuffer.buffer, indexBuffer.allocation);
	});*/
}