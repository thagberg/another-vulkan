#include "pch.h"
#include "InputManager.h"

#include <iostream>

#include "imgui/imgui.h"

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

		// Initialize imgui keymap
		ImGuiIO& io = ImGui::GetIO();
		io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
		io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
		io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
		io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
		io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
		io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
		io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
		io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
		io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
		io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
		io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
		io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
		io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
		io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
		io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
		io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
		io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

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
		ImGuiIO& io = ImGui::GetIO();
		io.KeysDown[key] = pressed;
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

	bool InputManager::isDown(InputID input) {
		if (input <= GLFW_KEY_LAST) {
			return currentKeysPressed[input];
		}
        return false;
	}

	bool InputManager::wasPressed(InputID input) {
		if (input <= GLFW_KEY_LAST) {
			return currentKeysPressed[input] && !previousKeysPressed[input];
		}
        return false;
	}
}
