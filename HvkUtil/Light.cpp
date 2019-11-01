#include "pch.h"
#include "Light.h"


namespace hvk {

	Light::Light(HVK_shared<Node> parent, glm::mat4 transform, glm::vec3 color, float intensity) :
		Node(parent, transform),
		mColor(color),
        mIntensity(intensity)
	{
	}

	Light::Light(HVK_shared<Node> parent, HVK_shared<Transform> transform, glm::vec3 color, float intensity) :
		Node(parent, transform),
		mColor(color),
		mIntensity(intensity)
	{
		
	}

	Light::~Light()
	{
	}
}
