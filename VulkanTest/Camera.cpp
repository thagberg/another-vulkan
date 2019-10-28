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
        mPitch(0.f),
        mYaw(0.f),
        mProjection(glm::perspective(glm::radians(mFov), mAspectRatio, mNearPlane, mFarPlane)) 
    {

    }


    Camera::~Camera() {
    }

	void Camera::updateProjection(float fov, float aspectRatio, float nearPlane, float farPlane) {
		mFov = fov;
		mAspectRatio = aspectRatio;
		mNearPlane = nearPlane;
		mFarPlane = farPlane;

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
        mPitch += radPitch;
        mYaw += radYaw;
        std::clamp(mPitch, -1.57f, 1.57f);
        if (mYaw > 6.28139) {
            mYaw -= 6.28139;
        }

        glm::mat4 trans = glm::translate(glm::mat4(1.f), getLocalPosition());
		glm::mat4 pitchRot = glm::rotate(glm::mat4(1.f), mPitch, glm::vec3(1.f, 0.f, 0.f));
        glm::mat4 yawRot = glm::rotate(glm::mat4(1.f), mYaw, glm::vec3(0.f, 1.f, 0.f));

        setLocalTransform( trans * yawRot * pitchRot );
	}
}
