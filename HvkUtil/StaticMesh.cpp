#include "pch.h"
#include "StaticMesh.h"


namespace hvk {

	StaticMesh::StaticMesh(Vertices vertices, Indices indices, Material material) :
		mVertices(vertices),
		mIndices(indices),
		mMaterial(material)
	{
	}

	StaticMesh::StaticMesh(Vertices vertices, Indices indices) :
		mVertices(vertices),
		mIndices(indices),
		mMaterial()
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
