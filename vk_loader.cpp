#include "stb_image.h"
#include <iostream>
#include "vk_loader.hpp"
#include <fstream>
#include "vk_engine.hpp"
#include "vk_initializers.hpp"
#include "vk_types.hpp"
#include <glm/gtx/quaternion.hpp>

#include "glm_element_traits.hpp"
#include "parser.hpp"
#include "tools.hpp"

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanEngine* engine, std::filesystem::path filePath)
{
  std::cout << "Loading GLTF: " << filePath << std::endl;

  fastgltf::GltfDataBuffer data;
  data.loadFromFile(filePath);

  constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers
  | fastgltf::Options::LoadExternalBuffers;

  fastgltf::Asset gltf;
  fastgltf::Parser parser {};

  auto load = parser.loadBinaryGLTF(&data, filePath.parent_path(), gltfOptions);
  if (load) {
    gltf = std::move(load.get());
  } else {
    std::cerr << "Failed to load glTF: {} \n" << fastgltf::to_underlying(load.error());
    return {};
  }
  std::vector<std::shared_ptr<MeshAsset>> meshes;

    // use the same vectors for all meshes so that the memory doesnt reallocate as
    // often
  std::vector<uint32_t> indices;
  std::vector<Vertex> vertices;
  for (fastgltf::Mesh& mesh : gltf.meshes) {
    MeshAsset newmesh;

    newmesh.name = mesh.name;

    indices.clear();
    vertices.clear();

    for (auto&& p : mesh.primitives) {
      GeoSurface newSurface;
      newSurface.startIndex = (uint32_t)indices.size();
      newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

      size_t initial_vtx = vertices.size();

      // load indexes
      {
        fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
        indices.reserve(indices.size() + indexaccessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor, 
          [&](std::uint32_t idx) {
          indices.push_back(idx + initial_vtx);
        });
      }

    // load vertex positions
      {
        fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
        vertices.resize(vertices.size() + posAccessor.count);

        fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
        [&](glm::vec3 v, size_t index) {
        Vertex newvtx;
        newvtx.position = v;
        newvtx.normal = { 1, 0, 0 };
        newvtx.color = glm::vec4 { 1.f };
        newvtx.uv.x = 0;
        newvtx.uv.y = 0;
        vertices[initial_vtx + index] = newvtx;
        });
      }

    // load vertex normals
        auto normals = p.findAttribute("NORMAL");
        if (normals != p.attributes.end()) 
        {
          fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
          [&](glm::vec3 v, size_t index) {
          vertices[initial_vtx + index].normal = v;
          });
        }

    // load UVs
        auto uv = p.findAttribute("TEXCOORD_0");
        if (uv != p.attributes.end()) 
        {
          fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
          [&](glm::vec2 v, size_t index) {
          vertices[initial_vtx + index].uv = v;
          });
        }

      // load vertex colors
        auto colors = p.findAttribute("COLOR_0");
        if (colors != p.attributes.end()) 
        {
          fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
            [&](glm::vec4 v, size_t index) {
            vertices[initial_vtx + index].color = v;
            });
          }
          newmesh.surfaces.push_back(newSurface);
        }

      // display the vertex normals
        constexpr bool OverrideColors = true;
        if (OverrideColors) 
        {
          for (Vertex& vtx : vertices) {
            vtx.color = glm::vec4(vtx.normal, 1.f);
          }
        }
        newmesh.meshBuffers = engine->uploadMeshBuffers(indices, vertices);

        meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newmesh)));
  }
  return meshes;
}

std::optional<AllocatedImage> load_image(VulkanEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image, std::string dirPath)
{
    AllocatedImage newImage {};

    int width, height, nrChannels;
    int idx = 0;
    std::visit(
        fastgltf::visitor {
            [](auto& arg) {},
            [&](fastgltf::sources::URI& filePath) {
                assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
                assert(filePath.uri.isLocalPath()); // We're only capable of loading
                                                    // local files.

                const std::string fileName(filePath.uri.path().begin(),
                    filePath.uri.path().end()); // Thanks C++.
                std::string path = dirPath + fileName;
                std::cerr << path << std::endl;
                unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
                if (data) {
                    VkExtent3D imagesize;
                    imagesize.width = width;
                    imagesize.height = height;
                    imagesize.depth = 1;

                    newImage = engine->create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT,false);
                    idx++;
                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::Vector& vector) {
                unsigned char* data = stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()),
                    &width, &height, &nrChannels, 4);
                if (data) {
                    VkExtent3D imagesize;
                    imagesize.width = width;
                    imagesize.height = height;
                    imagesize.depth = 1;

                    newImage = engine->create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT,false);
                    idx++;
                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::BufferView& view) {
                auto& bufferView = asset.bufferViews[view.bufferViewIndex];
                auto& buffer = asset.buffers[bufferView.bufferIndex];

                std::visit(fastgltf::visitor { // We only care about VectorWithMime here, because we
                                               // specify LoadExternalBuffers, meaning all buffers
                                               // are already loaded into a vector.
                               [](auto& arg) {},
                               [&](fastgltf::sources::Vector& vector) {
                                   unsigned char* data = stbi_load_from_memory(vector.bytes.data() + bufferView.byteOffset,
                                       static_cast<int>(bufferView.byteLength),
                                       &width, &height, &nrChannels, 4);
                                   if (data) {
                                       VkExtent3D imagesize;
                                       imagesize.width = width;
                                       imagesize.height = height;
                                       imagesize.depth = 1;

                                       newImage = engine->create_image(data, imagesize, VK_FORMAT_R8G8B8A8_UNORM,
                                           VK_IMAGE_USAGE_SAMPLED_BIT,false);
                                        idx++;
                                       stbi_image_free(data);
                                   }
                               } },
                    buffer.data);
            },
        },
        image.data);

    // if any of the attempts to load the data failed, we havent written the image
    // so handle is null
    std::cerr <<idx << std::endl;
    if (newImage._image == VK_NULL_HANDLE) {
        std::cerr << "image load faile execption" << std::endl;
        return {};
    } else {
        return newImage;
    }
}

VkFilter extract_filter(fastgltf::Filter filter)
{
    switch (filter) {
    // nearest samplers
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::NearestMipMapLinear:
        return VK_FILTER_NEAREST;

    // linear samplers
    case fastgltf::Filter::Linear:
    case fastgltf::Filter::LinearMipMapNearest:
    case fastgltf::Filter::LinearMipMapLinear:
    default:
        return VK_FILTER_LINEAR;
    }
}

VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter)
{
    switch (filter) {
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::LinearMipMapNearest:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;

    case fastgltf::Filter::NearestMipMapLinear:
    case fastgltf::Filter::LinearMipMapLinear:
    default:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VulkanEngine* engine,std::string dirPath, std::string fileName, MaterialPass passType, glm::mat4 transfrom)
{
    const std::string filePath = dirPath + fileName;
    std::cout << "Loading GLTF: "<<  filePath << std::endl;

    std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
    scene->creator = engine;
    LoadedGLTF& file = *scene.get();

    fastgltf::Parser parser {};

    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
    // fastgltf::Options::LoadExternalImages;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    fastgltf::Asset gltf;

    std::filesystem::path path = filePath;

    auto type = fastgltf::determineGltfFileType(&data);
    if (type == fastgltf::GltfType::glTF) {
        auto load = parser.loadGLTF(&data, path.parent_path(), gltfOptions);
        if (load) {
            gltf = std::move(load.get());
        } else {
            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    } else if (type == fastgltf::GltfType::GLB) {
        auto load = parser.loadBinaryGLTF(&data, path.parent_path(), gltfOptions);
        if (load) {
            gltf = std::move(load.get());
        } else {
            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
            return {};
        }
    } else {
        std::cerr << "Failed to determine glTF container" << std::endl;
        return {};
    }

    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = { 
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,4 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,4 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4 } };



    for (fastgltf::Sampler& sampler : gltf.samplers) {

        VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
        sampl.maxLod = VK_LOD_CLAMP_NONE;
        sampl.minLod = 0;

        sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
        sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        sampl.mipmapMode= extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        VkSampler newSampler;
        vkCreateSampler(engine->_device, &sampl, nullptr, &newSampler);
        file.samplers.push_back(newSampler);
    }
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<AllocatedImage> images;
    std::vector<std::shared_ptr<GLTFMaterial>> materials;

    for (fastgltf::Image& image : gltf.images) {
        std::optional<AllocatedImage> img = load_image(engine, gltf, image, dirPath);
        if (img.has_value()) {
            images.push_back(*img);
            file.images.push_back(*img);

        } else {
            // we failed to load, so lets give the slot a default white texture to not
            // completely break loading
            images.push_back(engine->_errorCheckerboardImage);
            std::cout << "gltf failed to load texture " << image.name << std::endl;
        }
    }
//> load_buffer
    // create buffer to hold the material data
    if (gltf.materials.size()){
        file.descriptorPool.init(engine->_device, gltf.materials.size(), sizes);
        file.materialDataBuffer = engine->create_buffer(sizeof(GLTFMetallic_Roughness::MaterialConstants) * gltf.materials.size(),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        int data_index = 0;
        GLTFMetallic_Roughness::MaterialConstants* sceneMaterialConstants = (GLTFMetallic_Roughness::MaterialConstants*)file.materialDataBuffer.info.pMappedData;
        for (fastgltf::Material& mat : gltf.materials) {
            std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
            materials.push_back(newMat);
            file.materials[mat.name.c_str()] = newMat;

            GLTFMetallic_Roughness::MaterialConstants constants;
            constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
            constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
            constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
            constants.colorFactors.w = mat.pbrData.baseColorFactor[3];
            constants.metal_rough_factors.x = mat.pbrData.metallicFactor;
            constants.metal_rough_factors.y = mat.pbrData.roughnessFactor;
            // write material parameters to buffer
            sceneMaterialConstants[data_index] = constants;

            // MaterialPass passType = MaterialPass::MainColor;
            // if (mat.alphaMode == fastgltf::AlphaMode::Blend) {
            //     passType = MaterialPass::Transparent;
            // }
            GLTFMetallic_Roughness::MaterialResources materialResources;
            // default the material textures
            // materialResources.colorImage = engine->_errorCheckerboardImage;
            // materialResources.colorSampler = engine->_defaultSamplerLinear;
            // materialResources.metalRoughImage = engine->_errorCheckerboardImage;
            // materialResources.metalRoughSampler = engine->_defaultSamplerLinear;
            materialResources.colorImage = engine->_errorCheckerboardImage;
            materialResources.colorSampler = engine->_defaultSamplerLinear;
            // if (passType != MaterialPass::SkyBoxVulkan){
            //     materialResources.colorImage = engine->_spaceBoxImage;
            //     materialResources.colorSampler = engine->_spaceBoxSamplerLinear;
            // }
            materialResources.metalRoughImage = engine->_errorCheckerboardImage;
            materialResources.metalRoughSampler = engine->_defaultSamplerLinear;
            materialResources.dataBuffer = file.materialDataBuffer.buffer;
            materialResources.dataBufferOffset = data_index * sizeof(GLTFMetallic_Roughness::MaterialConstants);
            // grab textures from gltf file

            if (mat.pbrData.baseColorTexture.has_value()) {
                auto& baseColorTexture = mat.pbrData.baseColorTexture.value();
                if (baseColorTexture.textureIndex < gltf.textures.size()) {
                    auto& texture = gltf.textures[baseColorTexture.textureIndex];
                    if (texture.imageIndex.has_value() && texture.imageIndex.value() < images.size()) {
                        materialResources.colorImage = images[texture.imageIndex.value()];
                    }
                    if (texture.samplerIndex.has_value() && texture.samplerIndex.value() < file.samplers.size()) {
                        materialResources.colorSampler = file.samplers[texture.samplerIndex.value()];
                    }
                }
            }
            std::cerr <<"load_imwrite_materialage" << std::endl;
            newMat->data = engine->metalRoughMaterial.write_material(engine, passType, materialResources, file.descriptorPool);

            data_index++;
        }
    } else {
        file.descriptorPool.init(engine->_device, 1, sizes);
        file.materialDataBuffer = engine->create_buffer(sizeof(GLTFMetallic_Roughness::MaterialConstants) * 1,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        GLTFMetallic_Roughness::MaterialConstants* sceneMaterialConstants = (GLTFMetallic_Roughness::MaterialConstants*)file.materialDataBuffer.info.pMappedData;

        std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
        materials.push_back(newMat);
        file.materials["emptyMat"] = newMat;

        GLTFMetallic_Roughness::MaterialConstants constants;
        constants.colorFactors = glm::vec4(1.0f);
        constants.metal_rough_factors = glm::vec4(0.0f);
        sceneMaterialConstants[0] = constants;
        std::cerr << "empty" << std::endl;
        GLTFMetallic_Roughness::MaterialResources materialResources;
        // default the material textures
        materialResources.colorImage = engine->_errorCheckerboardImage;
        materialResources.colorSampler = engine->_defaultSamplerLinear;
        // if (passType != MaterialPass::SkyBoxVulkan){
        //         materialResources.colorImage = engine->_spaceBoxImage;
        //         materialResources.colorSampler = engine->_spaceBoxSamplerLinear;
        // }
        materialResources.metalRoughImage = engine->_errorCheckerboardImage;
        materialResources.metalRoughSampler = engine->_defaultSamplerLinear;

        // set the uniform buffer for the material data
        materialResources.dataBuffer = file.materialDataBuffer.buffer;
        materialResources.dataBufferOffset = 0;
        newMat->data = engine->metalRoughMaterial.write_material(engine, passType, materialResources, file.descriptorPool);
    }
    vmaFlushAllocation(engine->_allocator, file.materialDataBuffer.allocation, 0, VK_WHOLE_SIZE);
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (fastgltf::Mesh& mesh : gltf.meshes) {
        std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
        meshes.push_back(newmesh);
        file.meshes[mesh.name.c_str()] = newmesh;
        newmesh->name = mesh.name;

        // clear the mesh arrays each mesh, we dont want to merge them by error
        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives) {
            GeoSurface newSurface;
            newSurface.startIndex = (uint32_t)indices.size();
            newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            size_t initial_vtx = vertices.size();

            // load indexes
            {
                fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
                    [&](std::uint32_t idx) {
                        indices.push_back(idx + initial_vtx);
                    });
            }

            // load vertex positions
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                    [&](glm::vec3 v, size_t index) {
                        Vertex newvtx;
                        newvtx.position = transfrom * glm::vec4(v,1.f);
                        newvtx.normal = { 1, 0, 0 };
                        newvtx.color = glm::vec4 { 1.f };
                        newvtx.uv = {0,0};
                        vertices[initial_vtx + index] = newvtx;
                    });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                    [&](glm::vec3 v, size_t index) {
                        vertices[initial_vtx + index].normal = v;
                    });
            }

            // load UVs
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
                    [&](glm::vec2 v, size_t index) {
                        vertices[initial_vtx + index].uv = v;
                    });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                    [&](glm::vec4 v, size_t index) {
                        vertices[initial_vtx + index].color = v;
                    });
            }

            if (p.materialIndex.has_value()) {
                newSurface.material = materials[p.materialIndex.value()];
            } else {
                newSurface.material = materials[0];
            }

            glm::vec3 minpos = vertices[initial_vtx].position;
            glm::vec3 maxpos = vertices[initial_vtx].position;
            for (int i = initial_vtx; i < vertices.size(); i++) {
                minpos = glm::min(minpos, vertices[i].position);
                maxpos = glm::max(maxpos, vertices[i].position);
            }

            newSurface.bounds.origin = (maxpos + minpos) / 2.f;
            newSurface.bounds.extents = (maxpos - minpos) / 2.f;
            newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);
            newmesh->surfaces.push_back(newSurface);
        }
        newmesh->meshBuffers = engine->uploadMeshBuffers(indices, vertices);
    }
    for (fastgltf::Node& node : gltf.nodes) {
        std::shared_ptr<Node> newNode;

        // find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
        if (node.meshIndex.has_value()) {
            newNode = std::make_shared<MeshNode>();
            static_cast<MeshNode*>(newNode.get())->mesh = meshes[*node.meshIndex];
        } else {
            newNode = std::make_shared<Node>();
        }

        nodes.push_back(newNode);
        file.nodes[node.name.c_str()];

        std::visit(fastgltf::visitor { [&](fastgltf::Node::TransformMatrix matrix) {
                                          memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
                                      },
                       [&](fastgltf::Node::TRS transform) {
                           glm::vec3 tl(transform.translation[0], transform.translation[1],
                               transform.translation[2]);
                           glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1],
                               transform.rotation[2]);
                           glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                           glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
                           glm::mat4 rm = glm::toMat4(rot);
                           glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

                           newNode->localTransform = tm * rm * sm;
                       } },
            node.transform);
    }
//< load_nodes
//> load_graph
    // run loop again to setup transform hierarchy
    for (int i = 0; i < gltf.nodes.size(); i++) {
        fastgltf::Node& node = gltf.nodes[i];
        std::shared_ptr<Node>& sceneNode = nodes[i];

        for (auto& c : node.children) {
            sceneNode->children.push_back(nodes[c]);
            nodes[c]->parent = sceneNode;
        }
    }

    // find the top nodes, with no parents
    for (auto& node : nodes) {
        if (node->parent.lock() == nullptr) {
            file.topNodes.push_back(node);
            node->refreshTransform(glm::mat4 { 1.f });
        }
    }
    return scene;
}

void LoadedGLTF::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
    // create renderables from the scenenodes
    for (auto& n : topNodes) {
        n->Draw(topMatrix, ctx);
    }
}

void loadCubeMap(VulkanEngine* engine, std::string dirPath, std::string fileName, VkFormat format, VkSampler* sampler, AllocatedImage* newImage)
{
    ktxResult result;
    ktxTexture* ktxTexture;

    const std::string filePath = dirPath + fileName;
    std::ifstream f(filePath.c_str());
    if (f.fail()){
        std::cerr << "load cube file path fail : " << filePath << std::endl;
    }
    result = ktxTexture_CreateFromNamedFile(filePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
    assert(result == KTX_SUCCESS);

    *newImage = engine->createCubeImage(ktxTexture, format, sampler);
    engine->_mainDeletionQueue.push_function([=]() {
        engine->destroy_image(*newImage);
    });
    ktxTexture_Destroy(ktxTexture);
}   

void LoadedGLTF::clearAll()
{
    VkDevice dv = creator->_device;
    std::cerr << "claerall call" << std::endl;
    for (auto& [k, v] : meshes) {

        creator->destroy_buffer(v->meshBuffers.indexBuffer);
        creator->destroy_buffer(v->meshBuffers.vertexBuffer);
    }

    for (auto& v : images) {

        if (v._image == creator->_errorCheckerboardImage._image) {
            // dont destroy the default images
            continue;
        }
        creator->destroy_image(v);
    }

    for (auto& sampler : samplers) {
        vkDestroySampler(dv, sampler, nullptr);
    }

    // auto samplersToDestroy = samplers;
    creator->destroy_buffer(materialDataBuffer);

    descriptorPool.destroy_pools(dv);
}