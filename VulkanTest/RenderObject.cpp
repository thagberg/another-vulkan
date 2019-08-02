#include "stdafx.h"
#include "RenderObject.h"

namespace hvk {

	RenderObject::RenderObject(NodeRef parent, glm::mat4 transform) : Node(parent, transform)
	{
	}


	RenderObject::~RenderObject()
	{
	}
}
