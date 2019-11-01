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
	public:
		CameraController(HVK_shared<Camera> camera);
		~CameraController();
		void update(double d, std::vector<Command>& commands);
	};
}
