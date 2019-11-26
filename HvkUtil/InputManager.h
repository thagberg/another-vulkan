#pragma once

#include <array>
#include <memory>
#include <vector>
#include <unordered_map>
#include <variant>

#include "GLFW\glfw3.h"

namespace hvk {

	typedef uint32_t InputID;
    typedef int KeyCode;
    typedef int MouseButton;

	typedef void (*MouseMoveHandler)(double, double);

	struct MouseState {
		double x;
		double y;
		bool leftDown;
		bool rightDown;
	};

	enum class InputRange : uint8_t {
		Action,
		Axis
	};

	enum class InputDeviceType : uint8_t {
		Keyboard,
		MouseButton,
		MouseAxis
	};

	struct InputObserverRequest {
		InputRange range;
		InputDeviceType deviceType;
		uint16_t inputCode;
	};

	enum class InputType : uint8_t {
		MouseButton,
		MouseAxis,
		Key,
		Button,
		JoyAxis,
		MouseButtonDown,
		MouseButtonUp,
		KeyPressed,
		KeyReleased,
		ButtonPressed,
		ButtonReleased,
		MouseAxisMoved,
		JoyAxisMoved,
	};

	/*enum class MouseButton : uint8_t {
		Left = GLFW_MOUSE_BUTTON_LEFT,
		Right = GLFW_MOUSE_BUTTON_RIGHT,
		Middle = GLFW_MOUSE_BUTTON_MIDDLE
	};*/

	struct MouseMoveEvent {
		double x;
		double y;
	};

	struct MouseButtonEvent {
		MouseButton button;
		bool pressed;
	};

	struct KeyEvent {
		uint16_t key;
		bool pressed;
	};

    class InputManager
    {
    private:
        InputManager();
        ~InputManager();

        static std::shared_ptr<GLFWwindow> window;
        static std::vector<KeyCode> keysToCheck;
		static bool initialized;
		//static uint32_t nextListenerId;
		//static std::unordered_map<

    public:
        static void processKeyEvent(GLFWwindow* window, KeyCode key, int scancode, int action, int mods);
        static void processMouseMovementEvent(GLFWwindow* window, double x, double y);
        static void processMouseClickEvent(GLFWwindow* window, MouseButton button, int action, int mods);

        static void init(std::shared_ptr<GLFWwindow> window);
        static void update();

		static bool isDown(InputID input);
		static bool wasPressed(InputID input);

		//static void registerEventCallback(InputType inputType)
		//static uint32_t registerMouseMoveEventListener();
		//static uint32_t registerMouseButtonEventListener();
		//static uint32_t registerKeyEventListener();

        static std::array<bool, GLFW_KEY_LAST> currentKeysPressed;
        static std::array<bool, GLFW_KEY_LAST> previousKeysPressed;
        static std::array<uint32_t, GLFW_KEY_LAST> keysFramesHeld;

		static MouseState currentMouseState;
		static MouseState previousMouseState;
    };
}
