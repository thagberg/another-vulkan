#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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
