#include "Light.h"


namespace hvk {

	Light::Light(NodeRef parent, glm::mat4 transform, glm::vec3 color) :
		Node(parent, transform),
		mColor(color),
		mIntensity(0.f)
	{
	}


	Light::~Light()
	{
	}
}
