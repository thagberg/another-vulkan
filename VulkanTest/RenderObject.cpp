#include "stdafx.h"
#include "RenderObject.h"

#include <glm/gtc/type_ptr.hpp>

namespace hvk {

	RenderObject::RenderObject(std::string name, HVK_shared<Node> parent, glm::mat4 transform) :
        Node(name, parent, transform)
	{
	}

	RenderObject::~RenderObject()
	{
	}

	StaticMeshRenderObject::StaticMeshRenderObject(std::string name, HVK_shared<Node> parent, glm::mat4 transform, std::shared_ptr<StaticMesh> mesh) :
		Node(name, parent, transform),
		mMesh(mesh)
	{

	}

    StaticMeshRenderObject::StaticMeshRenderObject(std::string name, HVK_shared<Node> parent, HVK_shared<Transform> transform, std::shared_ptr<StaticMesh> mesh) :
        Node(name, parent, transform),
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

	DebugMeshRenderObject::DebugMeshRenderObject(std::string name, HVK_shared<Node> parent, glm::mat4 transform, std::shared_ptr<DebugMesh> mesh) :
		Node(name, parent, transform),
		mMesh(mesh)
	{

	}

	DebugMeshRenderObject::DebugMeshRenderObject(std::string name, HVK_shared<Node> parent, HVK_shared<Transform> transform, std::shared_ptr<DebugMesh> mesh) :
		Node(name, parent, transform),
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

    CubeMeshRenderObject::CubeMeshRenderObject(
        std::string name,
        HVK_shared<Node> parent,
        glm::mat4 transform,
        HVK_shared<CubeMesh> mesh) :

        Node(name, parent, transform),
        mMesh(mesh)
    {

    }

    CubeMeshRenderObject::CubeMeshRenderObject(
        std::string name,
        HVK_shared<Node> parent,
        HVK_shared<Transform> transform,
        HVK_shared<CubeMesh> mesh) :

        Node(name, parent, transform),
        mMesh(mesh)
    {

    }
}
