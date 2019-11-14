#include "SkyboxGenerator.h"

#include "stb_image.h"

#include "vulkan-util.h"
#include "ResourceManager.h"


namespace hvk
{
	SkyboxGenerator:: SkyboxGenerator(
		VulkanDevice device, 
		VmaAllocator allocator, 
		VkQueue graphicsQueue, 
		VkRenderPass renderPass, 
		VkCommandPool commandPool) :

		DrawlistGenerator(device, allocator, graphicsQueue, renderPass, commandPool),
		mSkyboxMap()
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

		ResourceManager::free(copyTo, copySize);

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
