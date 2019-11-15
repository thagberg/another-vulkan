#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include "tiny_gltf.h"

#include "types.h"
#include "Node.h"
#include "StaticMesh.h"
#include "DebugMesh.h"
#include "CubeMesh.h"

namespace hvk {

	class RenderObject;
	typedef std::shared_ptr<RenderObject> RenderObjRef;

	class RenderObject : public Node
	{
	public:
		RenderObject(std::string name, HVK_shared<Node> parent, glm::mat4 transform);
		~RenderObject();

		virtual const std::vector<Vertex>& getVertices() const = 0;
		virtual const std::vector<VertIndex>& getIndices() const = 0;
		virtual Material& getMaterial() const = 0;
	};

	class StaticMeshRenderObject : public Node
	{
	private:
		std::shared_ptr<StaticMesh> mMesh;
	public:
		StaticMeshRenderObject(std::string name, HVK_shared<Node> parent, glm::mat4 transform, std::shared_ptr<StaticMesh> mesh);
		StaticMeshRenderObject(std::string name, HVK_shared<Node> parent, HVK_shared<Transform> transform, std::shared_ptr<StaticMesh> mesh);
		~StaticMeshRenderObject();

		const StaticMesh::Vertices getVertices() const;
		const StaticMesh::Indices getIndices() const;
		const std::shared_ptr<Material> getMaterial() const;
	};

	class DebugMeshRenderObject : public Node 
	{
	private:
		std::shared_ptr<DebugMesh> mMesh;
	public:
		DebugMeshRenderObject(std::string name, HVK_shared<Node> parent, glm::mat4 transform, std::shared_ptr<DebugMesh> mesh);
		DebugMeshRenderObject(std::string name, HVK_shared<Node> parent, HVK_shared<Transform> transform, std::shared_ptr<DebugMesh> mesh);
		~DebugMeshRenderObject();

		const DebugMesh::ColorVertices getVertices() const;
		const DebugMesh::Indices getIndices() const;
	};

    class CubeMeshRenderObject : public Node
    {
    private:
        HVK_shared<CubeMesh> mMesh;
    public:
        CubeMeshRenderObject(
            std::string name,
            HVK_shared<Node> parent,
            glm::mat4 transform,
            HVK_shared<CubeMesh> mesh);
        CubeMeshRenderObject(
            std::string name,
            HVK_shared<Node> parent,
            HVK_shared<Transform> transform,
            HVK_shared<CubeMesh> mesh);

        const CubeMesh::CubeVertices getVertices() const;
        const CubeMesh::Indices getIndices() const;
    };
}
