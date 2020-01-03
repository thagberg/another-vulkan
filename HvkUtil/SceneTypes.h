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
		glm::mat4 transform;
	};

	struct SceneNode
	{
		entt::entity parent{ entt::null };
	};

	struct WorldTransform
	{
		glm::mat4 transform;
	};

	struct WorldDirty
	{

	};
}
