#pragma once
#include "render_types.h"
VkPipeline buildRenderPipeline(VkDevice device, VkRenderPass renderPass, uint32_t viewportWidth, uint32_t viewportHeight, VertexInputDescription inputDescription);