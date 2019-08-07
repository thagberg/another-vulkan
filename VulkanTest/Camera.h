#pragma once

#include <glm/glm.hpp>

#include "Node.h"

namespace hvk {

	class Camera;
	typedef std::shared_ptr<Camera> CameraRef;

    class Camera : public Node
    {
    public:
        Camera(float fov, float aspectRatio, float near, float far, NodeRef parent, glm::mat4 transform);
        ~Camera();

		glm::mat4 getProjection() const { return mProjection; }
		glm::vec3 getUpVector() const;
		glm::vec3 getForwardVector() const;
		glm::vec3 getRightVector() const;
		glm::mat4 getViewTransform() const;
		void setLookAt(glm::vec3 center);
		virtual void translateLocal(const glm::vec3& trans) override;

    private:
		bool mLookLocked;
        float mFov;
        float mAspectRatio;
        float mNearPlane;
        float mFarPlane;
		glm::vec3 mLookVec;
        glm::mat4 mProjection;
    };
}
