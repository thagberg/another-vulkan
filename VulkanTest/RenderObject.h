#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include "tiny_gltf.h"

#include "types.h"
#include "Node.h"
#include "StaticMesh.h"
#include "DebugMesh.h"

namespace hvk {

    //typedef std::shared_ptr<std::vector<Vertex>> VerticesRef;
    //typedef std::shared_ptr<std::vector<VertIndex>> IndicesRef;
	//typedef std::shared_ptr<tinygltf::Image> TextureRef;

	class RenderObject;
	typedef std::shared_ptr<RenderObject> RenderObjRef;

	class RenderObject : public Node
	{
	public:
		RenderObject(NodeRef parent, glm::mat4 transform);
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
		StaticMeshRenderObject(NodeRef parent, glm::mat4 transform, std::shared_ptr<StaticMesh> mesh);
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
		DebugMeshRenderObject(NodeRef parent, glm::mat4 transform, std::shared_ptr<DebugMesh> mesh);
		~DebugMeshRenderObject();

		const DebugMesh::ColorVertices getVertices() const;
		const DebugMesh::Indices getIndices() const;
	};

}
