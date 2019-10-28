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
		glm::vec3 invLocalPos = -getLocalPosition();
		glm::vec3 rightVec = getRightVector();
		glm::vec3 upVec = getUpVector();

		//local = glm::translate(local, invLocalPos);

		glm::mat4 pitchRot = glm::rotate(glm::mat4(1.f), radPitch, rightVec);
        glm::mat4 yawRot = glm::rotate(glm::mat4(1.f), radYaw, glm::vec3(0.f, 1.f, 0.f));
		//glm::mat4 rollRot = glm::rotate(glm::mat4(1.f), radPitch, glm::vec3(0.f, 0.f, 1.f));
		//glm::mat4 yawRot = local;
		//local = local * yawRot;
		//local = local * pitchRot;
		//local = glm::translate(pitchRot, getLocalPosition());
        setLocalTransform( local * yawRot * pitchRot );
	}

}
