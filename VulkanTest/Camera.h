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

		const glm::mat4 getProjection() { return mProjection; }

    private:
        float mFov;
        float mAspectRatio;
        float mNearPlane;
        float mFarPlane;
        glm::vec3 mUpVec;
        glm::mat4 mProjection;
    };
}
