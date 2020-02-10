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

	struct LightAttenuation
	{
		float constant;
		float linear;
		float quadratic;
	};

	struct LightBinding
	{
		RuntimeResource<VkBuffer> ubo;
	};

    struct SpotLight
    {
        float umbra;
        float penumbra;
    };

	struct ShadowCaster
	{
		TextureMap shadowMap;
	};
}
