#pragma once
#include <memory>
#include <vector>
#include <variant>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "ResourceManager.h"
//#include "StaticMesh.h"
#include "tiny_gltf.h"
#include "imgui/imgui.h"


namespace hvk
{
	#define COMP3_4_ALIGN(t) alignas(4*sizeof(t))
	#define COMP2_ALIGN(t) alignas(2*sizeof(t))
	#define COMP1_ALIGN(t) alignas(sizeof(t))

	using VertIndex = uint16_t;

	const VkFormat VertexPositionFormat = VK_FORMAT_R32G32B32_SFLOAT;
	const VkFormat VertexColorFormat = VK_FORMAT_R32G32B32_SFLOAT;
	const VkFormat VertexUVFormat = VK_FORMAT_R32G32_SFLOAT;
	const VkFormat VertexNormalFormat = VK_FORMAT_R32G32B32_SFLOAT;
    const VkFormat VertexTangentFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    const VkFormat VertexUVWFormat = VK_FORMAT_R32G32B32_SFLOAT;

	template <typename T>
	using HVK_shared = std::shared_ptr<T>;

    template <typename T>
    using HVK_unique = std::unique_ptr<T, void(*)(T*)>;

	template <typename T, typename... Args>
	HVK_shared<T> HVK_make_shared(Args&& ... args) {
		Hallocator<T> alloc;
		return std::allocate_shared<T>(alloc, args...);
	}

	template <typename T>
	using HVK_vector = std::vector<T>;
	//using HVK_vector = std::vector<T, Hallocator<T>>;


	struct MaterialProperty {
		HVK_shared<tinygltf::Image> texture;
		float scale;
	};

	struct Material {
		MaterialProperty diffuseProp;
		MaterialProperty metallicRoughnessProp;
        MaterialProperty normalProp;
		bool sRGB;
	};

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 texCoord;
        glm::vec4 tangent;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
			attributeDescriptions.resize(4);

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VertexPositionFormat;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VertexNormalFormat;
			attributeDescriptions[1].offset = offsetof(Vertex, normal);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VertexUVFormat;
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VertexTangentFormat;
            attributeDescriptions[3].offset = offsetof(Vertex, tangent);

			return attributeDescriptions;
		}
	};

	struct ColorVertex {
		glm::vec3 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(ColorVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
			attributeDescriptions.resize(2);

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VertexPositionFormat;
			attributeDescriptions[0].offset = offsetof(ColorVertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VertexColorFormat;
			attributeDescriptions[1].offset = offsetof(ColorVertex, color);

			return attributeDescriptions;
		}
	};

    struct CubeVertex {
        glm::vec3 pos;
        glm::vec3 texCoord;

        static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(CubeVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
			attributeDescriptions.resize(2);

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VertexPositionFormat;
			attributeDescriptions[0].offset = offsetof(CubeVertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VertexUVWFormat;
			attributeDescriptions[1].offset = offsetof(CubeVertex, texCoord);

			return attributeDescriptions;
		}
    };

	struct Command {
		uint16_t id;
		std::string name;
		std::variant<uint32_t, float, bool> payload;
	};
}
