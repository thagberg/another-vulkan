#pragma once
#include "Node.h"
#include <glm/glm.hpp>

namespace hvk {
	class Light;
	typedef std::shared_ptr<Light> LightRef;

	class Light :
		public Node
	{
	private:
		glm::vec3 mColor;
	public:
		Light(NodeRef parent, glm::mat4 transform, glm::vec3 color);
		~Light();

		glm::vec3 getColor() const { return mColor; }
        void setColor(glm::vec3 color) { mColor = color; }
	};

}
