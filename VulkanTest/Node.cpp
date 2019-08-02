#include "stdafx.h"
#include "Node.h"

namespace hvk {
    Node::Node(NodeRef parent, glm::mat4 transform) :
        mParent(parent),
        mTransform(transform) {

    }

    Node::~Node() {

    }

    void Node::setLocalTransform(glm::mat4 transform) {
        mTransform = transform;
    }

    const glm::mat4 Node::getWorldTransform() {
        if (mParent) {
            return mParent->getWorldTransform() * mTransform;
        } else {
            return mTransform;
        }
    }
}