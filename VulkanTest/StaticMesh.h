#pragma once

#include <vector>

#include "tiny_gltf.h"
#include "types.h"

namespace hvk {

	class StaticMesh
	{
	private:
		std::vector<Vertex>& mVertices;
		std::vector<VertIndex>& mIndices;
		Material& mMaterial;

	public:
		StaticMesh(std::vector<Vertex>& vertices, std::vector<VertIndex>& indices, Material& material);
		~StaticMesh();

		const std::vector<Vertex>& getVertices() const { return mVertices; }
		const std::vector<VertIndex>& getIndices() const { return mIndices; }
		Material& getMaterial() const { return mMaterial; }
	};
}
