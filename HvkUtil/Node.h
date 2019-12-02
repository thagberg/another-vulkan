#pragma once

#include <string>

#include "HvkUtil.h"
#include "Transform.h"

namespace hvk {
    class Node;

    class Node {
    private:
		uint64_t mId;
		std::string mName;
        HVK_shared<Node> mParent;
        HVK_vector<HVK_shared<Node>> mChildren;

		static uint64_t sNextId;

    protected:
        HVK_shared<Transform> mTransform;

    public:
		HVK_shared<Node> getParent() { return mParent; }
		const HVK_vector<HVK_shared<Node>>& getChildren() { return mChildren; }
		void addChild(HVK_shared<Node> child);

        void setLocalTransform(glm::mat4 transform);
		void translateLocal(const glm::vec3& trans);
		HVK_shared<Transform> getTransform() { return mTransform; }
        glm::mat4 getLocalTransform() const { return mTransform->transform; }
        glm::mat4 getWorldTransform() const;
		glm::vec3 getWorldPosition() const;
		glm::vec3 getLocalPosition() const;

        Node(std::string name, HVK_shared<Node> parent, HVK_shared<Transform> transform);
        Node(std::string name, HVK_shared<Node> parent, glm::mat4 transform);
        virtual ~Node();

#define HVK_TOOLS 1
#ifdef HVK_TOOLS
    protected:
		virtual void showGui();
    public:
		void displayGui();
#endif
    };

}
