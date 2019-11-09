#pragma once

#include <array>
#include "HvkUtil.h"

namespace hvk
{
    std::array<glm::vec3, 8> cubePositions = {
        glm::vec3(-1.f, -1.f, 1.f),
        glm::vec3(1.f, -1.f, 1.f),
        glm::vec3(-1.f, 1.f, 1.f),
        glm::vec3(1.f, 1.f, 1.f),
        glm::vec3(-1.f, -1.f, -1.f),
        glm::vec3(1.f, -1.f, -1.f),
        glm::vec3(-1.f, 1.f, -1.f),
        glm::vec3(1.f, 1.f, -1.f)
    };

    std::array<VertIndex, 36> cubeIndices = {
        0,1,2,
        2,1,3,
        3,7,2,
        2,7,6,
        7,5,4,
        7,4,6,
        5,1,0,
        5,0,4,
        3,1,7,
        7,1,5,
        2,4,0,
        4,2,6
    };

    std::array<glm::vec2, 4> cubeUVs = {
        glm::vec2(0.f, 0.f),
        glm::vec2(1.f, 0.f),
        glm::vec2(0.f, 1.f),
        glm::vec2(1.f, 1.f)
    };
}
