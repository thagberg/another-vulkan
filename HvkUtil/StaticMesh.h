#pragma once

#include <memory>

#include "HvkUtil.h"

namespace hvk {

	class StaticMesh
	{
	public:
		using Vertices = HVK_shared<HVK_vector<Vertex>>;
		using Indices = HVK_shared<HVK_vector<VertIndex>>;

	private:
		Vertices mVertices;
		Indices mIndices;
		HVK_shared<Material> mMaterial;

	public:
		StaticMesh(Vertices vertices, Indices indices, HVK_shared<Material> material);
		StaticMesh(const StaticMesh& rhs);
		~StaticMesh();

		Vertices getVertices() { return mVertices; }
		Indices getIndices() { return mIndices; }
		HVK_shared<Material> getMaterial() { return mMaterial; }

		const Vertices getVertices() const { return mVertices; }
		const Indices getIndices() const { return mIndices; }
		const HVK_shared<Material> getMaterial() const { return mMaterial; }
		bool isUsingSRGBMat() { return mMaterial->sRGB; }
		void setUsingSRGMat(bool usingSRGB) { mMaterial->sRGB = usingSRGB; }
	};
}
