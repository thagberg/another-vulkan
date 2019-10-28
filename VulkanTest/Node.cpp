#include "stdafx.h"
#include "Node.h"
#include <glm/gtc/matrix_transform.hpp>


namespace hvk {
    Node::Node(NodeRef parent, glm::mat4 transform) :
        mParent(parent),
        mTransform(transform) 
	{

    }

    Node::~Node() 
	{

    }

    void Node::setLocalTransform(glm::mat4 transform) 
	{
        mTransform = transform;
    }
	
	void Node::translateLocal(const glm::vec3& trans) 
	{
		mTransform = glm::translate(glm::mat4(1.f), trans) * mTransform;
	}

    glm::mat4 Node::getWorldTransform() const 
	{
        if (mParent != nullptr) {
            return mParent->getWorldTransform() * getLocalTransform();
        } else {
            return getLocalTransform();
        }
    }
	
	glm::vec3 Node::getWorldPosition() const 
	{
		return glm::vec3(getWorldTransform()[3]);
	}

	void Node::addChild(NodeRef child)
	{
		mChildren.push_back(child);
	}

	glm::vec3 Node::getLocalPosition() const
    {
		return mTransform[3];
    }
}