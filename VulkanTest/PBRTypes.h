#pragma once

#include "HvkUtil.h"
#include "types.h"

namespace hvk
{
    struct PBRMesh
    {
        RuntimeResource<VkBuffer> vbo;
        RuntimeResource<VkBuffer> ibo;
		uint32_t numIndices;
    };

    struct PBRMaterial
    {
        TextureMap albedo;
        TextureMap metallicRoughness;
        TextureMap normal;
    };

	struct PBRBinding
	{
		RuntimeResource<VkBuffer> ubo; // should we make this a Resource<T> and store the allocation info?  Adds at least 24 bytes to the field.
		VkDescriptorSet descriptorSet;
	};
}
