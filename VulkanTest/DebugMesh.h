#pragma once

#include <memory>
#include <vector>
#include "types.h"

namespace hvk
{
	class DebugMesh
	{
	public:
		using ColorVertices = std::shared_ptr<std::vector<ColorVertex>>;
		using Indices = std::shared_ptr<std::vector<VertIndex>>;

	private:
		ColorVertices mVertices;
		Indices mIndices;
	public:
		DebugMesh(ColorVertices vertices, Indices indices);
		const ColorVertices getVertices() const { return mVertices; }
		const Indices getIndices() const { return mIndices; }
		~DebugMesh();
	};
}
