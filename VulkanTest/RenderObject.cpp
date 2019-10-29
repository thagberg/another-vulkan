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
		Node(parent, transform),
		mMesh(mesh)
	{

	}

	StaticMeshRenderObject::~StaticMeshRenderObject() 
	{

	}

	const StaticMesh::Vertices StaticMeshRenderObject::getVertices() const
	{
		return mMesh->getVertices();
	}

	const StaticMesh::Indices StaticMeshRenderObject::getIndices() const
	{
		return mMesh->getIndices();
	}

	const std::shared_ptr<Material> StaticMeshRenderObject::getMaterial() const
	{
		return mMesh->getMaterial();
	}

	DebugMeshRenderObject::DebugMeshRenderObject(NodeRef parent, glm::mat4 transform, std::shared_ptr<DebugMesh> mesh) :
		Node(parent, transform),
		mMesh(mesh)
	{

	}

	DebugMeshRenderObject::DebugMeshRenderObject(NodeRef parent, HVK_shared<Transform> transform, std::shared_ptr<DebugMesh> mesh) :
		Node(parent, transform),
		mMesh(mesh)
	{
		
	}

	DebugMeshRenderObject::~DebugMeshRenderObject()
	{

	}

	const DebugMesh::ColorVertices DebugMeshRenderObject::getVertices() const
	{
		return mMesh->getVertices();
	}

	const DebugMesh::Indices DebugMeshRenderObject::getIndices() const
	{
		return mMesh->getIndices();
	}
}
