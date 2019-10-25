#include "DebugMesh.h"


namespace hvk
{
	DebugMesh::DebugMesh(ColorVertices vertices, Indices indices) :
		mVertices(vertices),
		mIndices(indices)
	{
	}


	DebugMesh::~DebugMesh()
	{
	}
}
