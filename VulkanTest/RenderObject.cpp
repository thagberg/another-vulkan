#include "stdafx.h"
#include "RenderObject.h"

namespace hvk {

	RenderObject::RenderObject(
        NodeRef parent, 
        glm::mat4 transform,
        VerticesRef vertices,
        IndicesRef indices) : 
        
        Node(parent, transform),
        mVertices(vertices),
        mIndices(indices)
	{
	}

	RenderObject::~RenderObject()
	{
	}

	const VerticesRef RenderObject::getVertices() {
		return mVertices;
	}

	const IndicesRef RenderObject::getIndices() {
		return mIndices;
	}
}
