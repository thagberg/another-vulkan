// VulkanTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "vulkanapp.h"


const int HEIGHT = 600;
const int WIDTH = 800;

int main()
{
	hvk::VulkanApp app(WIDTH, HEIGHT, "Vulkan Test Window");
	app.init();
	app.run();

    return 0;
}

