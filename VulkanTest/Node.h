#pragma once

#include <memory>
#include <vector>

#define GLM_SWIZZLE_XYZW
#include <glm/glm.hpp>

#include "types.h"
#include "Transform.h"

namespace hvk {
    class Node;
    typedef std::shared_ptr<hvk::Node> NodeRef;

    class Node {
    private:
        HVK_shared<Node> mParent;
        std::vector<HVK_shared<Node>> mChildren;

    protected:
        HVK_shared<Transform> mTransform;

    public:
		HVK_shared<Node> getParent() { return mParent; }
		const std::vector<HVK_shared<Node>>& getChildren() { return mChildren; }
		void addChild(HVK_shared<Node> child);

        void setLocalTransform(glm::mat4 transform);
		void translateLocal(const glm::vec3& trans);
		HVK_shared<Transform> getTransform() { return mTransform; }
        glm::mat4 getLocalTransform() const { return mTransform->transform; }
        glm::mat4 getWorldTransform() const;
		glm::vec3 getWorldPosition() const;
		glm::vec3 getLocalPosition() const;

        Node(HVK_shared<Node> parent, HVK_shared<Transform> transform);
        Node(HVK_shared<Node> parent, glm::mat4 transform);
        virtual ~Node();
    };

}
