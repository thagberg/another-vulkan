#include "StaticMesh.h"


namespace hvk {

	StaticMesh::StaticMesh(Vertices vertices, Indices indices, std::shared_ptr<Material> material) :
		mVertices(vertices),
		mIndices(indices),
		mMaterial(material)
	{
	}


	StaticMesh::~StaticMesh()
	{
	}
}
