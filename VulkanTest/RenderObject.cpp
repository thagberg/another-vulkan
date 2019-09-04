#include "stdafx.h"
#include "RenderObject.h"

#include <glm/gtc/type_ptr.hpp>

namespace hvk {

	tinygltf::TinyGLTF RenderObject::sModelLoader = tinygltf::TinyGLTF();

	void RenderObject::allocateVertices(size_t reservedSize, VerticesRef vertices) {
		if (!vertices) {
			vertices = std::make_shared<std::vector<Vertex>>();
		}
		vertices->reserve(reservedSize);
	}

	void RenderObject::allocateIndices(size_t reservedSize, IndicesRef indices) {
		if (!indices) {
			indices = std::make_shared<std::vector<VertIndex>>();
		}
		indices->reserve(reservedSize);
	}

	RenderObject::RenderObject(
        NodeRef parent, 
        glm::mat4 transform,
        VerticesRef vertices,
        IndicesRef indices,
		TextureRef texture) : 
        
        Node(parent, transform),
        mVertices(vertices),
        mIndices(indices),
		mTexture(texture),
		mSpecularStrength(0.f)
	{
	}

	RenderObject::~RenderObject()
	{
	}

	const VerticesRef RenderObject::getVertices() const {
		return mVertices;
	}

	const IndicesRef RenderObject::getIndices() const {
		return mIndices;
	}

	const TextureRef RenderObject::getTexture() const {
		return mTexture;
	}

	float RenderObject::getSpecularStrength() const {
		return mSpecularStrength;
	}

	void RenderObject::setSpecularStrength(float specularStrength) {
		mSpecularStrength = specularStrength;
	}

	void RenderObject::processGltfNode(
		VerticesRef vertices, 
		IndicesRef indices, 
		tinygltf::Image& texture,
		tinygltf::Node& node, 
		tinygltf::Model& model) {

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
				const int numPositions = positionAccess.count;
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
				vertices->reserve(numPositions);

				for (size_t k = 0; k < numPositions; ++k) {
					Vertex v{};
					v.pos = glm::make_vec3(&positionData[k * 3]);
					v.normal = glm::make_vec3(&normalData[k * 3]);
					v.texCoord = glm::make_vec2(&uvData[k * 2]);
					vertices->push_back(v);
				}

				// process indices
				const tinygltf::Accessor indexAccess = model.accessors[prim.indices];
				assert(indexAccess.type == TINYGLTF_TYPE_SCALAR);
				const int numIndices = indexAccess.count;
				const tinygltf::BufferView indexView = model.bufferViews[indexAccess.bufferView];
				// TODO: need to handle more than just uint16_t indices (unsigned short)
				const uint16_t* indexData = reinterpret_cast<const uint16_t*>(
					&(model.buffers[indexView.buffer].data[indexView.byteOffset + indexAccess.byteOffset]));

				//RenderObject::allocateIndices(numIndices, indices);
				indices->reserve(numIndices);

				for (size_t k = 0; k < numIndices; ++k) {
					indices->push_back(indexData[k]);
				}

				// process material
				texture = model.images[model.materials[prim.material].pbrMetallicRoughness.baseColorTexture.index];
			}
		}

		for (int i = 0; i < node.children.size(); ++i) {
			processGltfNode(vertices, indices, texture, model.nodes[node.children[i]], model);
		}
	}

	RenderObjRef RenderObject::createFromGltf(const std::string& gltfFilename, NodeRef parent, glm::mat4 transform) {
		tinygltf::Model model;
		std::string err, warn;

		bool modelLoaded = sModelLoader.LoadASCIIFromFile(&model, &err, &warn, gltfFilename);
		if (!err.empty()) {

		}
		if (!warn.empty()) {

		}
		assert(modelLoaded);

        // traverse the model and create RenderObjects
		//VerticesRef modelVertices = RenderObject::allocateVertices();
		//IndicesRef modelIndices = RenderObject::allocateIndices();
		VerticesRef modelVertices = std::make_shared<std::vector<Vertex>>();
		IndicesRef modelIndices = std::make_shared<std::vector<VertIndex>>();
		tinygltf::Image texture;
        const tinygltf::Scene modelScene = model.scenes[model.defaultScene];
        // TODO: need to do this recursively for child nodes
        for (size_t i = 0; i < modelScene.nodes.size(); ++i) {
            tinygltf::Node sceneNode = model.nodes[modelScene.nodes[i]];
			// process any nodes with mesh
			processGltfNode(modelVertices, modelIndices, texture, sceneNode, model);
        }

		return std::make_shared<RenderObject>(
			parent, 
			transform, 
			modelVertices, 
			modelIndices, 
			std::make_shared<tinygltf::Image>(texture));
	}
}
