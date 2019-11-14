#pragma once

#include <memory>
#include <vector>
#include "HvkUtil.h"

namespace hvk
{
	class DebugMesh
	{
	public:
		using ColorVertices = HVK_shared<HVK_vector<ColorVertex>>;
		using Indices = HVK_shared<HVK_vector<VertIndex>>;

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
