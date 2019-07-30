#include "stdafx.h"

#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

namespace hvk {

    Camera::Camera(float fov, float aspectRatio, float near, float far, NodeRef parent, glm::mat4 transform) :
        Node(parent, transform),
        mFov(fov),
        mAspectRatio(aspectRatio),
        mNearPlane(near),
        mFarPlane(far) {

        mUpVec = glm::vec3(0.f, 0.f, 1.f);
        mProjection = glm::perspective(glm::radians(mFov), mAspectRatio, mNearPlane, mFarPlane);
    }


    Camera::~Camera() {
    }
}
