#include <iostream>
#include <variant>

#include "imgui\imgui.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "UserApp.h"
#include "HvkUtil.h"
#include "InputManager.h"
#include "Transform.h"
#include "RenderObject.h"
#include "gltf.h"

const uint32_t HEIGHT = 1024;
const uint32_t WIDTH = 1024;

class TestApp : public UserApp
{
private:
    hvk::HVK_shared<hvk::StaticMeshRenderObject> mDuck;
    hvk::HVK_shared<hvk::Light> mDynamicLight;
    hvk::HVK_shared<hvk::DebugMeshRenderObject> mLightBox;

public:
	TestApp(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle) :
		UserApp(windowWidth, windowHeight, windowTitle),
        mDuck(nullptr),
        mDynamicLight(nullptr),
        mLightBox(nullptr)
	{
        hvk::HVK_shared<hvk::StaticMesh> duckMesh(hvk::createMeshFromGltf("resources/duck/Duck.gltf"));
        glm::mat4 duckTransform = glm::mat4(1.f);
        duckTransform = glm::scale(duckTransform, glm::vec3(0.01, 0.01f, 0.01f));
        mDuck = hvk::HVK_make_shared<hvk::StaticMeshRenderObject>(
            nullptr, 
            duckTransform, 
            duckMesh);
        addStaticMeshInstance(mDuck);

        glm::mat4 lightTransform = glm::mat4(1.f);
        lightTransform = glm::scale(lightTransform, glm::vec3(0.1f));
        lightTransform = glm::translate(lightTransform, glm::vec3(3.f, 2.f, 1.5f));
        mDynamicLight = hvk::HVK_make_shared<hvk::Light>(
            nullptr, 
            lightTransform, 
            glm::vec3(1.f, 1.f, 1.f), 0.3f);
        addDynamicLight(mDynamicLight);
	}

	virtual ~TestApp() = default;

protected:
	bool run(double frameTime) override
	{
        bool shouldClose = false;
        bool cameraDrag = false;

        hvk::InputManager::update();
        if (hvk::InputManager::currentKeysPressed[GLFW_KEY_ESCAPE]) {
            shouldClose = true;
        }

        ImGuiIO& io = ImGui::GetIO();
        hvk::MouseState mouse = hvk::InputManager::currentMouseState;
        hvk::MouseState prevMouse = hvk::InputManager::previousMouseState;
        io.DeltaTime = static_cast<float>(frameTime);
        io.MousePos = ImVec2(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
        io.MouseDown[0] = mouse.leftDown;
        io.KeyCtrl = hvk::InputManager::isPressed(GLFW_KEY_LEFT_CONTROL);

        bool mouseClicked = mouse.leftDown && !prevMouse.leftDown;
        bool mouseReleased = prevMouse.leftDown && !mouse.leftDown;
        if (mouseClicked && !io.WantCaptureMouse) {
            cameraDrag = true;
            toggleCursor(false);
        }
        if (mouseReleased) {
            cameraDrag = false;
            toggleCursor(true);
        }

        float sensitivity = 0.1f;
        float mouseDeltY = static_cast<float>(mouse.y - prevMouse.y);
        float mouseDeltX = static_cast<float>(prevMouse.x - mouse.x);

        std::vector<hvk::Command> cameraCommands;
        cameraCommands.reserve(6);
        if (hvk::InputManager::isPressed(GLFW_KEY_W)) {
            cameraCommands.push_back(hvk::Command{ 0, "move_forward", -1.0f });
        }
        if (hvk::InputManager::isPressed(GLFW_KEY_S)) {
            cameraCommands.push_back(hvk::Command{ 0, "move_forward", 1.0f });
        }
        if (hvk::InputManager::isPressed(GLFW_KEY_A)) {
            cameraCommands.push_back(hvk::Command{ 1, "move_right", -1.0f });
        }
        if (hvk::InputManager::isPressed(GLFW_KEY_D)) {
            cameraCommands.push_back(hvk::Command{ 1, "move_right", 1.0f });
        }
        if (hvk::InputManager::isPressed(GLFW_KEY_Q)) {
            cameraCommands.push_back(hvk::Command{ 2, "move_up", -1.0f });
        }
        if (hvk::InputManager::isPressed(GLFW_KEY_E)) {
            cameraCommands.push_back(hvk::Command{ 2, "move_up", 1.0f });
        }
        if (hvk::InputManager::isPressed(GLFW_KEY_UP))
        {
            cameraCommands.push_back(hvk::Command{ 3, "camera_pitch", 0.25f });
        }
        if (hvk::InputManager::isPressed(GLFW_KEY_DOWN))
        {
            
            cameraCommands.push_back(hvk::Command{ 3, "camera_pitch", -0.25f });
        }
        if (hvk::InputManager::isPressed(GLFW_KEY_LEFT))
        {
            
            cameraCommands.push_back(hvk::Command{ 4, "camera_yaw", -0.25f });
        }
        if (hvk::InputManager::isPressed(GLFW_KEY_RIGHT))
        {
            cameraCommands.push_back(hvk::Command{ 4, "camera_yaw", 0.25f });
        }
        if (cameraDrag) {
            if (mouseDeltY) {
                cameraCommands.push_back(hvk::Command{ 3, "camera_pitch", mouseDeltY * sensitivity });
            }
            if (mouseDeltX) {
                cameraCommands.push_back(hvk::Command{ 4, "camera_yaw", mouseDeltX * sensitivity });
            }
        }

        // TODO: dispatch cameraCommands to a CameraController

        // GUI
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        ImGui::EndFrame();

        return shouldClose;
	}

	void close() override
	{
		std::cout << "Closing Test App" << std::endl;
	}
};

int main()
{
	TestApp thisApp(WIDTH, HEIGHT, "Test App");
	thisApp.runApp();
	thisApp.doClose();

    return 0;
}
