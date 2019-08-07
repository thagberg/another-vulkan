#include "stdafx.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

namespace hvk {

    Camera::Camera(float fov, float aspectRatio, float near, float far, NodeRef parent, glm::mat4 transform) :
        Node(parent, transform),
        mFov(fov),
        mAspectRatio(aspectRatio),
        mNearPlane(near),
        mFarPlane(far),
		mLookVec(glm::vec3(0.f, 0.f, 1.f)),
		mLookLocked(false) {

        mProjection = glm::perspective(glm::radians(mFov), mAspectRatio, mNearPlane, mFarPlane);
    }


    Camera::~Camera() {
    }

	//void Camera::setLocalTransform(glm::mat4 transform) {
		// TODO: Maybe an error here or something?  Shouldn't
		// explicitly set the full transform of the camera
	//}

	glm::vec3  Camera::getUpVector() const {
		return glm::vec3(getWorldTransform()[1]);
	}

	glm::vec3  Camera::getForwardVector() const {
		return glm::vec3(getWorldTransform()[2]);
	}

	glm::vec3  Camera::getRightVector() const {
		return glm::vec3(getWorldTransform()[0]);
	}

	glm::mat4 Camera::getViewTransform() const {
		return glm::lookAt(getWorldPosition(), mLookVec, getUpVector());
	}

	void Camera::setLookAt(glm::vec3 center) {
		mLookVec = glm::normalize(center);
	}

	void Camera::translateLocal(const glm::vec3& trans) {
		Node::translateLocal(trans);
		if (!mLookLocked) {
			mLookVec += trans;
		}
	}
}
