#pragma once

#include "vk_types.hpp"
#include <unordered_map>
#include <filesystem>
#include "vk_mesh.hpp"
#include "vk_descriptors.hpp"

struct Bounds {
    glm::vec3 origin;
    float sphereRadius;
    glm::vec3 extents;
};

struct GLTFMaterial {
	MaterialInstance data;
    explicit GLTFMaterial(){}
    explicit GLTFMaterial(const MaterialInstance& instance) : data(instance) {}
};

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
    Bounds bounds;
    std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset {
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
};

struct LoadedGLTF : public IRenderable {

    // storage for all the data on a given glTF file
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::vector<AllocatedImage> images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

    // nodes that dont have a parent, for iterating through the file in tree order
    std::vector<std::shared_ptr<Node>> topNodes;

    std::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptorPool;

    AllocatedBuffer materialDataBuffer;

    VulkanEngine* creator;

    ~LoadedGLTF() { clearAll(); };

    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx);

private:

    void clearAll();
};

//forward declaration
class VulkanEngine;
std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanEngine* engine, std::filesystem::path filePath);
std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine* engine,std::string dirPath, std::string fileName, MaterialPass passType);
void loadCubeMap(VulkanEngine* engine, std::string dirPath, std::string fileName, VkFormat format, VkSampler* sampler, AllocatedImage* newImage);