#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include "tiny_gltf.h"

#include "types.h"
#include "Node.h"
#include "StaticMesh.h"
#include "DebugMesh.h"
#include "CubeMesh.h"

namespace hvk {

    class CubeMeshRenderObject : public Node
    {
    private:
        HVK_shared<CubeMesh> mMesh;
    public:
        CubeMeshRenderObject(
            std::string name,
            HVK_shared<Node> parent,
            glm::mat4 transform,
            HVK_shared<CubeMesh> mesh);
        CubeMeshRenderObject(
            std::string name,
            HVK_shared<Node> parent,
            HVK_shared<Transform> transform,
            HVK_shared<CubeMesh> mesh);

        const CubeMesh::CubeVertices getVertices() const;
        const CubeMesh::Indices getIndices() const;
    };
}
