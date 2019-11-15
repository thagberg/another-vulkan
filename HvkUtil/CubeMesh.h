#pragma once

#include "HvkUtil.h"

namespace hvk
{
    class CubeMesh
    {
    public:
        using CubeVertices = HVK_shared<HVK_vector<CubeVertex>>;
        using Indices = HVK_shared<HVK_vector<VertIndex>>;

    private:
        CubeVertices mVertices;
        Indices mIndices;

    public:
        CubeMesh(CubeVertices vertices, Indices indices);
        const CubeVertices getVertices() const { return mVertices; }
        const Indices getIndices() const { return mIndices; }
        virtual ~CubeMesh();
    };
}
