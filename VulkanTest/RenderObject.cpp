#include "stdafx.h"
#include "RenderObject.h"

const std::vector<hvk::Vertex> vertices = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

namespace hvk {

	RenderObject::RenderObject( NodeRef parent, glm::mat4 transform) : Node(parent, transform)
	{
	}

	RenderObject::~RenderObject()
	{
	}

	const std::vector<Vertex> RenderObject::getVertices() {
		return vertices;
	}

	const std::vector<uint16_t> RenderObject::getIndices() {
		return indices;
	}
}
