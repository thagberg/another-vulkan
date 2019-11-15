#pragma once

#include <array>
#include "HvkUtil.h"
#include "DebugMesh.h"
#include "CubeMesh.h"

namespace hvk
{

	HVK_shared<DebugMesh> createColoredCube(glm::vec3&& color);

    HVK_shared<CubeMesh> createEnvironmentCube();
}
