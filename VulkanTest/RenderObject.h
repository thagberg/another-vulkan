#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include "tiny_gltf.h"

#include "types.h"
#include "Node.h"

namespace hvk {

    typedef std::shared_ptr<std::vector<Vertex>> VerticesRef;
    typedef std::shared_ptr<std::vector<VertIndex>> IndicesRef;
	typedef std::shared_ptr<tinygltf::Image> TextureRef;

	class RenderObject;
	typedef std::shared_ptr<RenderObject> RenderObjRef;

	class RenderObject : public Node
	{
    private:
        VerticesRef mVertices;
        IndicesRef mIndices;
		TextureRef mTexture;
		float mSpecularStrength;

		static tinygltf::TinyGLTF sModelLoader;

		static void processGltfNode(
			VerticesRef vertices, 
			IndicesRef indices, 
			tinygltf::Image& texture, 
			tinygltf::Node& node, 
			tinygltf::Model& model);

	public:
		RenderObject(
            NodeRef parent, 
            glm::mat4 transform, 
            VerticesRef vertices, 
            IndicesRef indices,
			TextureRef texture);
		~RenderObject();

		const VerticesRef getVertices() const;
		const IndicesRef getIndices() const;
		const TextureRef getTexture() const;
		float getSpecularStrength() const;
		void setSpecularStrength(float specularStrength);


		static void allocateVertices(size_t reservedSize, VerticesRef vertices);
		static void allocateIndices(size_t reservedSize, IndicesRef indices);
		static RenderObjRef createFromGltf(
			const std::string& gltfFilename, 
			NodeRef parent, 
			glm::mat4 transform);
	};
}
