#include "pch.h"
#include "RenderObject.h"

#include <glm/gtc/type_ptr.hpp>

namespace hvk {

    CubeMeshRenderObject::CubeMeshRenderObject(
        std::string name,
        HVK_shared<Node> parent,
        glm::mat4 transform,
        HVK_shared<CubeMesh> mesh) :

        Node(name, parent, transform),
        mMesh(mesh)
    {

    }

    CubeMeshRenderObject::CubeMeshRenderObject(
        std::string name,
        HVK_shared<Node> parent,
        HVK_shared<Transform> transform,
        HVK_shared<CubeMesh> mesh) :

        Node(name, parent, transform),
        mMesh(mesh)
    {

    }
}
