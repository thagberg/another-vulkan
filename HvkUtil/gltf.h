#pragma once

#include "HvkUtil.h"
#include "StaticMesh.h"

namespace hvk
{
	extern tinygltf::TinyGLTF ModelLoader;
	std::vector<StaticMesh> createMeshFromGltf(const std::string& filename);
}
