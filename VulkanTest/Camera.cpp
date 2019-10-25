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
        mProjection(glm::perspective(glm::radians(mFov), mAspectRatio, mNearPlane, mFarPlane)) 
    {

    }


    Camera::~Camera() {
    }

	void Camera::updateProjection(float fov, float aspectRatio, float near, float far) {
		mFov = fov;
		mAspectRatio = aspectRatio;
		mNearPlane = near;
		mFarPlane = far;

        mProjection = glm::perspective(glm::radians(mFov), mAspectRatio, mNearPlane, mFarPlane);
	}

	glm::vec3  Camera::getUpVector() const {
		return glm::vec3(getLocalTransform()[1]);
	}

	glm::vec3  Camera::getForwardVector() const {
		return glm::vec3(getLocalTransform()[2]);
	}

	glm::vec3  Camera::getRightVector() const {
		return glm::vec3(getLocalTransform()[0]);
	}

	glm::mat4 Camera::getViewTransform() const {
		return glm::inverse(getWorldTransform());
	}

	void Camera::rotate(float radPitch, float radYaw) {
        glm::mat4 local = getLocalTransform();

		glm::mat4 pitchRot = glm::rotate(local, radPitch, glm::vec3(1.f, 0.f, 0.f));
        glm::mat4 yawRot = glm::rotate(pitchRot, radYaw, glm::vec3(pitchRot[1]));
        setLocalTransform(yawRot);
	}

}
