#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "types.h"
#include "Node.h"

namespace hvk {
	class RenderObject;
	typedef std::shared_ptr<RenderObject> RenderObjRef;

	class RenderObject : Node
	{
	private:
		Resource<VkImage> mTexture;
		Resource<VkBuffer> mVertexBuffer;
		Resource<VkBuffer> mIndexBuffer;
		Resource<VkBuffer> mUniformBuffer;

		VkImageView mTextureView;
		VkSampler mTextureSampler;

		VkDescriptorSet mDescriptorSet;

	public:
		RenderObject(NodeRef parent, glm::mat4 transform);
		~RenderObject();
	};
}
