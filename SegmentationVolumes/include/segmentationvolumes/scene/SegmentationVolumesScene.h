#pragma once

#include "../Raystructs.h"
#include "../converter/builder/DAG.h"
#include "../data/MaterialGenerator.h"
#include "../data/SegmentationVolumeMaterial.h"
#include "Volume.h"
#include "imgui.h"
#include "raven/core/AccelerationStructure.h"
#include "raven/core/Texture.h"

#include <map>
#include <memory>
#include <vector>

namespace raven {
    class SegmentationVolumesScene {
    public:
        struct SceneSettings {
            std::optional<uint32_t> m_maxFrames;
            std::optional<uint32_t> m_tonemapper;
            std::optional<uint32_t> m_renderer;
            std::optional<uint32_t> m_debug;
            std::optional<uint32_t> m_spp;
            std::optional<uint32_t> m_lodType;
            std::optional<bool> m_lod;
            std::optional<bool> m_lodDisableOnFirstFrame;
            std::optional<bool> m_lodDistance;
            std::optional<uint32_t> m_lodDistanceVoxel;  // finest LOD, traverse octrees and occupancy fields up to 1^3, i.e. individual voxels
            std::optional<uint32_t> m_lodDistanceOctree; // middle LOD, traverse octrees up to 4^3
            std::optional<uint32_t> m_bounces;
            std::optional<bool> m_labelMetadata;
            std::optional<bool> m_labelMetadataJitterAlbedo;
        };

        struct SceneOptions {
            bool m_environmentMap = true;
            glm::vec2 m_environmentMapRotation = glm::vec2(0.f);
            glm::vec3 m_skyColor = glm::vec3(0);
        };
        SceneOptions m_sceneOptions{};

        std::map<std::string, uint32_t> m_materialKeys;
        std::map<std::string, uint32_t> m_materialNames;
        std::vector<std::shared_ptr<SegmentationVolumeMaterial>> m_materials;

        std::shared_ptr<Texture> m_textureEnvironment;

        std::shared_ptr<Buffer> m_objectDescriptorBuffer;
        std::shared_ptr<Buffer> m_materialBuffer;

        std::shared_ptr<TLAS> m_tlas;

        std::vector<std::shared_ptr<Volume>> m_volumes;

        uint32_t m_instanceCount = 0;

        void load(GPUContext *gpuContext, const std::string &dataPath, const std::string &sceneName, SceneSettings *sceneSettings) {
            std::string scenePath = Paths::m_resourceDirectoryPath + "/scenes/" + sceneName + ".xml";

            // // WORM
            // {
            //     pugi::xml_document doc;
            //
            //     pugi::xml_node volume = doc.append_child("volume");
            //
            //     pugi::xml_node info = volume.append_child("info");
            //     info.append_attribute("type") = "celegans";
            //     info.append_attribute("folder") = "celegans/svo";
            //
            //     pugi::xml_node aabbs = volume.append_child("aabbs");
            //     for (uint32_t i = 1; i <= 240; i++) {
            //         pugi::xml_node aabb = aabbs.append_child("aabb");
            //         std::string name = "neuron" + std::to_string(i);
            //         std::string lod = std::to_string(i);
            //         aabb.append_attribute("name") = name.c_str();
            //         aabb.append_attribute("type") = "default";
            //         aabb.append_attribute("lod") = lod.c_str();
            //         aabb.append_attribute("enabled") = true;
            //     }
            //
            //     pugi::xml_node lods = volume.append_child("lods");
            //     for (uint32_t i = 1; i <= 240; i++) {
            //         pugi::xml_node lod = lods.append_child("lod");
            //         std::string name = "neuron" + std::to_string(i);
            //         std::string key = std::to_string(i);
            //         lod.append_attribute("key") = key.c_str();
            //         lod.append_attribute("name") = name.c_str();
            //         lod.append_attribute("type") = "svo";
            //     }
            //
            //     std::string path = Paths::m_resourceDirectoryPath + "/scenes/volumes/celegans/svo.xml";
            //     if (!doc.save_file(path.c_str())) {
            //         throw std::runtime_error("Cannot save scene file.");
            //     }
            // }
            // {
            //     pugi::xml_document doc;
            //
            //     pugi::xml_node volume = doc.append_child("volume");
            //
            //     pugi::xml_node info = volume.append_child("info");
            //     info.append_attribute("type") = "celegans";
            //     info.append_attribute("folder") = "celegans/svdag";
            //
            //     pugi::xml_node aabbs = volume.append_child("aabbs");
            //     for (uint32_t i = 1; i <= 240; i++) {
            //         pugi::xml_node aabb = aabbs.append_child("aabb");
            //         std::string name = "neuron" + std::to_string(i);
            //         std::string lod = std::to_string(i);
            //         aabb.append_attribute("name") = name.c_str();
            //         aabb.append_attribute("type") = "default";
            //         aabb.append_attribute("lod") = lod.c_str();
            //         aabb.append_attribute("enabled") = true;
            //     }
            //
            //     pugi::xml_node lods = volume.append_child("lods");
            //     for (uint32_t i = 1; i <= 240; i++) {
            //         pugi::xml_node lod = lods.append_child("lod");
            //         std::string name = "neuron" + std::to_string(i);
            //         std::string key = std::to_string(i);
            //         lod.append_attribute("key") = key.c_str();
            //         lod.append_attribute("name") = name.c_str();
            //         lod.append_attribute("type") = "svdag";
            //     }
            //
            //     std::string path = Paths::m_resourceDirectoryPath + "/scenes/volumes/celegans/svdag.xml";
            //     if (!doc.save_file(path.c_str())) {
            //         throw std::runtime_error("Cannot save scene file.");
            //     }
            // }
            // {
            //     pugi::xml_document doc;
            //
            //     pugi::xml_node volume = doc.append_child("volume");
            //
            //     pugi::xml_node info = volume.append_child("info");
            //     info.append_attribute("type") = "celegans";
            //     info.append_attribute("folder") = "celegans/svdag_occupancy_field";
            //
            //     pugi::xml_node aabbs = volume.append_child("aabbs");
            //     for (uint32_t i = 1; i <= 240; i++) {
            //         pugi::xml_node aabb = aabbs.append_child("aabb");
            //         std::string name = "neuron" + std::to_string(i);
            //         std::string lod = std::to_string(i);
            //         aabb.append_attribute("name") = name.c_str();
            //         aabb.append_attribute("type") = "default";
            //         aabb.append_attribute("lod") = lod.c_str();
            //         aabb.append_attribute("enabled") = true;
            //     }
            //
            //     pugi::xml_node lods = volume.append_child("lods");
            //     for (uint32_t i = 1; i <= 240; i++) {
            //         pugi::xml_node lod = lods.append_child("lod");
            //         std::string name = "neuron" + std::to_string(i);
            //         std::string key = std::to_string(i);
            //         lod.append_attribute("key") = key.c_str();
            //         lod.append_attribute("name") = name.c_str();
            //         lod.append_attribute("type") = "svdag_occupancy_field";
            //     }
            //
            //     std::string path = Paths::m_resourceDirectoryPath + "/scenes/volumes/celegans/svdag_occupancy_field.xml";
            //     if (!doc.save_file(path.c_str())) {
            //         throw std::runtime_error("Cannot save scene file.");
            //     }
            // }

            // // MOUSECORTEX
            // {
            //     pugi::xml_document doc;
            //
            //     pugi::xml_node volume = doc.append_child("volume");
            //
            //     pugi::xml_node info = volume.append_child("info");
            //     info.append_attribute("type") = "mouse";
            //     info.append_attribute("folder") = "mouse/svo";
            //
            //     pugi::xml_node aabbs = volume.append_child("aabbs");
            //     for (uint32_t i = 1; i <= 96; i++) {
            //         pugi::xml_node aabb = aabbs.append_child("aabb");
            //         std::string name = "neuron" + std::to_string(i);
            //         std::string lod = std::to_string(i);
            //         aabb.append_attribute("name") = name.c_str();
            //         aabb.append_attribute("type") = "default";
            //         aabb.append_attribute("lod") = lod.c_str();
            //         aabb.append_attribute("enabled") = true;
            //     }
            //
            //     pugi::xml_node lods = volume.append_child("lods");
            //     for (uint32_t i = 1; i <= 96; i++) {
            //         pugi::xml_node lod = lods.append_child("lod");
            //         std::string name = "neuron" + std::to_string(i);
            //         std::string key = std::to_string(i);
            //         lod.append_attribute("key") = key.c_str();
            //         lod.append_attribute("name") = name.c_str();
            //         lod.append_attribute("type") = "svo";
            //     }
            //
            //     std::string path = Paths::m_resourceDirectoryPath + "/scenes/volumes/mouse/svo.xml";
            //     if (!doc.save_file(path.c_str())) {
            //         throw std::runtime_error("Cannot save scene file.");
            //     }
            // }
            // {
            //     pugi::xml_document doc;
            //
            //     pugi::xml_node volume = doc.append_child("volume");
            //
            //     pugi::xml_node info = volume.append_child("info");
            //     info.append_attribute("type") = "mouse";
            //     info.append_attribute("folder") = "mouse/svdag";
            //
            //     pugi::xml_node aabbs = volume.append_child("aabbs");
            //     for (uint32_t i = 1; i <= 96; i++) {
            //         pugi::xml_node aabb = aabbs.append_child("aabb");
            //         std::string name = "neuron" + std::to_string(i);
            //         std::string lod = std::to_string(i);
            //         aabb.append_attribute("name") = name.c_str();
            //         aabb.append_attribute("type") = "default";
            //         aabb.append_attribute("lod") = lod.c_str();
            //         aabb.append_attribute("enabled") = true;
            //     }
            //
            //     pugi::xml_node lods = volume.append_child("lods");
            //     for (uint32_t i = 1; i <= 96; i++) {
            //         pugi::xml_node lod = lods.append_child("lod");
            //         std::string name = "neuron" + std::to_string(i);
            //         std::string key = std::to_string(i);
            //         lod.append_attribute("key") = key.c_str();
            //         lod.append_attribute("name") = name.c_str();
            //         lod.append_attribute("type") = "svdag";
            //     }
            //
            //     std::string path = Paths::m_resourceDirectoryPath + "/scenes/volumes/mouse/svdag.xml";
            //     if (!doc.save_file(path.c_str())) {
            //         throw std::runtime_error("Cannot save scene file.");
            //     }
            // }
            // {
            //     pugi::xml_document doc;
            //
            //     pugi::xml_node volume = doc.append_child("volume");
            //
            //     pugi::xml_node info = volume.append_child("info");
            //     info.append_attribute("type") = "mouse";
            //     info.append_attribute("folder") = "mouse/svdag_occupancy_field";
            //
            //     pugi::xml_node aabbs = volume.append_child("aabbs");
            //     for (uint32_t i = 1; i <= 96; i++) {
            //         pugi::xml_node aabb = aabbs.append_child("aabb");
            //         std::string name = "neuron" + std::to_string(i);
            //         std::string lod = std::to_string(i);
            //         aabb.append_attribute("name") = name.c_str();
            //         aabb.append_attribute("type") = "default";
            //         aabb.append_attribute("lod") = lod.c_str();
            //         aabb.append_attribute("enabled") = true;
            //     }
            //
            //     pugi::xml_node lods = volume.append_child("lods");
            //     for (uint32_t i = 1; i <= 96; i++) {
            //         pugi::xml_node lod = lods.append_child("lod");
            //         std::string name = "neuron" + std::to_string(i);
            //         std::string key = std::to_string(i);
            //         lod.append_attribute("key") = key.c_str();
            //         lod.append_attribute("name") = name.c_str();
            //         lod.append_attribute("type") = "svdag_occupancy_field";
            //     }
            //
            //     std::string path = Paths::m_resourceDirectoryPath + "/scenes/volumes/mouse/svdag_occupancy_field.xml";
            //     if (!doc.save_file(path.c_str())) {
            //         throw std::runtime_error("Cannot save scene file.");
            //     }
            // }

            //            {
            //                std::vector<std::string> modes = {"svo", "svdag", "svdag_merged", "svdag_occupancy_field", "svdag_occupancy_field_merged"};
            //                std::vector<bool> lods = {true, false};
            //                std::vector<uint32_t> bounces = {32, 1};
            //                for (const auto &mode: modes) {
            //                    for (const auto &lod: lods) {
            //                        for (const auto &bounce: bounces) {
            //                            writeEvaluationSceneFileNastja1000(mode, lod, bounce);
            //                            writeEvaluationSceneFileWorm(mode, lod, bounce);
            //                            writeEvaluationSceneFileMouseCortex(mode, lod, bounce);
            //                        }
            //                    }
            //                }
            //
            //                std::vector<std::string> datasets = {"cells", "celegans", "mouse"};
            //                for (const auto &dataset : datasets) {
            //                    writeEvaluationScript(dataset);
            //                }
            //            }

            pugi::xml_document doc;
            if (pugi::xml_parse_result result = doc.load_file(scenePath.c_str()); !result) {
                std::cerr << "XML [" << scenePath << "] parsed with errors." << std::endl;
                std::cerr << "Error description: " << result.description() << std::endl;
                std::cerr << "Error offset: " << result.offset << std::endl;
                throw std::runtime_error("Cannot parse scene file.");
            }

            // volumes
            int32_t lodType = -1;
            for (const auto &volume: doc.child("scene").child("volumes")) {
                std::string name = volume.attribute("name").value();
                std::string translate = "0 0 0";
                if (!volume.attribute("translate").empty()) {
                    translate = volume.attribute("translate").value();
                }
                std::string scale = "1 1 1";
                if (!volume.attribute("scale").empty()) {
                    scale = volume.attribute("scale").value();
                }

                auto vol = Volume::create(dataPath, name, translate, scale);
                if (!vol) {
                    continue;
                }
                m_volumes.push_back(vol);

                if (lodType == -1) {
                    lodType = vol->getLODType();
                }
                if (lodType != vol->getLODType()) {
                    throw std::runtime_error("LOD types must be the same.");
                }
            }
            sceneSettings->m_lodType = static_cast<uint32_t>(glm::max(0, lodType));

            // load data
            for (const auto &volume: m_volumes) {
                volume->loadData(gpuContext);
            }

            // object descriptors
            // TLAS
            buildTLASAndObjectDescriptors(gpuContext);

            // materials
            std::unique_ptr<MaterialGenerator> materialGenerator;
            std::string materialGeneratorName = doc.child("scene").child("material").attribute("generator").value();
            uint32_t materialCount = doc.child("scene").child("material").attribute("count").as_uint();
            if (materialGeneratorName == "nastja400") {
                materialGenerator = std::make_unique<MaterialGenerator>(MaterialGenerator::default400PropertyGeneratorFunction(), materialCount);
            } else if (materialGeneratorName == "cells") {
                materialGenerator = std::make_unique<MaterialGenerator>(MaterialGenerator::default1000PropertyGeneratorFunction(), materialCount);
            } else {
                throw std::runtime_error("Unknown material generator: " + materialGeneratorName);
            }
            for (uint32_t i = 0; i < materialCount; i++) {
                std::string keyPrefix = "m_folder/m_file/" + std::to_string(0) + "/";

                // materials
                auto material = std::make_shared<SegmentationVolumeMaterial>();

                material->m_material.baseColor = materialGenerator->operator()(i);
                material->m_material.roughness = 0.5;
                material->m_emissiveFactor = material->m_material.baseColor;

                // // plastic
                // material->m_material.specularTransmission = 0.0f;
                // material->m_material.metallic = 0.0f;
                // material->m_material.subsurface = 0.8f;
                // material->m_material.specular = 0.3f;
                // material->m_material.roughness = 0.1f;
                // material->m_material.specularTint = 0.0f;
                // material->m_material.anisotropic = 0.0f;
                // material->m_material.sheen = 1.0f;
                // material->m_material.sheenTint = 0.5f;
                // material->m_material.clearcoat = 1.0f;
                // material->m_material.clearcoatGloss = 0.5f;
                // material->m_material.eta = 1.5f;

                // // all
                // material->m_material.specularTransmission = 0.5f;
                // material->m_material.metallic = 1.0f;
                // material->m_material.subsurface = 0.5f;
                // material->m_material.specular = 0.5f;
                // material->m_material.roughness = 0.2f;
                // material->m_material.specularTint = 0.5f;
                // material->m_material.anisotropic = 0.5f;
                // material->m_material.sheen = 0.5f;
                // material->m_material.sheenTint = 0.5f;
                // material->m_material.clearcoat = 0.5f;
                // material->m_material.clearcoatGloss = 0.5f;
                // material->m_material.eta = 1.5f;

                material->m_key = keyPrefix + std::to_string(i);
                material->m_name = std::to_string(i);

                if (m_materialKeys.contains(material->m_key)) {
                    throw std::runtime_error("Material " + material->m_key + " already exists.");
                }
                m_materialKeys[material->m_key] = static_cast<uint32_t>(m_materials.size());

                material->m_name = findNextFreeName(m_materialNames, material->m_name);
                m_materialNames[material->m_name] = static_cast<uint32_t>(m_materials.size());
                m_materials.push_back(material);
            }
            if (const auto &labelMetadata = doc.child("scene").child("material").attribute("labelMetadata"); !labelMetadata.empty()) {
                sceneSettings->m_labelMetadata = labelMetadata.as_bool();
            }
            if (const auto &labelMetadataJitterAlbedo = doc.child("scene").child("material").attribute("labelMetadataJitterAlbedo"); !labelMetadataJitterAlbedo.empty()) {
                sceneSettings->m_labelMetadataJitterAlbedo = labelMetadataJitterAlbedo.as_bool();
            }

            if (const auto &camera = doc.child("scene").child("camera"); !camera.empty()) {
                if (!camera.attribute("position").empty()) {
                    glm::vec3 cameraPosition;
                    if (!vec3FromString(camera.attribute("position").value(), &cameraPosition)) {
                        throw std::runtime_error("Cannot parse coordinate: " + std::string(camera.attribute("position").value()));
                    }
                    gpuContext->getCamera()->moveCenter(cameraPosition);
                }
                if (!camera.attribute("rotation").empty()) {
                    glm::vec3 cameraRotation;
                    if (!vec3FromString(camera.attribute("rotation").value(), &cameraRotation)) {
                        throw std::runtime_error("Cannot parse coordinate: " + std::string(camera.attribute("rotation").value()));
                    }
                    gpuContext->getCamera()->setRotation(cameraRotation.x, cameraRotation.y, cameraRotation.z);
                }
                if (!camera.attribute("speed").empty()) {
                    gpuContext->getCamera()->setSpeedMultiplier(camera.attribute("speed").as_float());
                }
            }

            if (const auto &renderer = doc.child("scene").child("renderer"); !renderer.empty()) {
                if (!renderer.attribute("maxFrames").empty()) {
                    sceneSettings->m_maxFrames = renderer.attribute("maxFrames").as_uint();
                }
                if (!renderer.attribute("tonemapper").empty()) {
                    sceneSettings->m_tonemapper = renderer.attribute("tonemapper").as_uint();
                }
                if (!renderer.attribute("renderer").empty()) {
                    sceneSettings->m_renderer = renderer.attribute("renderer").as_uint();
                }
                if (!renderer.attribute("debug").empty()) {
                    sceneSettings->m_debug = renderer.attribute("debug").as_uint();
                }
                if (!renderer.attribute("spp").empty()) {
                    sceneSettings->m_spp = renderer.attribute("spp").as_uint();
                }
                if (!renderer.attribute("lod").empty()) {
                    sceneSettings->m_lod = renderer.attribute("lod").as_bool();
                }
                if (!renderer.attribute("lodDisableOnFirstFrame").empty()) {
                    sceneSettings->m_lodDisableOnFirstFrame = renderer.attribute("lodDisableOnFirstFrame").as_bool();
                }
                if (!renderer.attribute("lodDistance").empty()) {
                    sceneSettings->m_lodDistance = renderer.attribute("lodDistance").as_bool();
                }
                if (!renderer.attribute("lodDistanceVoxel").empty()) {
                    sceneSettings->m_lodDistanceVoxel = renderer.attribute("lodDistanceVoxel").as_uint();
                }
                if (!renderer.attribute("lodDistanceOctree").empty()) {
                    sceneSettings->m_lodDistanceOctree = renderer.attribute("lodDistanceOctree").as_uint();
                }
                if (!renderer.attribute("bounces").empty()) {
                    sceneSettings->m_bounces = renderer.attribute("bounces").as_uint();
                }
            }

            if (const auto &window = doc.child("scene").child("window"); !window.empty()) {
                if (!window.attribute("width").empty() && !window.attribute("height").empty()) {
                    int width = window.attribute("width").as_int();
                    int height = window.attribute("height").as_int();
                    if (width > 0 && height > 0) {
                        gpuContext->resizeWindow(width, height);
                    }
                }
            }

            initMaterials(gpuContext);

            // environment texture
            {
                const std::string TEXTURE_PATH = Paths::m_resourceDirectoryPath + "/environment/white.png";
                // const std::string TEXTURE_PATH = Paths::m_resourceDirectoryPath + "/assets/lonely_road_afternoon_puresky_8k.hdr";
                // const std::string TEXTURE_PATH = Paths::m_resourceDirectoryPath + "/assets/metro_noord_4k.hdr";
                // const std::string TEXTURE_PATH = Paths::m_resourceDirectoryPath + "/assets/hayloft_4k.hdr";
                auto settings = Image::ImageSettings{.m_mipMapping = true, .m_name = "environment"};
                m_textureEnvironment = std::make_shared<Texture>(gpuContext, settings, TEXTURE_PATH);
                m_textureEnvironment->create();
            }

            uint64_t aabbBuffersSize = getAABBBuffersSize();
            uint64_t blasBuffersSize = getBLASBuffersSize();
            uint64_t lodBuffersSize = getLODBuffersSize();
            uint64_t tlasBufferSize = getTLASBufferSize();
            std::cout << "AABB buffers: " << aabbBuffersSize << " bytes (" << static_cast<float>(aabbBuffersSize) * glm::pow(10, -6) << " MB)" << std::endl;
            std::cout << "LOD buffers: " << lodBuffersSize << " bytes (" << static_cast<float>(lodBuffersSize) * glm::pow(10, -6) << " MB)" << std::endl;
            std::cout << "BLAS buffers: " << blasBuffersSize << " bytes (" << static_cast<float>(blasBuffersSize) * glm::pow(10, -6) << " MB)" << std::endl;
            std::cout << "TLAS buffer: " << tlasBufferSize << " bytes (" << static_cast<float>(tlasBufferSize) * glm::pow(10, -6) << " MB)" << std::endl;
            std::cout << "Total: " << aabbBuffersSize + lodBuffersSize + blasBuffersSize + tlasBufferSize << " bytes (" << static_cast<float>(aabbBuffersSize + lodBuffersSize + blasBuffersSize + tlasBufferSize) * glm::pow(10, -6) << " MB)" << std::endl;
        }

        void release() const {
            RAVEN_IMAGE_RELEASE(m_textureEnvironment);

            RAVEN_BUFFER_RELEASE(m_objectDescriptorBuffer);
            RAVEN_BUFFER_RELEASE(m_materialBuffer);

            m_tlas->release();

            for (const auto &volume: m_volumes) {
                volume->release();
            }
        }

        void guiScene(GPUContext *gpuContext, const std::function<void()> &recordGUIFunctionMaterialMetadata, bool *updateMaterial, bool *updateLight, uint32_t *numLights, bool *updateTLAS) {
            // if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::CollapsingHeader("Scene Hierarchy")) {
                if (ImGui::Button("Enable All")) {
                    for (const auto &volume: m_volumes) {
                        volume->enableAllAABBs();
                    }
                    buildTLASAndObjectDescriptors(gpuContext);
                    *updateTLAS = true;
                }
                if (ImGui::Button("Disable All")) {
                    for (const auto &volume: m_volumes) {
                        volume->disableAllAABBs();
                    }
                    buildTLASAndObjectDescriptors(gpuContext);
                    *updateTLAS = true;
                }
                for (const auto &volume: m_volumes) {
                    volume->recordHierarchyGUI([this, gpuContext] { buildTLASAndObjectDescriptors(gpuContext); }, updateTLAS);
                }
                // ImGui::TreePop();
            }
            if (ImGui::CollapsingHeader("Materials")) {
                recordGUIFunctionMaterialMetadata();
                std::vector<std::string> materialNames;
                std::vector<const char *> materials;
                for (const auto &material: m_materials) {
                    materialNames.push_back(material->m_key + " (" + material->m_name + ")");
                    materials.push_back(materialNames.back().c_str());
                }
                if (!materials.empty()) {
                    ImGui::Combo("Material", &m_selectedMaterial, materials.data(), static_cast<int>(materials.size()));

                    if (m_selectedMaterial >= 0 && m_selectedMaterial < materials.size()) {
                        auto &materialInfo = m_materials[m_selectedMaterial];
                        auto &material = materialInfo->m_material;

                        *updateMaterial = false;
                        *updateLight = false;
                        if (ImGui::ColorEdit3("Base Color", &material.baseColor.x)) {
                            *updateMaterial = true;
                        }
                        if (ImGui::SliderFloat("Specular Transmission", &material.specularTransmission, 0.f, 1.f)) {
                            *updateMaterial = true;
                        }
                        if (ImGui::SliderFloat("Metallic", &material.metallic, 0.f, 1.f)) {
                            *updateMaterial = true;
                        }
                        if (ImGui::SliderFloat("Subsurface", &material.subsurface, 0.f, 1.f)) {
                            *updateMaterial = true;
                        }
                        if (ImGui::SliderFloat("Specular", &material.specular, 0.f, 1.f)) {
                            *updateMaterial = true;
                        }
                        if (ImGui::SliderFloat("Roughness", &material.roughness, 0.f, 1.f)) {
                            *updateMaterial = true;
                        }
                        if (ImGui::SliderFloat("Specular Tint", &material.specularTint, 0.f, 1.f)) {
                            *updateMaterial = true;
                        }
                        if (ImGui::SliderFloat("Anisotropic", &material.anisotropic, 0.f, 1.f)) {
                            *updateMaterial = true;
                        }
                        if (ImGui::SliderFloat("Sheen", &material.sheen, 0.f, 1.f)) {
                            *updateMaterial = true;
                        }
                        if (ImGui::SliderFloat("Sheen Tint", &material.sheenTint, 0.f, 1.f)) {
                            *updateMaterial = true;
                        }
                        if (ImGui::SliderFloat("Clearcoat", &material.clearcoat, 0.f, 1.f)) {
                            *updateMaterial = true;
                        }
                        if (ImGui::SliderFloat("Clearcoat Gloss", &material.clearcoatGloss, 0.f, 1.f)) {
                            *updateMaterial = true;
                        }
                        if (ImGui::InputFloat("Eta", &material.eta)) {
                            *updateMaterial = true;
                        }

                        if (ImGui::ColorEdit3("Emissive Factor", &materialInfo->m_emissiveFactor.x)) {
                            materialInfo->updateEmission();
                            *updateMaterial = true;
                            *updateLight = true;
                        }
                        if (ImGui::SliderFloat("Emissive Strength", &materialInfo->m_emissiveStrength, 0.f, 1.f)) {
                            materialInfo->updateEmission();
                            *updateMaterial = true;
                            *updateLight = true;
                        }

                        if (*updateMaterial) {
                            initMaterials(gpuContext);
                        }
                        // if (*updateLight) {
                        //     initLights(gpuContext, numLights);
                        // }
                    }
                }
                // ImGui::TreePop();
            }
            // }
        }

        [[nodiscard]] uint64_t getAABBBuffersSize() const {
            uint64_t aabbBuffersSize = 0;
            for (const auto &volume: m_volumes) {
                aabbBuffersSize += volume->getAABBBufferSize();
            }
            return aabbBuffersSize;
        }

        [[nodiscard]] uint64_t getLODBuffersSize() const {
            uint64_t lodBuffersSize = 0;
            for (const auto &volume: m_volumes) {
                lodBuffersSize += volume->getLODBufferSize();
            }
            return lodBuffersSize;
        }

        [[nodiscard]] uint64_t getBLASBuffersSize() const {
            uint64_t blasBuffersSize = 0;
            for (const auto &volume: m_volumes) {
                blasBuffersSize += volume->getBLASSize();
            }
            return blasBuffersSize;
        }

        [[nodiscard]] uint64_t getTLASBufferSize() const {
            const uint64_t tlasBufferSize = m_tlas->getSizeBytes();
            return tlasBufferSize;
        }

    private:
        int32_t m_selectedMaterial = -1;

        static std::string findNextFreeName(const std::map<std::string, uint32_t> &names, std::string &name) {
            if (!names.contains(name)) {
                return name;
            }
            uint32_t i = 0;
            while (names.contains(name + std::to_string(i))) {
                i++;
            }
            return name + std::to_string(i);
        }

        void initMaterials(GPUContext *gpuContext) {
            gpuContext->m_device.waitIdle();

            if (m_materialBuffer) {
                m_materialBuffer->release();
            }
            // materials
            {
                std::vector<SegmentationVolumeMaterial::Material> materials;
                for (const auto &material: m_materials) {
                    materials.push_back(material->m_material);
                }
                if (materials.empty()) {
                    materials.emplace_back();
                }
                const auto bufferSettings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(sizeof(SegmentationVolumeMaterial::Material) * materials.size()), .m_bufferUsages = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal, .m_name = "materials"};
                m_materialBuffer = Buffer::fillDeviceWithStagingBuffer(gpuContext, bufferSettings, materials.data());
            }
        }

        // void initLights(GPUContext *gpuContext, uint32_t *numLights) {
        //     gpuContext->m_device.waitIdle();
        //
        //     if (m_lightBuffer) {
        //         m_lightBuffer->release();
        //     }
        //     // lights
        //     {
        //         m_scene.recordLights();
        //         std::vector<RavenLight::Light> lights;
        //         for (const auto &light: m_scene.m_lights) {
        //             lights.push_back(light->m_light);
        //         }
        //         if (lights.empty()) {
        //             lights.emplace_back();
        //         }
        //         auto bufferSettings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(sizeof(RavenLight::Light) * lights.size()), .m_bufferUsages = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal, .m_name = "lights"};
        //         m_lightBuffer = Buffer::fillDeviceWithStagingBuffer(gpuContext, bufferSettings, lights.data());
        //         *numLights = m_scene.m_lights.size();
        //     }
        // }

        void buildTLASAndObjectDescriptors(GPUContext *gpuContext) {
            gpuContext->m_device.waitIdle();

            {
                RAVEN_AS_RELEASE(m_tlas);

                m_tlas = std::make_shared<TLAS>(gpuContext);

                m_instanceCount = 0;
                for (const auto &volume: m_volumes) {
                    volume->buildBLAS(gpuContext);

                    if (auto blasHandle = volume->createBLASHandle(m_instanceCount); blasHandle.has_value()) {
                        m_tlas->addBLAS(blasHandle.value());
                    }
                }

                if (m_instanceCount == 0) {
                    return;
                }

                m_tlas->build();
            }

            {
                if (m_objectDescriptorBuffer) {
                    RAVEN_BUFFER_RELEASE(m_objectDescriptorBuffer);
                    m_objectDescriptorBuffer = nullptr;
                }

                std::vector<ObjectDescriptor> objectDescriptors;
                for (const auto &volume: m_volumes) {
                    if (auto objectDescriptor = volume->createObjectDescriptor(); objectDescriptor.has_value()) {
                        objectDescriptors.push_back(objectDescriptor.value());
                    }
                }

                if (!objectDescriptors.empty()) {
                    // object descriptor buffer
                    const auto settings = Buffer::BufferSettings{.m_sizeBytes = static_cast<uint32_t>(sizeof(ObjectDescriptor) * objectDescriptors.size()), .m_bufferUsages = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, .m_memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal, .m_name = "ObjectDescriptorBuffer"};
                    m_objectDescriptorBuffer = Buffer::fillDeviceWithStagingBuffer(gpuContext, settings, objectDescriptors.data());
                }
            }
        }

        static bool ivec3FromString(const std::string &str, glm::ivec3 *coordinate) {
            const std::regex rgx("([-+]?[0-9]+) ([-+]?[0-9]+) ([-+]?[0-9]+)");
            std::smatch matches;
            std::regex_search(str, matches, rgx);
            if (matches.size() == 4) {
                coordinate->x = std::stoi(matches[1]);
                coordinate->y = std::stoi(matches[2]);
                coordinate->z = std::stoi(matches[3]);
                return true;
            }
            return false;
        }

        static bool vec3FromString(const std::string &str, glm::vec3 *position) {
            const std::regex rgx("([+-]?[0-9]*[.]?[0-9]+) ([+-]?[0-9]*[.]?[0-9]+) ([+-]?[0-9]*[.]?[0-9]+)");
            std::smatch matches;
            std::regex_search(str, matches, rgx);
            if (matches.size() == 4) {
                position->x = std::stof(matches[1]);
                position->y = std::stof(matches[2]);
                position->z = std::stof(matches[3]);
                return true;
            }
            return false;
        }

        static void writeEvaluationSceneFileNastja1000(const std::string &mode, const bool lod, const uint32_t bounces) {
            pugi::xml_document doc;

            pugi::xml_node scene = doc.append_child("scene");

            pugi::xml_node volumes = scene.append_child("volumes");
            pugi::xml_node volume = volumes.append_child("volume");
            const std::string volumeName = "cells/" + mode;
            volume.append_attribute("name") = volumeName.c_str();
            volume.append_attribute("translate") = "0 0 0";
            volume.append_attribute("scale") = "1 1 1";

            pugi::xml_node camera = scene.append_child("camera");
            camera.append_attribute("position") = "99 740 -215";
            camera.append_attribute("rotation") = "0 3.660 1.907";
            camera.append_attribute("speed") = "50";

            pugi::xml_node material = scene.append_child("material");
            material.append_attribute("generator") = "cells";
            material.append_attribute("count") = 256;
            material.append_attribute("labelMetadata") = true;
            material.append_attribute("labelMetadataJitterAlbedo") = true;

            pugi::xml_node renderer = scene.append_child("renderer");
            renderer.append_attribute("maxFrames") = 1024;
            renderer.append_attribute("tonemapper") = 1;
            renderer.append_attribute("renderer") = 0;
            renderer.append_attribute("debug") = 0;
            renderer.append_attribute("spp") = 1;
            renderer.append_attribute("lod") = lod ? 1 : 0;
            renderer.append_attribute("lodDisableOnFirstFrame") = false;
            renderer.append_attribute("lodDistance") = false;
            renderer.append_attribute("lodDistanceVoxel") = 512;
            renderer.append_attribute("lodDistanceOctree") = 1024;
            renderer.append_attribute("bounces") = bounces;

            pugi::xml_node window = scene.append_child("window");
            window.append_attribute("width") = "1920";
            window.append_attribute("height") = "1080";

            const std::string path = Paths::m_resourceDirectoryPath + "/scenes/evaluation_cells_" + mode + "_" + (lod ? "wlod" : "wolod") + "_" + std::to_string(bounces) + ".xml";
            if (!doc.save_file(path.c_str())) {
                throw std::runtime_error("Cannot save scene file.");
            }
        }

        static void writeEvaluationSceneFileWorm(const std::string &mode, const bool lod, const uint32_t bounces) {
            pugi::xml_document doc;

            pugi::xml_node scene = doc.append_child("scene");

            pugi::xml_node volumes = scene.append_child("volumes");
            pugi::xml_node volume = volumes.append_child("volume");
            const std::string volumeName = "celegans/" + mode;
            volume.append_attribute("name") = volumeName.c_str();
            volume.append_attribute("translate") = "0 0 0";
            volume.append_attribute("scale") = "30 1 1";

            pugi::xml_node camera = scene.append_child("camera");
            camera.append_attribute("position") = "3733 15955 11733";
            camera.append_attribute("rotation") = "0 3.525 1.944";
            camera.append_attribute("speed") = "1000";

            pugi::xml_node material = scene.append_child("material");
            material.append_attribute("generator") = "cells";
            material.append_attribute("count") = 512;

            pugi::xml_node renderer = scene.append_child("renderer");
            renderer.append_attribute("maxFrames") = 1024;
            renderer.append_attribute("tonemapper") = 1;
            renderer.append_attribute("renderer") = 0;
            renderer.append_attribute("debug") = 0;
            renderer.append_attribute("spp") = 1;
            renderer.append_attribute("lod") = lod ? 1 : 0;
            renderer.append_attribute("lodDisableOnFirstFrame") = false;
            renderer.append_attribute("lodDistance") = false;
            renderer.append_attribute("lodDistanceVoxel") = 512;
            renderer.append_attribute("lodDistanceOctree") = 1024;
            renderer.append_attribute("bounces") = bounces;

            pugi::xml_node window = scene.append_child("window");
            window.append_attribute("width") = "1920";
            window.append_attribute("height") = "1080";

            const std::string path = Paths::m_resourceDirectoryPath + "/scenes/evaluation_celegans_" + mode + "_" + (lod ? "wlod" : "wolod") + "_" + std::to_string(bounces) + ".xml";
            if (!doc.save_file(path.c_str())) {
                throw std::runtime_error("Cannot save scene file.");
            }
        }

        static void writeEvaluationSceneFileMouseCortex(const std::string &mode, const bool lod, const uint32_t bounces) {
            pugi::xml_document doc;

            pugi::xml_node scene = doc.append_child("scene");

            pugi::xml_node volumes = scene.append_child("volumes");
            pugi::xml_node volume = volumes.append_child("volume");
            const std::string volumeName = "mouse/" + mode;
            volume.append_attribute("name") = volumeName.c_str();
            volume.append_attribute("translate") = "0 0 0";
            volume.append_attribute("scale") = "1 1 1";

            pugi::xml_node camera = scene.append_child("camera");
            camera.append_attribute("position") = "10629 9683 -4337";
            camera.append_attribute("rotation") = "0 2.264 2.081";
            camera.append_attribute("speed") = "1000";

            pugi::xml_node material = scene.append_child("material");
            material.append_attribute("generator") = "cells";
            material.append_attribute("count") = 512;

            pugi::xml_node renderer = scene.append_child("renderer");
            renderer.append_attribute("maxFrames") = 1024;
            renderer.append_attribute("tonemapper") = 1;
            renderer.append_attribute("renderer") = 0;
            renderer.append_attribute("debug") = 0;
            renderer.append_attribute("spp") = 1;
            renderer.append_attribute("lod") = lod ? 1 : 0;
            renderer.append_attribute("lodDisableOnFirstFrame") = false;
            renderer.append_attribute("lodDistance") = false;
            renderer.append_attribute("lodDistanceVoxel") = 512;
            renderer.append_attribute("lodDistanceOctree") = 1024;
            renderer.append_attribute("bounces") = bounces;

            pugi::xml_node window = scene.append_child("window");
            window.append_attribute("width") = "1080";
            window.append_attribute("height") = "1920";

            const std::string path = Paths::m_resourceDirectoryPath + "/scenes/evaluation_mouse_" + mode + "_" + (lod ? "wlod" : "wolod") + "_" + std::to_string(bounces) + ".xml";
            if (!doc.save_file(path.c_str())) {
                throw std::runtime_error("Cannot save scene file.");
            }
        }

        static void writeEvaluationScript(const std::string &dataset) {
            std::ofstream script(Paths::m_resourceDirectoryPath + "/scenes/evaluation_" + dataset + ".sh");
            if (!script.is_open()) {
                throw std::runtime_error("Cannot open script file.");
            }

            script << "#!/bin/bash\n\n";
            script << "data=...\n\n";

            std::vector<std::string> modes = {"svo", "svdag", "svdag_merged", "svdag_occupancy_field", "svdag_occupancy_field_merged"};
            std::vector lods = {true, false};
            std::vector<uint32_t> bounces = {32, 1};
            for (const auto &mode: modes) {
                for (const auto &lod: lods) {
                    for (const auto &bounce: bounces) {
                        script << "./segmentationvolumes $data evaluation_" + dataset + "_" + mode + "_" + (lod ? "wlod" : "wolod") + "_" + std::to_string(bounce) + " --evaluate\n";
                    }
                }
            }

            script.close();
        }
    };
} // namespace raven