#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "types.h"
#include "Node.h"

namespace hvk {
	class RenderObject;
	typedef std::shared_ptr<RenderObject> RenderObjRef;

	class RenderObject : public Node
	{
	public:
		RenderObject( NodeRef parent, glm::mat4 transform);
		~RenderObject();

		const std::vector<Vertex> getVertices();
		const std::vector<uint16_t> getIndices();
	};
}
