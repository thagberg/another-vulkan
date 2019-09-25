#pragma once
#include "Camera.h"

#include "types.h"

namespace hvk {

	struct CameraRotation {
		double yaw;
		double pitch;
		double roll;
	};

	class CameraController
	{
	private:
		CameraRef mCamera;
		CameraRotation mRotation;
	public:
		CameraController(CameraRef camera);
		~CameraController();
		void update(double d, std::vector<Command>& commands);
	};
}
