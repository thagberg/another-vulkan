#pragma once

#include "types.h"

namespace hvk
{
	struct DebugDrawMesh
	{
		RuntimeResource<VkBuffer> vbo;
		RuntimeResource<VkBuffer> ibo;
		uint32_t numIndices;
	};

	struct DebugDrawBinding
	{
		RuntimeResource<VkBuffer> ubo;
		VkDescriptorSet descriptorSet;
	};
}
