#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "types.h"
#include "Node.h"

namespace hvk {

    typedef std::shared_ptr<std::vector<Vertex>> VerticesRef;
    typedef std::shared_ptr<std::vector<uint16_t>> IndicesRef;

	class RenderObject;
	typedef std::shared_ptr<RenderObject> RenderObjRef;

	class RenderObject : public Node
	{
    private:
        VerticesRef mVertices;
        IndicesRef mIndices;

	public:
		RenderObject(
            NodeRef parent, 
            glm::mat4 transform, 
            VerticesRef vertices, 
            IndicesRef indices);
		~RenderObject();

		const VerticesRef getVertices();
		const IndicesRef getIndices();
	};
}
