#pragma once

#include <memory>

#include "HvkUtil.h"

namespace hvk {

	class StaticMesh
	{
	public:
		using Vertices = std::vector<Vertex>;
		using Indices = std::vector<VertIndex>;

	private:
		Vertices mVertices;
		Indices mIndices;
		Material mMaterial;

	public:
		StaticMesh(Vertices vertices, Indices indices, Material material);
		StaticMesh(Vertices vertices, Indices indices);
		StaticMesh(const StaticMesh& rhs);
		~StaticMesh();

		Vertices getVertices() { return mVertices; }
		Indices getIndices() { return mIndices; }

		const Vertices& getVertices() const { return mVertices; }
		const Indices& getIndices() const { return mIndices; }
		const Material& getMaterial() const { return mMaterial; }
		bool isUsingSRGBMat() { return mMaterial.sRGB; }
		void setUsingSRGMat(bool usingSRGB) { mMaterial.sRGB = usingSRGB; }
	};
}
