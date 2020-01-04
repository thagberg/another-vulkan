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
        TextureMap mDummyMap;
        bool mInitialized;

        void processGltfModel(std::shared_ptr<StaticMesh> model, const std::string& modelName);
        void processDebugModel(std::shared_ptr<DebugMesh> model, const std::string& modelName);

    public:
        ModelPipeline();
        ~ModelPipeline();
        void init();

        PBRMesh fetchMesh(std::string&& name);
        PBRMaterial fetchMaterial(std::string&& name);
        DebugDrawMesh fetchDebugMesh(const std::string& name);
        bool loadAndFetchModel(
            std::shared_ptr<StaticMesh> model, 
            std::string&& name, 
            PBRMesh* outMesh, 
            PBRMaterial* outMaterial);
        bool loadAndFetchDebugModel(
            std::shared_ptr<DebugMesh> model,
            std::string&& name,
            DebugDrawMesh* outMesh);
    };
}
