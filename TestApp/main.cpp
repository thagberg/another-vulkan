#include <iostream>
#include <variant>
#include <algorithm>

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
#include "DebugDrawGenerator.h"
#include "LightTypes.h"

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

void createWorldTransform(entt::entity entity, entt::registry& registry, NodeTransform& localTransform)
{
	registry.assign<WorldTransform>(entity, localTransform.transform);
}


class TestApp : public UserApp
{
private:
    CameraController mCameraController;
	bool mSceneDirty;
	entt::entity mSceneEntity;

	//void markSceneDirty(entt::entity entity, entt::registry& registry, SceneNode& sceneNode)
	void markSceneDirty()
	{
		mSceneDirty = true;
	}

	//void removeChild(entt::entity entity, entt::registry& registry, SceneNode& sceneNode)
	//void removeChild(entt::entity entity, entt::registry& registry)
	void removeChild(entt::entity entity)
	{
		auto& sceneNode = mRegistry.get<SceneNode>(entity);
		if (sceneNode.parent != entt::null)
		{
			auto& parentNode = mRegistry.get<SceneNode>(sceneNode.parent);
			auto found = std::find(parentNode.children.begin(), parentNode.children.end(), entity);
			parentNode.children.erase(found);
		}
	}

	void addChild(entt::entity entity)
	{
		auto& sceneNode = mRegistry.get<SceneNode>(entity);
		if (sceneNode.parent != entt::null)
		{
			auto& parentNode = mRegistry.get<SceneNode>(sceneNode.parent);
			parentNode.children.push_back(entity);
		}
	}

public:
	TestApp(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle) :
		UserApp(windowWidth, windowHeight, windowTitle),
        mCameraController(nullptr),
		mSceneDirty(false),
		mSceneEntity()
	{
		mRegistry.on_construct<SceneNode>().connect<&TestApp::markSceneDirty>(*this);
		mRegistry.on_destroy<SceneNode>().connect<&TestApp::markSceneDirty>(*this);
		mRegistry.on_construct<SceneNode>().connect<&TestApp::addChild>(*this);
		mRegistry.on_destroy<SceneNode>().connect<&TestApp::removeChild>(*this);
		mRegistry.on_replace<SceneNode>().connect<&TestApp::markSceneDirty>(*this);
		mRegistry.on_construct<NodeTransform>().connect<&createWorldTransform>();
		mRegistry.on_construct<NodeTransform>().connect<&entt::registry::assign_or_replace<WorldDirty>>(&mRegistry);
		mRegistry.on_replace<NodeTransform>().connect<&entt::registry::assign_or_replace<WorldDirty>>(&mRegistry);

        std::shared_ptr<hvk::StaticMesh> duckMesh(hvk::createMeshFromGltf("resources/bottle/WaterBottle.gltf"));
		duckMesh->setUsingSRGMat(true);
        glm::mat4 duckTransform = glm::mat4(1.f);

		// ENTT experiments
		mSceneEntity = mRegistry.create();
		entt::entity modelEntity = mRegistry.create();

		mRegistry.assign<SceneNode>(mSceneEntity, entt::null, "Scene");
		mRegistry.assign<NodeTransform>(mSceneEntity, glm::mat4(1.f));

        PBRMesh duckPbrMesh;
        PBRMaterial duckPbrMaterial;
        getModelPipeline().loadAndFetchModel(duckMesh, "Duck", &duckPbrMesh, &duckPbrMaterial);
        mRegistry.assign<PBRMesh>(modelEntity, duckPbrMesh);
        mRegistry.assign<PBRMaterial>(modelEntity, duckPbrMaterial);

		mRegistry.assign<SceneNode>(modelEntity, mSceneEntity, "Bottle");
		mRegistry.assign<NodeTransform>(modelEntity, duckTransform);

		const auto& materialComp = mRegistry.get<PBRMaterial>(modelEntity);
		mRegistry.assign<PBRBinding>(modelEntity, mPBRMeshRenderer->createPBRBinding(materialComp));

		entt::entity lightBoxHolder = mRegistry.create();
		auto lightBoxTransform = glm::mat4(1.f);
		mRegistry.assign<SceneNode>(lightBoxHolder, mSceneEntity, "LightHolder");
		mRegistry.assign<NodeTransform>(lightBoxHolder, lightBoxTransform);

		entt::entity boxEntity = mRegistry.create();
		auto boxTransform = glm::mat4(1.f);
		boxTransform = glm::scale(boxTransform, glm::vec3(.01f, .01f, .01f));
		auto boxMesh = createColoredCube(glm::vec3(1.f, 1.f, 1.f));
		DebugDrawMesh lightBoxMesh;
		getModelPipeline().loadAndFetchDebugModel(boxMesh, "cube", &lightBoxMesh);
		mRegistry.assign<DebugDrawMesh>(boxEntity, lightBoxMesh);
		mRegistry.assign<SceneNode>(boxEntity, lightBoxHolder, "LightBox");
		mRegistry.assign<NodeTransform>(boxEntity, boxTransform);
		mRegistry.assign<DebugDrawBinding>(boxEntity, mDebugRenderer->createDebugDrawBinding());
		
		entt::entity lightEntity = mRegistry.create();
		auto lightTransform = glm::mat4(1.f);
		mRegistry.assign<SceneNode>(lightEntity, lightBoxHolder, "Light");
		mRegistry.assign<NodeTransform>(lightEntity, lightTransform);
		mRegistry.assign<LightColor>(lightEntity, glm::vec3(188.f, 0.f, 255.f), 0.01f);

		// test triggering dirty flag
		NodeTransform& testTrans = mRegistry.get<NodeTransform>(modelEntity);
		mRegistry.assign_or_replace<NodeTransform>(modelEntity, glm::translate(testTrans.transform, glm::vec3(1.f, 0.f, 0.f)));

        mCameraController = CameraController(mCamera, 1.f);

        activateCamera(mCamera);
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

		// sort the scene?
		//registry.sort<relationship>([&registry](const entt::entity lhs, const entt::entity rhs) {
		//	const auto& clhs = registry.get<relationship>(lhs);
		//	const auto& crhs = registry.get<relationship>(rhs);
		//	return crhs.parent == lhs || clhs.next == rhs
		//		|| (!(clhs.parent == rhs || crhs.next == lhs) && (clhs.parent < crhs.parent || (clhs.parent == crhs.parent && &clhs < &crhs)));
		//	});
		if (mSceneDirty)
		{
			mRegistry.sort<SceneNode>([&](const entt::entity lhs, const entt::entity rhs) {
				const auto& sceneLeft = mRegistry.get<SceneNode>(lhs);
				const auto& sceneRight = mRegistry.get<SceneNode>(rhs);
				return sceneRight.parent == lhs || 
					(!(sceneLeft.parent == rhs) && (sceneLeft.parent < sceneRight.parent || (sceneLeft.parent == sceneRight.parent && lhs < rhs)));
				});
			mSceneDirty = false;
		}

        // GUI
        ImGui::NewFrame();

        ImGui::Begin("Rendering");
		ImGui::SliderFloat("Gamma", &mGammaSettings.gamma, 0.f, 10.f);
		ImGui::SliderFloat("Metallic", &mPBRWeight.metallic, 0.f, 1.f);
		ImGui::SliderFloat("Roughness", &mPBRWeight.roughness, 0.f, 1.f);
		ImGui::SliderFloat("Exposure", &mExposureSettings.exposure, 0.f, 10.f);
		ImGui::Text("Ambient Light");
		ImGui::ColorEdit3("Color##Ambient", &mAmbientLight.lightColor.r);
		ImGui::SliderFloat("Intensity##Ambient", &mAmbientLight.lightIntensity, 0.f, 10.f);

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

		entt::entity activeEntity = entt::null;
		ImGui::Begin("Objects");
		auto& sceneNode = mRegistry.get<SceneNode>(mSceneEntity);
		if (ImGui::TreeNode(&mSceneEntity, sceneNode.name.c_str()))
		{
			auto& children = sceneNode.children;
			for (auto& child : children)
			{
				auto& childNode = mRegistry.get<SceneNode>(child);
				if (ImGui::TreeNode(&child, childNode.name.c_str()))
				{
					activeEntity = child;
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
		//auto sceneView = mRegistry.view<SceneNode>();
		//sceneView.each([&](auto entity, const auto& sceneNode) {
		//	std::cout << entt::to_integer(entity) << std::endl;
		//});
		ImGui::End();

		if (activeEntity != entt::null)
		{
			ImGui::Begin("Current Object");

			ImGui::End();
		}
		
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
