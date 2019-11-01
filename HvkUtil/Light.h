#pragma once
#include "Node.h"
#include <glm/glm.hpp>

namespace hvk {

	class Light :
		public Node
	{
	private:
		glm::vec3 mColor;
		float mIntensity;
	public:
		Light(HVK_shared<Node> parent, glm::mat4 transform, glm::vec3 color, float intensity);
		Light(HVK_shared<Node> parent, HVK_shared<Transform> transform, glm::vec3 color, float intensity);
		~Light();

		glm::vec3 getColor() const { return mColor; }
        void setColor(glm::vec3 color) { mColor = color; }
		float getIntensity() const { return mIntensity; }
		void setIntensity(float intensity) { mIntensity = intensity; }
	};

}
