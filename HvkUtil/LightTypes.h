#pragma once

#include "entt/entt.hpp"
#include "glm/glm.hpp"

#include "types.h"

namespace hvk
{
	struct LightColor
	{
		glm::vec3 color;
		float intensity;
	};

	struct LightBinding
	{
		RuntimeResource<VkBuffer> ubo;
	};
}
