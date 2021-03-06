#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace hvk
{
	struct NodeTransform
	{
		glm::mat4 transform;
	};

	struct SceneNode
	{
		entt::entity parent{ entt::null };
		std::string name;
		std::vector<entt::entity> children;	// figure out how to not make this grow dynamically?
	};

	struct WorldTransform
	{
		glm::mat4 transform;
	};

	struct WorldDirty
	{

	};

	struct Direction
	{
		glm::vec3 direction;
	};

	struct Projection
	{
		glm::mat4 projection;
	};
}
