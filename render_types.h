#pragma once

#include "vk_mem_alloc.h"

struct AllocatedBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
};