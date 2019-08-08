#include "stdafx.h"
#include "Camera.h"

#include <math.h>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>


namespace hvk {

    Camera::Camera(float fov, float aspectRatio, float near, float far, NodeRef parent, glm::mat4 transform) :
        Node(parent, transform),
        mFov(fov),
        mAspectRatio(aspectRatio),
        mNearPlane(near),
        mFarPlane(far),
		mLookVec(glm::vec3(0.f, 0.f, 1.f)),
		mPitch(0.0f),
		mYaw(0.0f) {

        mProjection = glm::perspective(glm::radians(mFov), mAspectRatio, mNearPlane, mFarPlane);
    }


    Camera::~Camera() {
    }

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
		glm::mat4 xRot = glm::rotate(glm::mat4(1.0f), mPitch, glm::vec3(1.f, 0.f, 0.f));
		glm::mat4 yRot = glm::rotate(glm::mat4(1.0f), mYaw, glm::vec3(0.f, 1.f, 0.f));

		return glm::inverse(getWorldTransform() *yRot * xRot);
	}

	void Camera::setLookAt(glm::vec3 center) {
		mLookVec = glm::normalize(center);
	}

	void Camera::rotate(float radPitch, float radYaw) {
		mPitch += radPitch;
		mYaw += radYaw;

		mPitch = std::clamp(mPitch, -89.0f, 89.0f);
	}
}
