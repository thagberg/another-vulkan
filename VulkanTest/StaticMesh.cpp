#include "StaticMesh.h"


namespace hvk {

	StaticMesh::StaticMesh(std::vector<Vertex>& vertices, std::vector<VertIndex>& indices, Material& material) :
		mVertices(vertices),
		mIndices(indices),
		mMaterial(material)
	{
	}


	StaticMesh::~StaticMesh()
	{
	}
}
