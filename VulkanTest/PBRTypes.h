#pragma once

#include "HvkUtil.h"
#include "types.h"

namespace hvk
{
    struct PBRMesh
    {
        RuntimeResource<VkBuffer> vbo;
        RuntimeResource<VkBuffer> ibo;
    };

    struct PBRMaterial
    {
        TextureMap albedo;
        TextureMap metallicRoughness;
        TextureMap normal;
    };
}
