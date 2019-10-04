#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include "tiny_gltf.h"

#include "types.h"
#include "Node.h"
#include "StaticMesh.h"

namespace hvk {

    typedef std::shared_ptr<std::vector<Vertex>> VerticesRef;
    typedef std::shared_ptr<std::vector<VertIndex>> IndicesRef;
	typedef std::shared_ptr<tinygltf::Image> TextureRef;

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

	class StaticMeshRenderObject : public RenderObject
	{
	private:
		StaticMesh& mMesh;
	public:
		StaticMeshRenderObject(NodeRef parent, glm::mat4 transform, StaticMesh& mesh);
		~StaticMeshRenderObject();

		const std::vector<Vertex>& getVertices() const override;
		const std::vector<VertIndex>& getIndices() const override;
		Material& getMaterial() const override;
	};
}
