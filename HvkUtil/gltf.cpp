#include "pch.h"
#include "framework.h"

#include <iostream>

#include "gltf.h"
#define TINYGLTF_IMPLEMENTATION
#ifndef STB_IMAGE_IMPLEMENTATION
#endif
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#define _CRT_SECURE_NO_WARNINGS
#include "tiny_gltf.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace hvk
{
	tinygltf::TinyGLTF ModelLoader = tinygltf::TinyGLTF();

	void processGltfNode(
		tinygltf::Node& node,
		tinygltf::Model& model,
		std::vector<StaticMesh>& outMeshes)
	{
		if (node.mesh >= 0) {
			std::vector<Vertex> vertices;
			std::vector<VertIndex> indices;
			Material mat;

			tinygltf::Mesh mesh = model.meshes[node.mesh];
			for (size_t j = 0; j < mesh.primitives.size(); ++j) {
				tinygltf::Primitive prim = mesh.primitives[j];
				const auto positionAttr = prim.attributes.find("POSITION");
				const auto uvAttr = prim.attributes.find("TEXCOORD_0");
				const auto normalAttr = prim.attributes.find("NORMAL");
                const auto tangentAttr = prim.attributes.find("TANGENT");

				assert(positionAttr != prim.attributes.end());
				assert(uvAttr != prim.attributes.end());
				assert(normalAttr != prim.attributes.end());

				// process position data
				const tinygltf::Accessor positionAccess = model.accessors[positionAttr->second];
				assert(positionAccess.type == TINYGLTF_TYPE_VEC3);
				const size_t numPositions = positionAccess.count;
				const tinygltf::BufferView positionView = model.bufferViews[positionAccess.bufferView];
				const float* positionData = reinterpret_cast<const float*>(
					&(model.buffers[positionView.buffer].data[positionView.byteOffset + positionAccess.byteOffset]));

				// process UV data
				const tinygltf::Accessor uvAccess = model.accessors[uvAttr->second];
				assert(uvAccess.type == TINYGLTF_TYPE_VEC2);
				const tinygltf::BufferView uvView = model.bufferViews[uvAccess.bufferView];
				const float* uvData = reinterpret_cast<const float*>(
					&(model.buffers[uvView.buffer].data[uvView.byteOffset + uvAccess.byteOffset]));

				// process normal data
				const tinygltf::Accessor normalAccess = model.accessors[normalAttr->second];
				assert(normalAccess.type == TINYGLTF_TYPE_VEC3);
				const tinygltf::BufferView normalView = model.bufferViews[normalAccess.bufferView];
				const float* normalData = reinterpret_cast<const float*>(
					&(model.buffers[normalView.buffer].data[normalView.byteOffset + normalAccess.byteOffset]));

                // process tangent data
                float* tangentData = nullptr;
                if (tangentAttr != prim.attributes.end())
                {
                    const tinygltf::Accessor tangentAccess = model.accessors[tangentAttr->second];
                    assert(tangentAccess.type == TINYGLTF_TYPE_VEC4);
                    const tinygltf::BufferView tangentView = model.bufferViews[tangentAccess.bufferView];
                    tangentData = reinterpret_cast<float*>(&(model.buffers[tangentView.buffer].data[tangentView.byteOffset + tangentAccess.byteOffset]));
                }

				assert(positionAccess.count == uvAccess.count);

				//RenderObject::allocateVertices(numPositions, vertices);
				vertices.reserve(numPositions);

				for (size_t k = 0; k < numPositions; ++k) {
					Vertex v{};
					v.pos = glm::make_vec3(&positionData[k * 3]);
					v.normal = glm::make_vec3(&normalData[k * 3]);
					v.texCoord = glm::make_vec2(&uvData[k * 2]);
                    if (tangentData != nullptr)
                    {
                        v.tangent = glm::make_vec4(&tangentData[k * 4]);
                    }
					vertices.push_back(v);
				}

				// process indices
				const tinygltf::Accessor indexAccess = model.accessors[prim.indices];
				assert(indexAccess.type == TINYGLTF_TYPE_SCALAR);
				const size_t numIndices = indexAccess.count;
				const tinygltf::BufferView indexView = model.bufferViews[indexAccess.bufferView];
				// TODO: need to handle more than just uint16_t indices (unsigned short)
				const uint16_t * indexData = reinterpret_cast<const uint16_t*>(
					&(model.buffers[indexView.buffer].data[indexView.byteOffset + indexAccess.byteOffset]));

				//RenderObject::allocateIndices(numIndices, indices);
				indices.reserve(numIndices);

				for (size_t k = 0; k < numIndices; ++k) {
					indices.push_back(indexData[k]);
				}

				// process material
				//mat.diffuseProp.texture = Pool<tinygltf::Image>::alloc(model.images[model.materials[prim.material].pbrMetallicRoughness.baseColorTexture.index]);
                int diffuseIndex = model.materials[prim.material].pbrMetallicRoughness.baseColorTexture.index;
                int metallicRoughnessIndex = model.materials[prim.material].pbrMetallicRoughness.metallicRoughnessTexture.index;
                int normalIndex = model.materials[prim.material].normalTexture.index;
                if (diffuseIndex > -1) {
                    mat.diffuseProp.texture = HVK_shared<tinygltf::Image>(new tinygltf::Image(model.images[diffuseIndex]));
                }
                if (metallicRoughnessIndex > -1) {
                    mat.metallicRoughnessProp.texture = HVK_shared<tinygltf::Image>(new tinygltf::Image(model.images[metallicRoughnessIndex]));
                }
                if (normalIndex > -1) {
                    mat.normalProp.texture = HVK_shared<tinygltf::Image>(new tinygltf::Image(model.images[normalIndex]));
                }
			}

			outMeshes.emplace_back(vertices, indices, mat);
		}

		for (const int& childId : node.children) {
			tinygltf::Node& childNode = model.nodes[childId];
			processGltfNode(childNode, model, outMeshes);
		}
	}

	std::vector<StaticMesh> createMeshFromGltf(const std::string& filename)
	{
		std::vector<StaticMesh> meshes;

		tinygltf::Model model;
		std::string err, warn;

		bool modelLoaded = ModelLoader.LoadASCIIFromFile(&model, &err, &warn, filename);
		if (!err.empty()) {
			std::cout << err << std::endl;
		}
		if (!warn.empty()) {
			std::cout << warn << std::endl;
		}
		assert(modelLoaded);

		const tinygltf::Scene modelScene = model.scenes[model.defaultScene];
		for (const int& nodeId : modelScene.nodes) {
			tinygltf::Node& sceneNode = model.nodes[nodeId];
			processGltfNode(sceneNode, model, meshes);
		}

		return meshes;
	}
}
