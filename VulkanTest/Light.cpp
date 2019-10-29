#include "Light.h"


namespace hvk {

	Light::Light(NodeRef parent, glm::mat4 transform, glm::vec3 color, float intensity) :
		Node(parent, transform),
		mColor(color),
        mIntensity(intensity)
	{
	}

	Light::Light(NodeRef parent, HVK_shared<Transform> transform, glm::vec3 color, float intensity) :
		Node(parent, transform),
		mColor(color),
		mIntensity(intensity)
	{
		
	}

	Light::~Light()
	{
	}
}
