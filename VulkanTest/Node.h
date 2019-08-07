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

        virtual void setLocalTransform(glm::mat4 transform);
		virtual void translateLocal(const glm::vec3& trans);
        glm::mat4 getLocalTransform() const { return mTransform; }
        glm::mat4 getWorldTransform() const;
		glm::vec3 getWorldPosition() const;

        Node(NodeRef parent, glm::mat4 transform);
        ~Node();
    };

}
