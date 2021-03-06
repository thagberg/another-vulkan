#include "pch.h"
#include "ModelPipeline.h"
#include "GpuManager.h"
#include "image-util.h"
#include "PBRTypes.h"
#include "DebugDrawTypes.h"


namespace hvk
{
    TextureMap createTextureMapFromImageData(
        VkDevice device,
        VmaAllocator allocator,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        const tinygltf::Image& texture)
    {
        return util::image::createTextureMap(
            device,
            allocator,
            commandPool,
            graphicsQueue,
            texture.image.data(),
            texture.width,
            texture.height,
            texture.component * (texture.bits / 8));
    }

    ModelPipeline::ModelPipeline() :
        mMeshStore(),
        mMaterialStore(),
        mDummyAlbedoMap(),
        mDummyNormalMap(),
        mDummyMetallicRoughnessMap(),
        mInitialized(false)
    {
    }


    ModelPipeline::~ModelPipeline()
    {
    }

    void ModelPipeline::init()
    {
        mMeshStore.reserve(50);
        mMaterialStore.reserve(50);
        mDebugMeshStore.reserve(50);
        mDummyAlbedoMap = util::image::createTextureMapFromFile(
            GpuManager::getDevice(),
            GpuManager::getAllocator(),
            GpuManager::getCommandPool(),
            GpuManager::getGraphicsQueue(),
            std::string("resources/dummy-white.png"));
        mDummyNormalMap = util::image::createTextureMapFromFile(
            GpuManager::getDevice(),
            GpuManager::getAllocator(),
            GpuManager::getCommandPool(),
            GpuManager::getGraphicsQueue(),
            std::string("resources/dummy-normal.png"));
        mDummyMetallicRoughnessMap = util::image::createTextureMapFromFile(
            GpuManager::getDevice(),
            GpuManager::getAllocator(),
            GpuManager::getCommandPool(),
            GpuManager::getGraphicsQueue(),
            std::string("resources/dummy-occlusionmetallicroughness.png"));
        mInitialized = true;
    }

    void ModelPipeline::processGltfModel(const StaticMesh& model, const std::string& name)
    {
        assert(mInitialized);

        PBRMesh mesh;
        PBRMaterial material;

        const auto& allocator = GpuManager::getAllocator();
        const auto& device = GpuManager::getDevice();
        const auto& commandPool = GpuManager::getCommandPool();
        const auto& graphicsQueue = GpuManager::getGraphicsQueue();

		const StaticMesh::Vertices vertices = model.getVertices();
		const StaticMesh::Indices indices = model.getIndices();

		mesh.numIndices = indices.size();

        // Create vertex buffer
        size_t vertexMemorySize = sizeof(Vertex) * vertices.size();
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = vertexMemorySize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo vboAllocInfo;
        vmaCreateBuffer(
            GpuManager::getAllocator(),
            &bufferInfo,
            &allocCreateInfo,
            &mesh.vbo.memoryResource,
            &mesh.vbo.allocation,
            &vboAllocInfo);

        memcpy(vboAllocInfo.pMappedData, vertices.data(), vertexMemorySize);

        // Create index buffer
        size_t indexMemorySize = sizeof(uint16_t) * indices.size();
        VkBufferCreateInfo iboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        iboInfo.size = indexMemorySize;
        iboInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo indexAllocCreateInfo = {};
        indexAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        indexAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo iboAllocInfo;
        vmaCreateBuffer(
            allocator,
            &iboInfo,
            &indexAllocCreateInfo,
            &mesh.ibo.memoryResource,
            &mesh.ibo.allocation,
            &iboAllocInfo);

        memcpy(iboAllocInfo.pMappedData, indices.data(), indexMemorySize);

        // Create texture maps
        const auto& mat = model.getMaterial();

        if (mat.diffuseProp.texture != nullptr)
        {
			const auto& diffuseTex = *mat.diffuseProp.texture;
			material.albedo = createTextureMapFromImageData(
				device,
				allocator,
				commandPool,
				graphicsQueue,
				diffuseTex);
        }
        else
        {
            material.albedo = mDummyAlbedoMap;
        }

        if (mat.metallicRoughnessProp.texture != nullptr)
        {
            const auto& mtlRoughTex = *mat.metallicRoughnessProp.texture;
            material.metallicRoughness = createTextureMapFromImageData(
                device,
                allocator,
                commandPool,
                graphicsQueue,
                mtlRoughTex);
        }
        else
        {
            material.metallicRoughness = mDummyMetallicRoughnessMap;
        }

        if (mat.normalProp.texture != nullptr)
        {
            const auto& normalTex = *mat.normalProp.texture;
            material.normal = createTextureMapFromImageData(
                device,
                allocator,
                commandPool,
                graphicsQueue,
                normalTex);
        }
        else
        {
            material.normal = mDummyNormalMap;
        }

        // register mesh and material in the store
        mMeshStore.insert({ name + "_lod0", mesh });
        mMaterialStore.insert({ name + "_material_lod0", material });
    }

    void ModelPipeline::processDebugModel(const DebugMesh& model, const std::string& modelName)
    {
        assert(mInitialized);

        DebugDrawMesh mesh;

        const auto& allocator = GpuManager::getAllocator();
        const auto& device = GpuManager::getDevice();

        const auto vertices = model.getVertices();
        const auto indices = model.getIndices();

        mesh.numIndices = indices->size();

        // create vertex buffer
        size_t vertexMemorySize = sizeof(ColorVertex) * vertices->size();
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = vertexMemorySize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo vboAllocInfo;
        vmaCreateBuffer(
            allocator,
            &bufferInfo,
            &allocCreateInfo,
            &mesh.vbo.memoryResource,
            &mesh.vbo.allocation,
            &vboAllocInfo);

        memcpy(vboAllocInfo.pMappedData, vertices->data(), vertexMemorySize);

        // create index buffer
        uint32_t indexMemorySize = sizeof(uint16_t) * mesh.numIndices;
        VkBufferCreateInfo iboInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        iboInfo.size = indexMemorySize;
        iboInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo indexAllocCreateInfo = {};
        indexAllocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        indexAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo iboAllocInfo;
        vmaCreateBuffer(
            allocator,
            &iboInfo,
            &indexAllocCreateInfo,
            &mesh.ibo.memoryResource,
            &mesh.ibo.allocation,
            &iboAllocInfo);

        memcpy(iboAllocInfo.pMappedData, indices->data(), (size_t)indexMemorySize);

        // register mesh in the store
        mDebugMeshStore.insert({ modelName, mesh });
    }

    PBRMesh ModelPipeline::fetchMesh(std::string&& name)
    {
        auto found = mMeshStore.find(name);
        assert(found != mMeshStore.end());
        return found->second;
    }

    PBRMaterial ModelPipeline::fetchMaterial(std::string&& name)
    {
        auto found = mMaterialStore.find(name);
        assert(found != mMaterialStore.end());
        return found->second;
    }

    DebugDrawMesh ModelPipeline::fetchDebugMesh(const std::string& name)
    {
        auto found = mDebugMeshStore.find(name);
        assert(found != mDebugMeshStore.end());
        return found->second;
    }

    bool ModelPipeline::loadAndFetchModel(
        const StaticMesh& model, 
        const std::string& name, 
        PBRMesh& outMesh, 
        PBRMaterial& outMaterial)
    {
        bool newLoad = false;
        auto meshName = name + "_lod0";
        auto materialName = name + "_material_lod0";

        auto meshAt = mMeshStore.find(meshName);
        auto materialAt = mMaterialStore.find(materialName);
        bool meshFound = meshAt != mMeshStore.end();
        bool materialFound = materialAt != mMaterialStore.end();

        // If only one is found and not the other, something bad happened
        assert(meshFound == materialFound);

        if (meshFound)
        {
            outMesh = meshAt->second;
            outMaterial = materialAt->second;
        }
        else
        {
            newLoad = true;
            processGltfModel(model, name);
            outMesh = mMeshStore.at(meshName);
            outMaterial = mMaterialStore.at(materialName);
        }

        return newLoad;
    }

    bool ModelPipeline::loadAndFetchDebugModel(
        const DebugMesh& model,
        const std::string& name,
        DebugDrawMesh& outMesh)
    {
        bool newLoad = false;

        auto meshAt = mDebugMeshStore.find(name);
        bool meshFound = meshAt != mDebugMeshStore.end();

        if (meshFound)
        {
            outMesh = meshAt->second;
        }
        else
        {
            newLoad = true;
            processDebugModel(model, name);
            outMesh = mDebugMeshStore.at(name);
        }

        return newLoad;
    }
}
