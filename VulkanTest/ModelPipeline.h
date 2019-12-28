#pragma once

#include <unordered_map>

#include "HvkUtil.h"
#include "StaticMesh.h"
#include "PBRTypes.h"

namespace hvk
{
    class ModelPipeline
    {
    private:
        std::unordered_map<std::string, PBRMesh> mMeshStore;
        std::unordered_map<std::string, PBRMaterial> mMaterialStore;
        TextureMap mDummyMap;
        bool mInitialized;

        void processGltfModel(HVK_shared<StaticMesh> model, const std::string& modelName);

    public:
        ModelPipeline();
        ~ModelPipeline();
        void init();

        PBRMesh fetchMesh(std::string&& name);
        PBRMaterial fetchMaterial(std::string&& name);
        bool loadAndFetchModel(HVK_shared<StaticMesh> model, std::string&& name, PBRMesh* outMesh, PBRMaterial* outMaterial);

    };
}
