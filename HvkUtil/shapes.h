#pragma once

#include <array>
#include "HvkUtil.h"
#include "DebugMesh.h"

namespace hvk
{

	HVK_shared<DebugMesh> createColoredCube(glm::vec3&& color);
}
