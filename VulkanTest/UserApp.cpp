#include "UserApp.h"
#include "vulkanapp.h"

UserApp::UserApp(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle) :
	mApp(std::make_unique<hvk::VulkanApp>(windowWidth, windowHeight, windowTitle))
{
	mApp->init();
}

UserApp::~UserApp() = default;

void UserApp::runApp()
{
	run();
	mApp->run();
}

void UserApp::doClose()
{
	close();
}

