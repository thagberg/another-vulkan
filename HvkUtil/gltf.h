#pragma once

#include "HvkUtil.h"
#include "StaticMesh.h"

namespace hvk
{
	extern tinygltf::TinyGLTF ModelLoader;
	HVK_unique<StaticMesh> createMeshFromGltf(const std::string& filename);
}
