#pragma once

#include "HvkUtil.h"
#include "StaticMesh.h"

namespace hvk
{
	extern tinygltf::TinyGLTF ModelLoader;
	std::unique_ptr<StaticMesh, void(*)(StaticMesh*)> createMeshFromGltf(const std::string& filename);
}
