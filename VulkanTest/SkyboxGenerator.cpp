#include "SkyboxGenerator.h"

#include "stb_image.h"

#include "vulkan-util.h"
#include "ResourceManager.h"
#include "shapes.h"


namespace hvk
{
	SkyboxGenerator:: SkyboxGenerator(
		VulkanDevice device, 
		VmaAllocator allocator, 
		VkQueue graphicsQueue, 
		VkRenderPass renderPass, 
		VkCommandPool commandPool) :

		DrawlistGenerator(device, allocator, graphicsQueue, renderPass, commandPool),
		mSkyboxMap(),
		mSkyboxRenderable()
	{
        // load skybox textures
        /*
        
                T
            |L |F |R |BA
                BO
        
        */
        std::array<std::string, 6> skyboxFiles = {
            "resources/sky/desertsky_lf.tga",
            "resources/sky/desertsky_ft.tga",
            "resources/sky/desertsky_up.tga",
            "resources/sky/desertsky_dn.tga",
            "resources/sky/desertsky_rt.tga",
            "resources/sky/desertsky_bk.tga"
        };

        int width, height, numChannels;
		int copyOffset = 0;
		int copySize = 0;
		int layerSize = 0;
		unsigned char* copyTo = nullptr;
		unsigned char* layers[6];
        for (size_t i = 0; i < skyboxFiles.size(); ++i)
        {
            unsigned char* data = stbi_load(
                skyboxFiles[i].c_str(), 
                &width, 
                &height, 
                &numChannels, 
                0);

			assert(data != nullptr);

			layers[i] = data;
			copySize += width * height * numChannels;
        }
		layerSize = copySize / skyboxFiles.size();

		copyTo = static_cast<unsigned char*>(ResourceManager::alloc(copySize, alignof(unsigned char)));
		for (size_t i = 0; i < 6; ++i)
		{
			void* dst = copyTo + copyOffset;
			copyOffset += layerSize;
			memcpy(dst, layers[i], layerSize);
			stbi_image_free(layers[i]);
		}

		mSkyboxMap.texture = createTextureImage(
			mDevice.device, 
			mAllocator, 
			mCommandPool, 
			mGraphicsQueue, 
			copyTo, 
			6, 
			width, 
			height, 
			numChannels);
		mSkyboxMap.view = createImageView(
			mDevice.device,
			mSkyboxMap.texture.memoryResource,
			VK_FORMAT_R8G8B8A8_UNORM);
		mSkyboxMap.sampler = createTextureSampler(mDevice.device);

		ResourceManager::free(copyTo, copySize);

		auto skyboxMesh = createColoredCube(glm::vec3(0.1f, 4.f, 1.f));
		auto vertices = skyboxMesh->getVertices();
		auto indices = skyboxMesh->getIndices();
		mSkyboxRenderable.numVertices = static_cast<uint32_t>(vertices->size());
		mSkyboxRenderable.numIndices = static_cast<uint32_t>(indices->size());

		// Create VBO
		size_t vertexMemorySize = sizeof(ColorVertex) * mSkyboxRenderable.numVertices;
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = vertexMemorySize;
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaCreateBuffer(
			mAllocator,
			&bufferInfo,
			&allocCreateInfo,
			&mSkyboxRenderable.vbo.memoryResource,
			&mSkyboxRenderable.vbo.allocation,
			&mSkyboxRenderable.vbo.allocationInfo);

		memcpy(mSkyboxRenderable.vbo.allocationInfo.pMappedData, vertices->data(), vertexMemorySize);

		// Create IBO
        size_t indexMemorySize = sizeof(uint16_t) * mSkyboxRenderable.numIndices;
        VkBufferCreateInfo iboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        iboInfo.size = indexMemorySize;
        iboInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo indexAllocCreateInfo = {};
        indexAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        indexAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        vmaCreateBuffer(
            mAllocator,
            &iboInfo,
            &indexAllocCreateInfo,
            &mSkyboxRenderable.ibo.memoryResource,
            &mSkyboxRenderable.ibo.allocation,
            &mSkyboxRenderable.ibo.allocationInfo);

        memcpy(mSkyboxRenderable.ibo.allocationInfo.pMappedData, indices->data(), indexMemorySize);

		// Create UBO
        uint32_t uboMemorySize = sizeof(hvk::UniformBufferObject);
        VkBufferCreateInfo uboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        uboInfo.size = uboMemorySize;
        uboInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		VmaAllocationCreateInfo uniformAllocCreateInfo = {};
		uniformAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		uniformAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		vmaCreateBuffer(
			mAllocator,
			&uboInfo,
			&uniformAllocCreateInfo,
			&mSkyboxRenderable.ubo.memoryResource,
			&mSkyboxRenderable.ubo.allocation,
			&mSkyboxRenderable.ubo.allocationInfo);

		// Create descriptor pool

		// Create descriptor set

		// Update descriptor set

		setInitialized(true);
	}


	SkyboxGenerator::~SkyboxGenerator()
	{
		destroyMap(mDevice.device, mAllocator, mSkyboxMap);
	}

	void SkyboxGenerator::invalidate()
	{
		setInitialized(false);
	}
}
