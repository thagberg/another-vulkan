#include "CameraController.h"

#include <GLFW/glfw3.h>
#include "InputManager.h"

namespace hvk {

	enum class CameraControl : uint8_t {
		move_left,
		move_right,
		move_up,
		move_down,
		move_forward,
		move_backward
	};
	
	std::unordered_map<CameraControl, int> cameraControlMapping({
		{CameraControl::move_left, GLFW_KEY_A},
		{CameraControl::move_right, GLFW_KEY_D},
		{CameraControl::move_forward, GLFW_KEY_W},
		{CameraControl::move_backward, GLFW_KEY_S},
		{CameraControl::move_up, GLFW_KEY_Q},
		{CameraControl::move_down, GLFW_KEY_E}
	});

	CameraController::CameraController(CameraRef camera) :
		mCamera(camera),
		mRotation{0.f, 0.f, 0.f}
	{
	}


	CameraController::~CameraController()
	{
	}

	void CameraController::update(double d, std::vector<Command>& commands) {
		MouseState mouse = InputManager::currentMouseState;
		MouseState prevMouse = InputManager::previousMouseState;

		const glm::vec3 forwardMovement = (float)d *  mCamera->getForwardVector();
		const glm::vec3 lateralMovement = (float)d * mCamera->getRightVector();
		const glm::vec3 verticalMovement = (float)d * mCamera->getUpVector();

		bool rotated = false;
		float pitch = 0.f, yaw = 0.f;

		for (const Command& c : commands) {
			switch (c.id) {
			case 0: // move forward
				if (auto pSpeed = std::get_if<float>(&c.payload)) {
					mCamera->translateLocal(*pSpeed * forwardMovement);
				}
				break;
			case 1: // move right
				if (auto pSpeed = std::get_if<float>(&c.payload)) {
					mCamera->translateLocal(*pSpeed * lateralMovement);
				}
				break;
			case 2: // move up
				if (auto pSpeed = std::get_if<float>(&c.payload)) {
					mCamera->translateLocal(*pSpeed * verticalMovement);
				}
				break;
			case 3: // pitch
				if (auto pPitch = std::get_if<float>(&c.payload)) {
					pitch = *pPitch;
					mRotation.pitch += pitch;
					rotated = true;
				}
				break;
			case 4: // yaw
				if (auto pYaw = std::get_if<float>(&c.payload)) {
					yaw = *pYaw;
					mRotation.yaw += yaw;
					rotated = true;
				}
				break;
			}
		}

		if (rotated) {
			mCamera->rotate(glm::radians(pitch), glm::radians(yaw));
		}
	}
}
