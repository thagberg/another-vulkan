#pragma once

#include <array>
#include "HvkUtil.h"

namespace hvk
{
	class StaticMesh;
	class DebugMesh;
	class CubeMesh;

	std::shared_ptr<DebugMesh> createColoredCube(glm::vec3&& color);

    std::shared_ptr<CubeMesh> createEnvironmentCube();

	//std::shared_ptr<StaticMesh> createStaticCube();
}
