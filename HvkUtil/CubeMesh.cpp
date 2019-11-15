#include "pch.h"
#include "CubeMesh.h"


namespace hvk
{
    CubeMesh::CubeMesh(CubeVertices vertices, Indices indices) :
        mVertices(vertices),
        mIndices(indices)
    {
    }


    CubeMesh::~CubeMesh()
    {
    }
}
