#pragma once

#include <vector>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

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
				VkDescriptorPool& pool);

			void createDescriptorSetLayout(
				VkDevice device,
				std::vector<VkDescriptorSetLayoutBinding>& bindings,
				VkDescriptorSetLayout& descriptorSetLayout);

			void allocateDescriptorSets(
				VkDevice device,
				VkDescriptorPool& pool,
				VkDescriptorSet& descriptorSet,
				std::vector<VkDescriptorSetLayout> layouts);

			VkDescriptorSetLayoutBinding generateUboLayoutBinding(
				uint32_t binding,
				uint32_t descriptorCount,
				VkShaderStageFlags flags=VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

			VkDescriptorSetLayoutBinding generateSamplerLayoutBinding(
				uint32_t binding,
				uint32_t descriptorCount,
				VkShaderStageFlags flags=VK_SHADER_STAGE_FRAGMENT_BIT);

			VkWriteDescriptorSet createDescriptorBufferWrite(
				std::vector<VkDescriptorBufferInfo>& bufferInfos,
				VkDescriptorSet& descriptorSet,
				uint32_t binding);

			VkWriteDescriptorSet createDescriptorImageWrite(
				std::vector<VkDescriptorImageInfo>& imageInfos,
				VkDescriptorSet& descriptorSet,
				uint32_t binding);

			void writeDescriptorSets(
				VkDevice device,
				std::vector<VkWriteDescriptorSet>& descriptorWrites);


			template <VkDescriptorType T>
			void _buildPoolSizes(std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t count)
			{
				poolSizes.push_back({
					T,
					count});
			}


			template <VkDescriptorType T, VkDescriptorType... Args>
			void _buildPoolSizes(std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t count, uint32_t counts...)
			{
				poolSizes.push_back({
					T,
					count});
				_buildPoolSizes<Args...>(poolSizes, counts);
			}


			template <VkDescriptorType... Args>
			std::vector<VkDescriptorPoolSize> createPoolSizes(const uint32_t count, const uint32_t counts...)
			{
				std::vector<VkDescriptorPoolSize> poolSizes;
				poolSizes.reserve(sizeof...(Args));
				_buildPoolSizes<Args...>(poolSizes, count, counts);
				return poolSizes;
			}


			template <VkDescriptorType T>
			std::vector<VkDescriptorPoolSize> createPoolSizes(const uint32_t count)
			{
				std::vector<VkDescriptorPoolSize> poolSizes;
				poolSizes.reserve(1);
				_buildPoolSizes<T>(poolSizes, count);
				return poolSizes;
			}
		}
	}
}
