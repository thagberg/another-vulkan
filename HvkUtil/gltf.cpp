#include "pch.h"
#include "framework.h"

#include <iostream>

#include "gltf.h"
#include "tiny_gltf.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace hvk
{
	tinygltf::TinyGLTF ModelLoader = tinygltf::TinyGLTF();

	void processGltfNode(
		tinygltf::Node& node,
		tinygltf::Model& model,
		HVK_vector<Vertex>& vertices,
		HVK_vector<VertIndex>& indices,
		Material& mat)
	{
		if (node.mesh >= 0) {
			tinygltf::Mesh mesh = model.meshes[node.mesh];
			for (size_t j = 0; j < mesh.primitives.size(); ++j) {
				tinygltf::Primitive prim = mesh.primitives[j];
				const auto positionAttr = prim.attributes.find("POSITION");
				const auto uvAttr = prim.attributes.find("TEXCOORD_0");
				const auto normalAttr = prim.attributes.find("NORMAL");

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

				assert(positionAccess.count == uvAccess.count);

				//RenderObject::allocateVertices(numPositions, vertices);
				vertices.reserve(numPositions);

				for (size_t k = 0; k < numPositions; ++k) {
					Vertex v{};
					v.pos = glm::make_vec3(&positionData[k * 3]);
					v.normal = glm::make_vec3(&normalData[k * 3]);
					v.texCoord = glm::make_vec2(&uvData[k * 2]);
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
				mat.diffuseProp.texture = new tinygltf::Image(model.images[model.materials[prim.material].pbrMetallicRoughness.baseColorTexture.index]);
			}
		}

		for (const int& childId : node.children) {
			tinygltf::Node& childNode = model.nodes[childId];
			processGltfNode(childNode, model, vertices, indices, mat);
		}
	}

	std::unique_ptr<StaticMesh, void(*)(StaticMesh*)> createMeshFromGltf(const std::string& filename)
	{
		// TODO: use a real allocation system instead of "new"
		//std::vector<Vertex>* vertices = new std::vector<Vertex>();
		//std::shared_ptr<std::vector<Vertex, Hallocator<Vertex>>> vertices = std::make_shared<std::vector<Vertex, Hallocator<Vertex>>>();
		HVK_shared<HVK_vector<Vertex>> vertices = HVK_make_shared<HVK_vector<Vertex>>();
		HVK_shared<HVK_vector<VertIndex>> indices = HVK_make_shared<HVK_vector<VertIndex>>();
		//std::vector<VertIndex>* indices = new std::vector<VertIndex>();
		//std::shared_ptr<std::vector<VertIndex>> indices = std::make_shared<std::vector<VertIndex>>();
		HVK_shared<Material> mat = HVK_make_shared<Material>();
		//Material* mat = Pool<Material>::alloc();

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
			processGltfNode(sceneNode, model, *vertices.get(), *indices.get(), *mat.get());
		}

		// TODO: this is going to result in a memory leak!!!  See above comment
		//return StaticMesh(*vertices, *indices, *mat);
		return Pool<StaticMesh>::alloc(vertices, indices, mat);
	}
}
