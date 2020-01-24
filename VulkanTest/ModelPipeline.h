#pragma once

#include <unordered_map>

#include "HvkUtil.h"
#include "StaticMesh.h"
#include "DebugMesh.h"
#include "types.h"

namespace hvk
{
    struct PBRMesh;
    struct PBRMaterial;
    struct DebugDrawMesh;

    class ModelPipeline
    {
    private:
        std::unordered_map<std::string, PBRMesh> mMeshStore;
        std::unordered_map<std::string, PBRMaterial> mMaterialStore;
        std::unordered_map<std::string, DebugDrawMesh> mDebugMeshStore;
        TextureMap mDummyAlbedoMap;
        TextureMap mDummyNormalMap;
        TextureMap mDummyMetallicRoughnessMap;
        bool mInitialized;

        void processGltfModel(const StaticMesh& model, const std::string& modelName);
        void processDebugModel(const DebugMesh& model, const std::string& modelName);

    public:
        ModelPipeline();
        ~ModelPipeline();
        void init();

        PBRMesh fetchMesh(std::string&& name);
        PBRMaterial fetchMaterial(std::string&& name);
        DebugDrawMesh fetchDebugMesh(const std::string& name);
        bool loadAndFetchModel(
            const StaticMesh& model, 
            const std::string& name, 
            PBRMesh& outMesh, 
            PBRMaterial& outMaterial);
        bool loadAndFetchDebugModel(
            const DebugMesh& model,
            const std::string& name,
            DebugDrawMesh& outMesh);
    };
}
