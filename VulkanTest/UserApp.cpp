#include "UserApp.h"
#include "vulkanapp.h"
#include "InputManager.h"

UserApp::UserApp(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle) :
	mApp(std::make_unique<hvk::VulkanApp>(windowWidth, windowHeight, windowTitle)),
    mClock()
{
	mApp->init();
}

UserApp::~UserApp() = default;

void UserApp::runApp()
{
    double frameTime = 0.f;
    bool shouldClose = false;
    while (!shouldClose)
    {
        mClock.start();
		frameTime = mClock.getDelta();


        shouldClose |= run(frameTime);
        shouldClose |= mApp->update(frameTime);
        mClock.end();
    }
}

void UserApp::doClose()
{
	close();
}

void UserApp::toggleCursor(bool enabled)
{
    mApp->toggleCursor(enabled);
}


void UserApp::addStaticMeshInstance(hvk::HVK_shared<hvk::StaticMeshRenderObject> node)
{
    mApp->addStaticMeshInstance(node);
}

void UserApp::addDynamicLight(hvk::HVK_shared<hvk::Light> light)
{
    mApp->addDynamicLight(light);
}

void UserApp::addDebugMeshInstance(hvk::HVK_shared<hvk::DebugMeshRenderObject> node)
{
    mApp->addDebugMeshInstance(node);
}
