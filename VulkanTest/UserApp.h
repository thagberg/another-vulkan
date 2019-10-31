#pragma once

#include <memory>

#include "Clock.h"
#include "HvkUtil.h"
#include "RenderObject.h"
#include "Light.h"

namespace hvk
{
	class VulkanApp;
}

class UserApp
{
private:
	std::unique_ptr<hvk::VulkanApp> mApp;
    hvk::Clock mClock;

protected:
	virtual bool run(double frameTime) = 0;
	virtual void close() = 0;
	
public:
	UserApp(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle);
	virtual ~UserApp();

	void runApp();
	void doClose();
    void toggleCursor(bool enabled);
    void addStaticMeshInstance(hvk::HVK_shared<hvk::StaticMeshRenderObject> node);
    void addDynamicLight(hvk::HVK_shared<hvk::Light> light);
    void addDebugMeshInstance(hvk::HVK_shared<hvk::DebugMeshRenderObject> node);
};
