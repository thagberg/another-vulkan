#include "pch.h"
#include "Node.h"
#include "glm/gtc/matrix_transform.hpp"


namespace hvk {
    Node::Node(std::string name, HVK_shared<Node> parent, HVK_shared<Transform> transform) :
		mName(name),
        mParent(parent),
        mTransform(transform),
		mChildren()
	{

    }

    Node::Node(std::string name, HVK_shared<Node> parent, glm::mat4 transform) :
		mName(name),
        mParent(parent),
		mChildren()
    {
        mTransform = HVK_make_shared<Transform>(Transform{ transform });
    }

    Node::~Node() 
	{

    }

    void Node::setLocalTransform(glm::mat4 transform) 
	{
        mTransform->transform = transform;
    }
	
	void Node::translateLocal(const glm::vec3& trans) 
	{
		mTransform->transform = glm::translate(glm::mat4(1.f), trans) * mTransform->transform;
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

	void Node::addChild(HVK_shared<Node> child)
	{
		mChildren.push_back(child);
	}

	glm::vec3 Node::getLocalPosition() const
    {
		return mTransform->transform[3];
    }

#ifdef HVK_TOOLS
	void Node::showGui()
	{
		glm::vec3 position = getLocalPosition();
		ImGui::DragFloat3("Position", &position.x, 0.1f);
		glm::vec3 delta = position - getLocalPosition();
		translateLocal(delta);
	}

	void Node::displayGui()
    {
		if (ImGui::TreeNode(mName.c_str()))
		{
			showGui();
			for (auto child : mChildren)
			{
				child->displayGui();
			}
			ImGui::TreePop();
		}
    }
#endif

}