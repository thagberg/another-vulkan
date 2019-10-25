#pragma once

#include <memory>
#include <vector>

#include "tiny_gltf.h"
#include "types.h"
#include "ResourceManager.h"

namespace hvk {

	class StaticMesh
	{
	public:
		//using Vertices = std::shared_ptr<std::vector<Vertex>>;
		using Vertices = std::shared_ptr<std::vector<Vertex, Hallocator<Vertex>>>;
		using Indices = std::shared_ptr<std::vector<VertIndex>>;

	private:
		Vertices mVertices;
		Indices mIndices;
		std::shared_ptr<Material> mMaterial;

	public:
		StaticMesh(Vertices vertices, Indices indices, std::shared_ptr<Material> material);
		~StaticMesh();

		const Vertices getVertices() const { return mVertices; }
		const Indices getIndices() const { return mIndices; }
		const std::shared_ptr<Material> getMaterial() const { return mMaterial; }
	};
}
