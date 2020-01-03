#pragma once
#include "Camera.h"

#include "HvkUtil.h"

namespace hvk {

	struct CameraRotation {
		double yaw;
		double pitch;
		double roll;
	};

	class CameraController
	{
	private:
		HVK_shared<Camera> mCamera;
		CameraRotation mRotation;
		float mMovementSpeed;
	public:
		CameraController(HVK_shared<Camera> camera, float movementSpeed=1.f);
		~CameraController();
		void update(double d, std::vector<Command>& commands);
        void setCamera(HVK_shared<Camera> camera)
        {
            mCamera = camera;
        }
	};
}
