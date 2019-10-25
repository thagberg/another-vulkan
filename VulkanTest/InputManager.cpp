#include "InputManager.h"

#include <iostream>

namespace hvk {

    std::array<bool, GLFW_KEY_LAST> InputManager::currentKeysPressed;
    std::array<bool, GLFW_KEY_LAST> InputManager::previousKeysPressed;
    std::array<uint32_t, GLFW_KEY_LAST> InputManager::keysFramesHeld;
    std::vector<KeyCode> InputManager::keysToCheck;
    std::shared_ptr<GLFWwindow> InputManager::window = nullptr;
	MouseState InputManager::currentMouseState = { 0.f, 0.f, false, false };
	MouseState InputManager::previousMouseState = { 0.f, 0.f, false, false };
	bool InputManager::initialized = false;
	//uint32_t InputManager::nextListenerId = 0;

    InputManager::InputManager() {
    }


    InputManager::~InputManager() {
    }

    void InputManager::init(std::shared_ptr<GLFWwindow> window) {
        window = window;
        currentKeysPressed.fill(false);
        previousKeysPressed.fill(false);
        keysFramesHeld.fill(0);
        keysToCheck.reserve(GLFW_KEY_LAST);

		glfwGetCursorPos(window.get(), &currentMouseState.x, &currentMouseState.y);

        glfwSetKeyCallback(window.get(), processKeyEvent);
        glfwSetCursorPosCallback(window.get(), processMouseMovementEvent);
        glfwSetMouseButtonCallback(window.get(), processMouseClickEvent);

		initialized = true;
    }

    void InputManager::processKeyEvent(
        GLFWwindow* window, 
        KeyCode key, 
        int scancode, 
        int action, 
        int mods) {

        bool pressed = (action & (GLFW_PRESS | GLFW_REPEAT));
		std::cout << "Key " << key << " pressed: " << pressed << std::endl;
        currentKeysPressed[key] = pressed;
        keysToCheck.push_back(key);
    }

	void InputManager::processMouseMovementEvent(GLFWwindow* window, double x, double y) {
		currentMouseState.x = x;
		currentMouseState.y = y;
	}

	void InputManager::processMouseClickEvent(GLFWwindow* window, MouseButton button, int action, int mods) {
		bool pressed = action == GLFW_PRESS;
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			currentMouseState.leftDown = pressed;
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			currentMouseState.rightDown = pressed;
		}
	}

    void InputManager::update() {
		previousMouseState = currentMouseState;
        previousKeysPressed = currentKeysPressed;
        glfwPollEvents();

        while (keysToCheck.size()) {
            KeyCode nextKey = keysToCheck.back();
            bool currentlyPressed = currentKeysPressed[nextKey];
            bool previouslyPressed = previousKeysPressed[nextKey];
            if (currentlyPressed && previouslyPressed) {
                keysFramesHeld[nextKey] += 1;
            }
            else if (previouslyPressed) {
                keysFramesHeld[nextKey] = 0;
            }
            keysToCheck.pop_back();
        }
    }

	bool InputManager::isPressed(InputID input) {
		if (input <= GLFW_KEY_LAST) {
			return currentKeysPressed[input];
		}
        return false;
	}
}
