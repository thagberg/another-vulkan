#pragma once

#include <glm/glm.hpp>

#include "Node.h"

namespace hvk {

    class Camera : public Node
    {
    public:
        Camera(float fov, float aspectRatio, float near, float far, NodeRef parent, glm::mat4 transform);
        ~Camera();

    private:
        float mFov;
        float mAspectRatio;
        float mNearPlane;
        float mFarPlane;
        glm::vec3 mUpVec;
        glm::mat4 mProjection;
    };
}
