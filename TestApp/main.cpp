#include <iostream>
#include <variant>

#define HVK_TOOLS 1

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "stb_image.h"
#include "entt/entt.hpp"

#include "Node.h"
#include "UserApp.h"
#include "HvkUtil.h"
#include "InputManager.h"
#include "DebugMesh.h"
#include "RenderObject.h"
#include "Light.h"
#include "CameraController.h"
#include "gltf.h"
#include "vulkan-util.h"
#include "shapes.h"
#include "ModelPipeline.h"
#include "SceneTypes.h"
#include "StaticMeshGenerator.h"

using namespace hvk;

const uint32_t HEIGHT = 1024;
const uint32_t WIDTH = 1024;

template <typename GroupType>
void TestGroup(GroupType& elements)
{
	std::cout << "Testing group reference" << std::endl;
	elements.each([](
		auto entity, 
		const hvk::NodeTransform& transform, 
		const hvk::PBRMesh& mesh, 
		const hvk::PBRBinding& binding) {

		std::cout << "Iterating over each element in group" << std::endl;
	});
}

class TestApp : public UserApp
{
private:
	HVK_shared<Node> mScene;
    HVK_shared<StaticMeshRenderObject> mDuck;
    HVK_shared<Light> mDynamicLight;
    HVK_shared<DebugMeshRenderObject> mLightBox;
    CameraController mCameraController;

public:
	TestApp(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle) :
		UserApp(windowWidth, windowHeight, windowTitle),
		mScene(nullptr),
        mDuck(nullptr),
        mDynamicLight(nullptr),
        mLightBox(nullptr),
        mCameraController(nullptr)
	{
		mScene = hvk::HVK_make_shared<hvk::Node>("Scene", nullptr, glm::mat4(1.f));
        //hvk::HVK_shared<hvk::StaticMesh> duckMesh(hvk::createMeshFromGltf("resources/duck/Duck.gltf"));
        hvk::HVK_shared<hvk::StaticMesh> duckMesh(hvk::createMeshFromGltf("resources/bottle/WaterBottle.gltf"));
		duckMesh->setUsingSRGMat(true);
        glm::mat4 duckTransform = glm::mat4(1.f);
        duckTransform = glm::scale(duckTransform, glm::vec3(0.1, 0.1f, 0.1f));
        mDuck = hvk::HVK_make_shared<hvk::StaticMeshRenderObject>(
			"Bottle",
			mScene,
            duckTransform, 
            duckMesh);
        //addStaticMeshInstance(mDuck);

		// ENTT experiments
		entt::entity modelEntity = mRegistry.create();

        PBRMesh duckPbrMesh;
        PBRMaterial duckPbrMaterial;
        getModelPipeline().loadAndFetchModel(duckMesh, "Duck", &duckPbrMesh, &duckPbrMaterial);
        mRegistry.assign<PBRMesh>(modelEntity, duckPbrMesh);
        mRegistry.assign<PBRMaterial>(modelEntity, duckPbrMaterial);

		mRegistry.assign<NodeTransform>(modelEntity, entt::null, duckTransform);

		//auto comp = mRegistry.get<PBRMaterial, PBRMesh>(modelEntity);
		//getMeshRenderer()->preparePBREntity(mRegistry, modelEntity);
		const auto& materialComp = mRegistry.get<PBRMaterial>(modelEntity);
		mRegistry.assign<PBRBinding>(modelEntity, mPBRMeshRenderer->createPBRBinding(materialComp));

		auto renderGroup = mRegistry.group<NodeTransform, PBRMesh, PBRBinding>();
		TestGroup(renderGroup);
		for (const auto& entity : renderGroup)
		{
			const auto& mesh = renderGroup.get<PBRMesh>(entity);
			const auto& transform = renderGroup.get<NodeTransform>(entity);
		}

		entt::entity lightEntity = mRegistry.create();

        //hvk::HVK_shared<hvk::StaticMesh> duckMesh(hvk::createMeshFromGltf("resources/bottle/WaterBottle.gltf"));
        //hvk::HVK_shared<hvk::StaticMesh> duckMesh(hvk::createMeshFromGltf("resources/FlightHelmet/FlightHelmet.gltf"));

        glm::mat4 lightTransform = glm::mat4(1.f);
        lightTransform = glm::scale(lightTransform, glm::vec3(0.1f));
        lightTransform = glm::translate(lightTransform, glm::vec3(3.f, 2.f, 1.5f));
		HVK_shared<DebugMesh> debugMesh = createColoredCube(glm::vec3(1.f, 1.f, 1.f));
		mLightBox = HVK_make_shared<DebugMeshRenderObject>(
			"Dynamic Light Box",
			mScene,
			//mDynamicLight->getTransform(), 
			lightTransform,
			debugMesh);
		//addDebugMeshInstance(mLightBox);

        mDynamicLight = hvk::HVK_make_shared<hvk::Light>(
			"Dynamic Light",
			mLightBox,
            glm::mat4(1.f), 
            glm::vec3(1.f, 1.f, 1.f), 0.3f);
        //addDynamicLight(mDynamicLight);
		mLightBox->addChild(mDynamicLight);

        mCameraController = CameraController(mCamera);

		mScene->addChild(mDuck);
		//mScene->addChild(mDynamicLight);
		mScene->addChild(mLightBox);
		mScene->addChild(mCamera);

        activateCamera(mCamera);

		//generateEnvironmentMap();

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
        io.KeyCtrl = hvk::InputManager::isDown(GLFW_KEY_LEFT_CONTROL);

        bool mouseClicked = mouse.leftDown && !prevMouse.leftDown;
        bool mouseReleased = prevMouse.leftDown && !mouse.leftDown;
        if (mouseClicked && !io.WantCaptureMouse) {
            toggleCursor(false);
        }
        if (mouseReleased) {
            toggleCursor(true);
        }

		if (mouse.leftDown && !io.WantCaptureMouse)
		{
			cameraDrag = true;
		}

        float sensitivity = 0.1f;
        float mouseDeltY = static_cast<float>(mouse.y - prevMouse.y);
        float mouseDeltX = static_cast<float>(prevMouse.x - mouse.x);

        std::vector<hvk::Command> cameraCommands;
        cameraCommands.reserve(6);

		// Don't allow keyboard inputs if the UI is capturing them
		if (!io.WantCaptureKeyboard)
		{
			if (hvk::InputManager::isDown(GLFW_KEY_W)) {
				cameraCommands.push_back(hvk::Command{ 0, "move_forward", -1.0f });
			}
			if (hvk::InputManager::isDown(GLFW_KEY_S)) {
				cameraCommands.push_back(hvk::Command{ 0, "move_forward", 1.0f });
			}
			if (hvk::InputManager::isDown(GLFW_KEY_A)) {
				cameraCommands.push_back(hvk::Command{ 1, "move_right", -1.0f });
			}
			if (hvk::InputManager::isDown(GLFW_KEY_D)) {
				cameraCommands.push_back(hvk::Command{ 1, "move_right", 1.0f });
			}
			if (hvk::InputManager::isDown(GLFW_KEY_Q)) {
				cameraCommands.push_back(hvk::Command{ 2, "move_up", -1.0f });
			}
			if (hvk::InputManager::isDown(GLFW_KEY_E)) {
				cameraCommands.push_back(hvk::Command{ 2, "move_up", 1.0f });
			}
			if (hvk::InputManager::isDown(GLFW_KEY_UP))
			{
				cameraCommands.push_back(hvk::Command{ 3, "camera_pitch", 0.25f });
			}
			if (hvk::InputManager::isDown(GLFW_KEY_DOWN))
			{

				cameraCommands.push_back(hvk::Command{ 3, "camera_pitch", -0.25f });
			}
			if (hvk::InputManager::isDown(GLFW_KEY_LEFT))
			{

				cameraCommands.push_back(hvk::Command{ 4, "camera_yaw", -0.25f });
			}
			if (hvk::InputManager::isDown(GLFW_KEY_RIGHT))
			{
				cameraCommands.push_back(hvk::Command{ 4, "camera_yaw", 0.25f });
			}
			if (hvk::InputManager::wasPressed(GLFW_KEY_I))
			{
				cameraCommands.push_back(hvk::Command{ 3, "camera_pitch", -90.f });
			}
			if (hvk::InputManager::wasPressed(GLFW_KEY_K))
			{
				cameraCommands.push_back(hvk::Command{ 3, "camera_pitch", 90.f });
			}
			if (hvk::InputManager::wasPressed(GLFW_KEY_J))
			{
				cameraCommands.push_back(hvk::Command{ 4, "camera_yaw", 90.f });
			}
			if (hvk::InputManager::wasPressed(GLFW_KEY_L))
			{
				cameraCommands.push_back(hvk::Command{ 4, "camera_yaw", -90.f });
			}
		}
        if (cameraDrag) {
            if (mouseDeltY != 0.f) {
                cameraCommands.push_back(hvk::Command{ 3, "camera_pitch", mouseDeltY * sensitivity });
            }
            if (mouseDeltX != 0.f) {
                cameraCommands.push_back(hvk::Command{ 4, "camera_yaw", mouseDeltX * sensitivity });
            }
        }

        mCameraController.update(frameTime, cameraCommands);

        // GUI
        ImGui::NewFrame();

		ImGui::Begin("Objects");
		//mDuck->displayGui();
		//mDynamicLight->displayGui();
		mScene->displayGui();
		ImGui::End();

        ImGui::Begin("Rendering");
  //      float gamma = getGammaCorrection();
		//auto gammaSettings = getGammaSettings();
  //      bool useSRGBTex = isUseSRGBTex();
  //      //ImGui::SliderFloat("Gamma", &gamma, 0.f, 10.f);
  //      ImGui::SliderFloat("Gamma", &gammaSettings->gamma, 0.f, 10.f);
  //      ImGui::Checkbox("Use sRGB Textures", &useSRGBTex);
  //      //setGammaCorrection(gamma);
  //      setUseSRGBTex(useSRGBTex);
		//ImGui::Text("Ambient Light");
		//auto ambientLight = getAmbientLight();
		////ambientLight->lightColor.r
		//ImGui::ColorEdit3("Color##Ambient", &(ambientLight->lightColor.r));
		//ImGui::SliderFloat("IntensityF##Ambient", &(ambientLight->lightIntensity), 0.f, 1.f);
		//auto pbrWeight = getPBRWeight();
		//ImGui::SliderFloat("Metallic", &pbrWeight->metallic, 0.f, 1.f);
		//ImGui::SliderFloat("Roughness", &pbrWeight->roughness, 0.f, 1.f);
		//auto exposureSettings = getExposureSettings();
		//ImGui::SliderFloat("Exposure", &exposureSettings->exposure, 0.f, 10.f);

		//auto skySettings = getSkySettings();
		//static int skyChoice = 0;
		//static float skyLod = 0.f;
		//bool skyChanged = false;
		//skyChanged = ImGui::RadioButton("Environment Map", &skyChoice, 1); ImGui::SameLine();
		//skyChanged |= ImGui::RadioButton("Irradiance Map", &skyChoice, 2);
		//skyChanged |= ImGui::RadioButton("Prefiltered Map", &skyChoice, 3);
		//if (skyChoice == 3)
		//{
		//	ImGui::SliderFloat("LOD", &skySettings->lod, 0.f, 9.f);
		//}
		//if (skyChanged)
		//{
		//	switch (skyChoice)
		//	{
		//	case 1:
		//		useEnvironmentMap();
		//		break;
		//	case 2:
		//		useIrradianceMap();
		//		break;
		//	case 3:
		//		usePrefilteredMap(skyLod);
		//	}
		//}

        ImGui::End();
		
		ImGui::Begin("Status");
		ImGui::Text("Mouse State");
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::Checkbox("LB", &mouse.leftDown);
		ImGui::SameLine();
		ImGui::Checkbox("RB", &mouse.rightDown);
		float mouseX = static_cast<float>(mouse.x);
		float mouseY = static_cast<float>(mouse.y);
		ImGui::InputFloat("X", &mouseX);
		ImGui::SameLine();
		ImGui::InputFloat("Y", &mouseY);
		ImGui::Text("Prev Mouse State");
		ImGui::Checkbox("LB##Prev", &mouse.leftDown);
		ImGui::SameLine();
		ImGui::Checkbox("RB##Prev", &mouse.rightDown);
		ImGui::PopItemFlag();
		ImGui::End();

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
