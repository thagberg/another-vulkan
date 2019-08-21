#pragma once

#include <array>
#include <memory>
#include <vector>

#include "GLFW\glfw3.h"

namespace hvk {

    typedef int KeyCode;
    typedef int MouseButton;

    class InputManager
    {
    private:
        InputManager();
        ~InputManager();

        static std::shared_ptr<GLFWwindow> window;
        static std::vector<KeyCode> keysToCheck;

    public:
        static void processKeyEvent(GLFWwindow* window, KeyCode key, int scancode, int action, int mods);
        static void processMouseMovementEvent(GLFWwindow* window, double x, double y);
        static void processMouseClickEvent(GLFWwindow* window, MouseButton button, int action, int mods);

        static void init(std::shared_ptr<GLFWwindow> window);
        static void update();

        static std::array<bool, GLFW_KEY_LAST> currentKeysPressed;
        static std::array<bool, GLFW_KEY_LAST> previousKeysPressed;
        static std::array<uint32_t, GLFW_KEY_LAST> keysFramesHeld;
    };
}
