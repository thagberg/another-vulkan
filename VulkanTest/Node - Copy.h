#pragma once

#include <memory>
#include <vector>

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
        void setLocalTransform(glm::mat4 transform);
        glm::mat4 getLocalTransform() { return mTransform; }
        glm::mat4 getWorldTransform();

        Node(NodeRef parent, glm::mat4 transform);
        ~Node();
    };

}
