#pragma once

#include "entt/entt.hpp"
#include "glm/glm.hpp"

//namespace entt
//{
//	enum class entity;
//}

namespace hvk
{
	struct NodeTransform
	{
		entt::entity parent{ entt::null };
		glm::mat4 localTransform;
	};
}
