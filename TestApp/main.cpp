#include <iostream>

#include "UserApp.h"

const uint32_t HEIGHT = 1024;
const uint32_t WIDTH = 1024;

class TestApp : public UserApp
{
public:
	TestApp(uint32_t windowWidth, uint32_t windowHeight, const char* windowTitle) :
		UserApp(windowWidth, windowHeight, windowTitle)
	{
		
	}

	virtual ~TestApp() = default;

protected:
	void run() override
	{
		std::cout << "Running Test App" << std::endl;
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
