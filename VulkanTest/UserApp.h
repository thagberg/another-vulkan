#pragma once

#include <memory>

namespace hvk
{
	class VulkanApp;
}

class UserApp
{
private:
	std::unique_ptr<hvk::VulkanApp> mApp;

protected:
	virtual void run() = 0;
	virtual void close() = 0;
	
public:
	UserApp(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle);
	virtual ~UserApp();

	void runApp();
	void doClose();
};
