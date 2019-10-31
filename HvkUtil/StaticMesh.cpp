#include "pch.h"
#include "StaticMesh.h"


namespace hvk {

	StaticMesh::StaticMesh(Vertices vertices, Indices indices, HVK_shared<Material> material) :
		mVertices(vertices),
		mIndices(indices),
		mMaterial(material)
	{
	}

	StaticMesh::StaticMesh(const StaticMesh& rhs)  :
		mVertices(rhs.getVertices()),
		mIndices(rhs.getIndices()),
		mMaterial(rhs.getMaterial())
	{
	}

	StaticMesh::~StaticMesh() = default;
}
