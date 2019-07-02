// VulkanTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>

//#define GLFW_INCLUDE_VULKAN
//#include <GLFW\glfw3.h>

/*#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>*/

#include "vulkanapp.h"


const int HEIGHT = 600;
const int WIDTH = 800;

int main()
{
	hvk::VulkanApp app(WIDTH, HEIGHT, "Vulkan Test Window");
	app.init();
	app.run();
    // window loop
    /*while (!glfwWindowShouldClose(app.getWindow().get())) {
        glfwPollEvents();
    }*/

    return 0;
}

