#include <iostream>
#include <variant>
#include <algorithm>

#define HVK_TOOLS 1

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
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
#include "math-util.h"
#include "ToolsTypes.h"

using namespace hvk;

const uint32_t HEIGHT = 1024;
const uint32_t WIDTH = 1024;

const glm::vec3 X_AXIS = glm::vec3(-1.f, 0.f, 0.f);
const glm::vec3 Y_AXIS = glm::vec3(0.f, -1.f, 0.f);
const glm::vec3 Z_AXIS = glm::vec3(0.f, 0.f, -1.f);


void createWorldTransform(entt::entity entity, entt::registry& registry, NodeTransform& localTransform)
{
	registry.assign<WorldTransform>(entity, localTransform.transform);
}

void createEditorRotation(entt::entity entity, entt::registry& registry, NodeTransform& localTransform)
{
	registry.assign<EditorRotation>(entity, glm::vec3(0.f));
}


class TestApp : public UserApp
{
private:
    CameraController mCameraController;
	bool mSceneDirty;
	entt::entity mModelEntity;

	void markSceneDirty()
	{
		mSceneDirty = true;
	}

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

	void dirtyTree(entt::entity entity)
	{
		auto& sceneNode = mRegistry.get<SceneNode>(entity);
		for (auto& child : sceneNode.children)
		{
			mRegistry.assign_or_replace<WorldDirty>(child);
		}
	}

	void iterateChildren(entt::entity parent, const SceneNode& parentNode, entt::entity& activeEntity)
	{
		const auto& children = parentNode.children;
		for (const auto& child : children)
		{
			const auto& childNode = mRegistry.get<SceneNode>(child);
			if (ImGui::TreeNode(&child, childNode.name.c_str()))
			{
				activeEntity = child;
				iterateChildren(child, childNode, activeEntity);
				ImGui::TreePop();
			}
		}
	}


public:
	TestApp(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle) :
		UserApp(windowWidth, windowHeight, windowTitle),
        mCameraController(nullptr),
		mSceneDirty(false)
	{
		mRegistry.on_construct<SceneNode>().connect<&TestApp::markSceneDirty>(*this);
		mRegistry.on_destroy<SceneNode>().connect<&TestApp::markSceneDirty>(*this);
		mRegistry.on_construct<SceneNode>().connect<&TestApp::addChild>(*this);
		mRegistry.on_destroy<SceneNode>().connect<&TestApp::removeChild>(*this);
		mRegistry.on_replace<SceneNode>().connect<&TestApp::markSceneDirty>(*this);
		mRegistry.on_construct<NodeTransform>().connect<&createWorldTransform>();
		mRegistry.on_construct<NodeTransform>().connect<&createEditorRotation>();
		mRegistry.on_construct<NodeTransform>().connect<&entt::registry::assign_or_replace<WorldDirty>>(&mRegistry);
		mRegistry.on_replace<NodeTransform>().connect<&entt::registry::assign_or_replace<WorldDirty>>(&mRegistry);
		mRegistry.on_construct<WorldDirty>().connect<&TestApp::dirtyTree>(*this);

        StaticMesh duckMesh(hvk::createMeshFromGltf("resources/bottle/WaterBottle.gltf")[0]);
		duckMesh.setUsingSRGMat(true);
        glm::mat4 duckTransform = glm::mat4(1.f);
		StaticMesh staticBoxMesh(hvk::createMeshFromGltf("resources/box/box.gltf")[0]);

		mModelEntity = mRegistry.create();

		mRegistry.assign<SceneNode>(mSceneEntity, entt::null, "Scene");
		mRegistry.assign<NodeTransform>(mSceneEntity, glm::mat4(1.f));

		// Model entity
        PBRMesh duckPbrMesh;
        PBRMaterial duckPbrMaterial;
		entt::entity modelEntity = mModelEntity;
		mRegistry.assign<SceneNode>(modelEntity, mSceneEntity, "Bottle");
		mRegistry.assign<NodeTransform>(modelEntity, duckTransform);
        getModelPipeline().loadAndFetchModel(duckMesh, "Duck", duckPbrMesh, duckPbrMaterial);
        mRegistry.assign<PBRMesh>(modelEntity, duckPbrMesh);
        mRegistry.assign<PBRMaterial>(modelEntity, duckPbrMaterial);
		const auto& materialComp = mRegistry.get<PBRMaterial>(modelEntity);
		mRegistry.assign<PBRBinding>(modelEntity, mPBRMeshRenderer->createPBRBinding(materialComp));

		// Point light holder
		entt::entity lightBoxHolder = mRegistry.create();
		auto lightBoxTransform = glm::mat4(1.f);
		mRegistry.assign<SceneNode>(lightBoxHolder, mSceneEntity, "LightHolder");
		mRegistry.assign<NodeTransform>(lightBoxHolder, lightBoxTransform);

		// Point light box
		entt::entity boxEntity = mRegistry.create();
		auto boxTransform = glm::mat4(1.f);
		boxTransform = glm::scale(boxTransform, glm::vec3(.01f, .01f, .01f));
		auto boxMesh = createColoredCube(glm::vec3(1.f, 1.f, 1.f));
		DebugDrawMesh lightBoxMesh;
		getModelPipeline().loadAndFetchDebugModel(*boxMesh, "cube", lightBoxMesh);
		mRegistry.assign<DebugDrawMesh>(boxEntity, lightBoxMesh);
		mRegistry.assign<SceneNode>(boxEntity, lightBoxHolder, "LightBox");
		mRegistry.assign<NodeTransform>(boxEntity, boxTransform);
		mRegistry.assign<DebugDrawBinding>(boxEntity, mDebugRenderer->createDebugDrawBinding());
		
		// Point light
		entt::entity lightEntity = mRegistry.create();
		auto lightTransform = glm::mat4(1.f);
		mRegistry.assign<SceneNode>(lightEntity, lightBoxHolder, "Light");
		mRegistry.assign<NodeTransform>(lightEntity, lightTransform);
		mRegistry.assign<LightColor>(lightEntity, glm::vec3(188.f, 0.f, 255.f), 0.01f);

		// Floor
		PBRMesh boxPbrMesh;
		PBRMaterial boxPbrMaterial;
		entt::entity floorEntity = mRegistry.create();
		auto floorTransform = glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(10.f, 0.1f, 10.f)), glm::vec3(0.f, -2.5f, 0.f));
		mRegistry.assign<SceneNode>(floorEntity, mSceneEntity, "Floor");
		mRegistry.assign<NodeTransform>(floorEntity, floorTransform);
		getModelPipeline().loadAndFetchModel(staticBoxMesh, "boxMesh", boxPbrMesh, boxPbrMaterial);
		mRegistry.assign<PBRMesh>(floorEntity, boxPbrMesh);
		const auto& boxMaterialComp = mRegistry.assign<PBRMaterial>(floorEntity, boxPbrMaterial);
		mRegistry.assign<PBRBinding>(floorEntity, mPBRMeshRenderer->createPBRBinding(boxMaterialComp));

		// directional light
		mRegistry.assign<SceneNode>(mSkyEntity, mSceneEntity, "DirectionalLight");
		mRegistry.assign<LightColor>(mSkyEntity, glm::vec3(250.f, 211.f, 57.f), 1.f);
		mRegistry.assign<Direction>(mSkyEntity, glm::vec3(3.f, -4.f, -1.f));

        mCameraController = CameraController(mCamera, 1.f);
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
        float mouseDeltY = -static_cast<float>(mouse.y - prevMouse.y);
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
				cameraCommands.push_back(hvk::Command{ 2, "move_up", 1.0f });
			}
			if (hvk::InputManager::isDown(GLFW_KEY_E)) {
				cameraCommands.push_back(hvk::Command{ 2, "move_up", -1.0f });
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

		// test some rotation stuff
		//const auto& modelTransform = mRegistry.get<NodeTransform>(mModelEntity).transform;
		//auto newTransform = glm::rotate(modelTransform, 0.001f, X_AXIS);
		//newTransform = glm::rotate(newTransform, 0.001f, Y_AXIS);
		//mRegistry.replace<NodeTransform>(mModelEntity, newTransform);

		// sort the scene?
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

		// update world transforms
		auto dirtyObjects = mRegistry.group<WorldDirty>(entt::get<SceneNode, NodeTransform, WorldTransform>);
		dirtyObjects.each([&](
			auto entity, 
			const auto& dirty, 
			const auto& sceneNode, 
			const auto& localTransform, 
			const auto& worldTransform) {

				mRegistry.remove<WorldDirty>(entity);
				glm::mat4 newWorld = localTransform.transform;
				const auto& parent = sceneNode.parent;
				if (parent != entt::null)
				{
					const auto& parentTrans = mRegistry.get<WorldTransform>(parent);
					newWorld = parentTrans.transform * newWorld;
				}
				mRegistry.replace<WorldTransform>(entity, newWorld);
		});

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
			iterateChildren(mSceneEntity, sceneNode, activeEntity);
			ImGui::TreePop();
		}
		ImGui::End();

		if (activeEntity != entt::null)
		{
			auto& sceneNode = mRegistry.get<SceneNode>(activeEntity);
			std::string windowLabel = sceneNode.name + "###ObjectPanel";
			ImGui::Begin(windowLabel.c_str());

			if (mRegistry.has<NodeTransform>(activeEntity))
			{
				auto& transform = mRegistry.get<NodeTransform>(activeEntity).transform;
				auto translation = glm::vec3(transform[3]);

				bool changed = false;
				std::string label = "Position##" + sceneNode.name;
				changed |= ImGui::DragFloat3(label.c_str(), &translation.x, 0.1f);

				glm::quat rotQuat;
				if (mRegistry.has<EditorRotation>(activeEntity))
				{
					auto rotation = mRegistry.get<EditorRotation>(activeEntity);
					ImGui::Text("Rotation");

					// get angles from transform
					glm::vec3 angles = util::math::getEulerAnglesFromMatrix(transform);

					changed |= ImGui::SliderAngle("X##ObjectRotation", &rotation.eulerRotation.x, -360.f, 360.f);
					changed |= ImGui::SliderAngle("Y##ObjectRotation", &rotation.eulerRotation.y, -360.f, 360.f);
					changed |= ImGui::SliderAngle("Z##ObjectRotation", &rotation.eulerRotation.z, -360.f, 360.f);

					if (changed)
					{
						mRegistry.replace<EditorRotation>(activeEntity, rotation.eulerRotation);
					}

					auto pitch = glm::angleAxis(rotation.eulerRotation.x, X_AXIS);
					auto yaw = glm::angleAxis(rotation.eulerRotation.y, Y_AXIS);
					auto roll = glm::angleAxis(rotation.eulerRotation.z, Z_AXIS);
					rotQuat = pitch * yaw * roll;
				}

				if (changed)
				{
					// rebuild transform matrix
					auto trans = glm::translate(glm::mat4(1.f), translation);

					mRegistry.replace<NodeTransform>(activeEntity, trans * glm::mat4_cast(rotQuat));
				}

				auto newTrans = mRegistry.get<NodeTransform>(activeEntity).transform;
				ImGui::InputFloat3("X Axis", &newTrans[0].x, 3);
				ImGui::InputFloat3("Y Axis", &newTrans[1].x, 3);
				ImGui::InputFloat3("Z Axis", &newTrans[2].x, 3);
			}

			if (mRegistry.has<LightColor>(activeEntity))
			{
				auto& lightColor = mRegistry.get<LightColor>(activeEntity);
				auto& color = lightColor.color;
				float intensity = lightColor.intensity;
				std::string colorLabel = "Color##" + sceneNode.name;
				std::string intensityLabel = "Intensity##" + sceneNode.name;
				bool colorChanged = ImGui::ColorEdit3(colorLabel.c_str(), &color.r);
				bool intensityChanged = ImGui::SliderFloat(intensityLabel.c_str(), &intensity, 0.f, 1.f);
				if (colorChanged || intensityChanged)
				{
					mRegistry.replace<LightColor>(activeEntity, color, intensity);
				}
			}

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
