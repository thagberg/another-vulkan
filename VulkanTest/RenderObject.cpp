#include "stdafx.h"
#include "RenderObject.h"

#include <glm/gtc/type_ptr.hpp>

namespace hvk {

	RenderObject::RenderObject(NodeRef parent, glm::mat4 transform) :
        Node(parent, transform)
	{
	}

	RenderObject::~RenderObject()
	{
	}

	StaticMeshRenderObject::StaticMeshRenderObject(NodeRef parent, glm::mat4 transform, std::shared_ptr<StaticMesh> mesh) :
		RenderObject(parent, transform),
		mMesh(mesh)
	{

	}

	StaticMeshRenderObject::~StaticMeshRenderObject() 
	{

	}

	const std::vector<Vertex>& StaticMeshRenderObject::getVertices() const
	{
		return mMesh->getVertices();
	}

	const std::vector<VertIndex>& StaticMeshRenderObject::getIndices() const
	{
		return mMesh->getIndices();
	}

	Material& StaticMeshRenderObject::getMaterial() const
	{
		return mMesh->getMaterial();
	}
}
