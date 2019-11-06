#include "pch.h"
#include "Light.h"


namespace hvk {

	Light::Light(std::string name, HVK_shared<Node> parent, glm::mat4 transform, glm::vec3 color, float intensity) :
		Node(name, parent, transform),
		mColor(color),
        mIntensity(intensity)
	{
	}

	Light::Light(std::string name, HVK_shared<Node> parent, HVK_shared<Transform> transform, glm::vec3 color, float intensity) :
		Node(name, parent, transform),
		mColor(color),
		mIntensity(intensity)
	{
		
	}

	Light::~Light()
	{
	}

#ifdef HVK_TOOLS
	void Light::showGui()
	{
		Node::showGui();
		ImGui::ColorEdit3("Color", &mColor.r);
		ImGui::SliderFloat("Intensity", &mIntensity, 0.f, 100.f);
	}
#endif
}
