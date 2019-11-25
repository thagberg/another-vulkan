#include "pch.h"
#include "descriptor-util.h"


namespace hvk
{
	namespace util
	{
		namespace descriptor
		{
			void createDescriptorPool(
				VkDevice device,
				std::vector<VkDescriptorPoolSize>& poolSizes,
				uint32_t maxSets,
				VkDescriptorPool& pool)
			{
				VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
				poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
				poolInfo.pPoolSizes = poolSizes.data();
				poolInfo.maxSets = maxSets;

				assert(vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) == VK_SUCCESS);
			}


			void createDescriptorSetLayout(
				VkDevice device,
				std::vector<VkDescriptorSetLayoutBinding>& bindings,
				VkDescriptorSetLayout& descriptorSetLayout)
			{
				VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
				layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
				layoutInfo.pBindings = bindings.data();

				assert(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) == VK_SUCCESS);
			}


			VkDescriptorSetLayoutBinding generateUboLayoutBinding(
				uint32_t binding,
				uint32_t descriptorCount,
				VkShaderStageFlags flags)
			{
				return VkDescriptorSetLayoutBinding {
					binding,
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					descriptorCount,
					flags,
					nullptr
				};
			}


			VkDescriptorSetLayoutBinding generateSamplerLayoutBinding(
				uint32_t binding,
				uint32_t descriptorCount,
				VkShaderStageFlags flags)
			{
				return VkDescriptorSetLayoutBinding{
					binding,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					descriptorCount,
					flags,
					nullptr
				};
			}


			void allocateDescriptorSets(
				VkDevice device,
				VkDescriptorPool& pool,
				VkDescriptorSet& descriptorSet,
				std::vector<VkDescriptorSetLayout> layouts)
			{
				VkDescriptorSetAllocateInfo alloc = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
				alloc.descriptorPool = pool;
				alloc.descriptorSetCount = layouts.size();
				alloc.pSetLayouts = layouts.data();

				assert(vkAllocateDescriptorSets(device, &alloc, &descriptorSet) == VK_SUCCESS);
			}


			VkWriteDescriptorSet createDescriptorBufferWrite(
				std::vector<VkDescriptorBufferInfo>& bufferInfos,
				VkDescriptorSet& descriptorSet,
				uint32_t binding)
			{
				return VkWriteDescriptorSet{
					VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					nullptr,
					descriptorSet,
					binding,
					0,
					static_cast<uint32_t>(bufferInfos.size()),
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					nullptr,
					bufferInfos.data(),
					nullptr
				};
			}


			VkWriteDescriptorSet createDescriptorImageWrite(
				std::vector<VkDescriptorImageInfo>& imageInfos,
				VkDescriptorSet& descriptorSet,
				uint32_t binding)
			{
				return VkWriteDescriptorSet{
					VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					nullptr,
					descriptorSet,
					binding,
					0,
					static_cast<uint32_t>(imageInfos.size()),
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					imageInfos.data(),
					nullptr,
					nullptr
				};
			}


			void writeDescriptorSets(
				VkDevice device,
				std::vector<VkWriteDescriptorSet>& descriptorWrites)
			{
				vkUpdateDescriptorSets(
					device,
					static_cast<uint32_t>(descriptorWrites.size()),
					descriptorWrites.data(),
					0,
					nullptr
				);
			}
		}
	}
}
