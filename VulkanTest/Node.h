#pragma once

#include <memory>
#include <vector>

#define GLM_SWIZZLE_XYZW
#include <glm/glm.hpp>


namespace hvk {
    class Node;
    typedef std::shared_ptr<hvk::Node> NodeRef;

    class Node {
    private:
        NodeRef mParent;
        std::vector<NodeRef> mChildren;
        glm::mat4 mTransform;

    public:
		const NodeRef getParent() { return mParent; }
		const std::vector<NodeRef>& getChildren() { return mChildren; }

        void setLocalTransform(glm::mat4 transform);
        const glm::mat4 getLocalTransform() { return mTransform; }
        const glm::mat4 getWorldTransform();
		const glm::vec3 getWorldPosition();

        Node(NodeRef parent, glm::mat4 transform);
        ~Node();
    };

}