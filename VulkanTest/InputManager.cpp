#include "InputManager.h"


namespace hvk {

    std::array<bool, GLFW_KEY_LAST> InputManager::currentKeysPressed;
    std::array<bool, GLFW_KEY_LAST> InputManager::previousKeysPressed;
    std::array<uint32_t, GLFW_KEY_LAST> InputManager::keysFramesHeld;
    std::vector<KeyCode> InputManager::keysToCheck;
    std::shared_ptr<GLFWwindow> InputManager::window = nullptr;

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

        glfwSetKeyCallback(window.get(), processKeyEvent);
        glfwSetCursorPosCallback(window.get(), processMouseMovementEvent);
        glfwSetMouseButtonCallback(window.get(), processMouseClickEvent);
    }

    void InputManager::processKeyEvent(
        GLFWwindow* window, 
        KeyCode key, 
        int scancode, 
        int action, 
        int mods) {

        bool pressed = (action & (GLFW_PRESS | GLFW_RELEASE));
        currentKeysPressed[key] = pressed;
        keysToCheck.push_back(key);
    }

    void InputManager::update() {
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

        previousKeysPressed = currentKeysPressed;
        currentKeysPressed.fill(0);
    }
}
