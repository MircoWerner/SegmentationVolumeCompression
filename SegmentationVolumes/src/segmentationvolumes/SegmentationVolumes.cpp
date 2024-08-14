#include "segmentationvolumes/SegmentationVolumes.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "raven/util/ImagePFM.h"
#include "stb/stb_image_write.h"

#include <complex>
#include <random>
#include <utility>

namespace raven {
    SegmentationVolumes::SegmentationVolumes(SegmentationVolumesSettings settings) : m_settings(std::move(settings)) {}

    void SegmentationVolumes::initResources(GPUContext *gpuContext) {
        gpuContext->getCamera()->registerCameraUpdateCallback([this, gpuContext] {
            m_renderOptions.m_frame = 0;

            const glm::mat4 view_to_clip_space = gpuContext->getCamera()->getViewToClipSpace();
            const glm::mat4 world_to_view_space = gpuContext->getCamera()->getWorldToViewSpace();
            const glm::mat4 vp = view_to_clip_space * world_to_view_space;
            m_renderOptions.m_vp_prev = m_renderOptions.m_vp;
            m_renderOptions.m_vp = vp;
        });

        SegmentationVolumesScene::SceneSettings sceneSettings;
        m_scene.load(gpuContext, m_settings.m_data, m_settings.m_scene, &sceneSettings);
        if (sceneSettings.m_maxFrames.has_value()) {
            m_renderOptions.m_maxFrames = sceneSettings.m_maxFrames.value();
        }
        if (sceneSettings.m_tonemapper.has_value()) {
            m_renderOptions.m_tonemapper = sceneSettings.m_tonemapper.value();
        }
        if (sceneSettings.m_renderer.has_value()) {
            m_renderOptions.m_renderer = sceneSettings.m_renderer.value();
        }
        if (sceneSettings.m_debug.has_value()) {
            m_renderOptions.m_debug = sceneSettings.m_debug.value();
        }
        if (sceneSettings.m_spp.has_value()) {
            m_renderOptions.m_spp = sceneSettings.m_spp.value();
        }
        if (sceneSettings.m_lodType.has_value()) {
            m_renderOptions.m_lodType = sceneSettings.m_lodType.value();
        }
        if (sceneSettings.m_lod.has_value()) {
            m_renderOptions.m_lod = sceneSettings.m_lod.value();
        }
        if (sceneSettings.m_lodDisableOnFirstFrame.has_value()) {
            m_renderOptions.m_lodDisableOnFirstFrame = sceneSettings.m_lodDisableOnFirstFrame.value();
        }
        if (sceneSettings.m_lodDistance.has_value()) {
            m_renderOptions.m_lodDistance = sceneSettings.m_lodDistance.value();
        }
        if (sceneSettings.m_lodDistanceVoxel.has_value()) {
            m_renderOptions.m_lodDistanceVoxel = sceneSettings.m_lodDistanceVoxel.value();
        }
        if (sceneSettings.m_lodDistanceOctree.has_value()) {
            m_renderOptions.m_lodDistanceOctree = sceneSettings.m_lodDistanceOctree.value();
        }
        if (sceneSettings.m_bounces.has_value()) {
            m_renderOptions.m_bounces = sceneSettings.m_bounces.value();
        }
        if (sceneSettings.m_labelMetadata.has_value()) {
            m_renderOptions.m_labelMetadata = sceneSettings.m_labelMetadata.value();
        }
        if (sceneSettings.m_labelMetadataJitterAlbedo.has_value()) {
            m_renderOptions.m_labelMetadataJitterAlbedo = sceneSettings.m_labelMetadataJitterAlbedo.value();
        }

        m_renderOptions.m_numLights = 0;

        // m_bSplinePosition.m_controlPoints.emplace_back(99, 740, -215);
        // m_bSplinePosition.m_controlPoints.emplace_back(99, 740, -215);
        // m_bSplinePosition.m_controlPoints.emplace_back(420, 740, -84);
        // m_bSplinePosition.m_controlPoints.emplace_back(511, 740, -16);
        // m_bSplinePosition.m_controlPoints.emplace_back(597, 740, 49);
        // m_bSplinePosition.m_controlPoints.emplace_back(640, 655, 325);
        // m_bSplinePosition.m_controlPoints.emplace_back(599, 655, 484);
        // m_bSplinePosition.m_controlPoints.emplace_back(517, 643, 613);
        // m_bSplinePosition.m_controlPoints.emplace_back(474, 643, 859);
        // m_bSplinePosition.m_controlPoints.emplace_back(474, 643, 859);
        //
        // m_bSplineOrientation.m_controlPoints.emplace_back(0, 3.648f, 1.774f);
        // m_bSplineOrientation.m_controlPoints.emplace_back(0, 3.648f, 1.774f);
        // m_bSplineOrientation.m_controlPoints.emplace_back(0, 3.648f, 1.774f);
        // m_bSplineOrientation.m_controlPoints.emplace_back(0, 3.877f, 1.657f);
        // m_bSplineOrientation.m_controlPoints.emplace_back(0, 3.686f, 1.298f);
        // m_bSplineOrientation.m_controlPoints.emplace_back(0, 3.305f, 1.583f);
        // m_bSplineOrientation.m_controlPoints.emplace_back(0, 2.941f, 1.633f);
        // m_bSplineOrientation.m_controlPoints.emplace_back(0, 2.727f, 1.730f);
        // m_bSplineOrientation.m_controlPoints.emplace_back(0, 2.727f, 1.730f);
        // m_bSplineOrientation.m_controlPoints.emplace_back(0, 2.727f, 1.730f);

        // float offset = 512.f;
        // glm::vec3 p0 = glm::vec3(-offset, 512, -offset);
        // glm::vec3 p1 = glm::vec3(-offset, 512, 1024 + offset);
        // glm::vec3 p2 = glm::vec3(1024 + offset, 512, 1024 + offset);
        // glm::vec3 p3 = glm::vec3(1024 + offset, 512, -offset);
        //
        // glm::vec3 o0 = glm::vec3(0, 5.f / 4.f * glm::pi<float>(), 1.571f); // 5 / 4 PI
        // glm::vec3 o1 = glm::vec3(0, 7.f / 4.f * glm::pi<float>(), 1.571f); // 7 / 4 PI
        // glm::vec3 o2 = glm::vec3(0, 9.f / 4.f * glm::pi<float>(), 1.571f); // 9 / 4 PI
        // glm::vec3 o3 = glm::vec3(0, 11.f / 4.f * glm::pi<float>(), 1.571f); // 11 / 4 PI
        // glm::vec3 o4 = glm::vec3(0, 13.f / 4.f * glm::pi<float>(), 1.571f); // 13 / 4 PI
        // glm::vec3 o5 = glm::vec3(0, 15.f / 4.f * glm::pi<float>(), 1.571f); // 15 / 4 PI
        // glm::vec3 o6 = glm::vec3(0, 17.f / 4.f * glm::pi<float>(), 1.571f); // 17 / 4 PI
        //
        // m_bSplinePosition.m_controlPoints.push_back(p0);
        // m_bSplinePosition.m_controlPoints.push_back(p1);
        // m_bSplinePosition.m_controlPoints.push_back(p2);
        // m_bSplinePosition.m_controlPoints.push_back(p3);
        // m_bSplinePosition.m_controlPoints.push_back(p0);
        // m_bSplinePosition.m_controlPoints.push_back(p1);
        // m_bSplinePosition.m_controlPoints.push_back(p2);
        //
        // m_bSplineOrientation.m_controlPoints.push_back(o0);
        // m_bSplineOrientation.m_controlPoints.push_back(o1);
        // m_bSplineOrientation.m_controlPoints.push_back(o2);
        // m_bSplineOrientation.m_controlPoints.push_back(o3);
        // m_bSplineOrientation.m_controlPoints.push_back(o4);
        // m_bSplineOrientation.m_controlPoints.push_back(o5);
        // m_bSplineOrientation.m_controlPoints.push_back(o6);
    }

    void SegmentationVolumes::initShaderResources(GPUContext *gpuContext) {
        // pre processing passes
        m_preProcessingPass = std::make_shared<PassCompute>(gpuContext, Pass::PassSettings{.m_name = "preProcessingPass"});
        m_preProcessingPass->create();

        m_passDebug = std::make_shared<PassDebug>(gpuContext, m_renderOptions.m_lodType);
        m_passDebug->create();
        m_uniformPassDebug = m_passDebug->getUniform(0, 3);
        m_passDebug->setAccelerationStructure(0, 0, m_scene.m_tlas->getAccelerationStructure());
        m_passDebug->setStorageBuffer(0, 10, m_scene.m_objectDescriptorBuffer.get());
        m_passDebug->setSamplerImage(0, 21, m_scene.m_textureEnvironment.get(), vk::ImageLayout::eReadOnlyOptimal);

        // segmentation volumes passes
        m_SegmentationVolumesPass = std::make_shared<PassCompute>(gpuContext, Pass::PassSettings{.m_name = "SegmentationVolumesPass"});
        m_SegmentationVolumesPass->create();

        if (m_settings.m_rayquery) {
            m_segmentationVolumesPassPathtrace = std::make_shared<SegmentationVolumesPassPathtraceRayQuery>(gpuContext, m_renderOptions.m_lodType);
        } else {
            m_segmentationVolumesPassPathtrace = std::make_shared<SegmentationVolumesPassPathtraceRayTracing>(gpuContext, m_renderOptions.m_lodType);
        }
        m_segmentationVolumesPassPathtrace->create();
        m_segmentationVolumesPassPathtracePushConstants = std::make_shared<SegmentationVolumesPassPathtrace>();
        m_uniformSegmentationVolumesPassPathtrace = m_segmentationVolumesPassPathtrace->getUniform(0, 3);
        m_segmentationVolumesPassPathtrace->setAccelerationStructure(0, 0, m_scene.m_tlas->getAccelerationStructure());
        m_segmentationVolumesPassPathtrace->setStorageBuffer(0, 10, m_scene.m_objectDescriptorBuffer.get());
        m_segmentationVolumesPassPathtrace->setSamplerImage(0, 21, m_scene.m_textureEnvironment.get(), vk::ImageLayout::eReadOnlyOptimal);
        if (!m_settings.m_rayquery) {
            m_uniformSegmentationVolumesPassPathtraceIntersection = m_segmentationVolumesPassPathtrace->getUniform(1, 3);
            m_segmentationVolumesPassPathtrace->setStorageBuffer(1, 10, m_scene.m_objectDescriptorBuffer.get());
        }

        // post processing passes
        m_postProcessingPass = std::make_shared<PassCompute>(gpuContext, Pass::PassSettings{.m_name = "postProcessingPass"});
        m_postProcessingPass->create();

        m_passTonemapper = std::make_shared<PassTonemapper>(gpuContext);
        m_passTonemapper->create();

        passesSetMaterialBuffer();
        // passesSetLightBuffer();
    }

    void SegmentationVolumes::initSwapchainResources(GPUContext *gpuContext, vk::Extent2D extent) {
        m_renderOptions.m_pixels = {extent.width, extent.height};

        m_passDebug->setGlobalInvocationSize(extent.width, extent.height, 1);

        m_segmentationVolumesPassPathtrace->setGlobalInvocationSize(extent.width, extent.height, 1);

        m_passTonemapper->setGlobalInvocationSize(extent.width, extent.height, 1);

        {
            auto settings = Image::ImageSettings{.m_width = extent.width, .m_height = extent.height, .m_format = vk::Format::eR32G32B32A32Sfloat, .m_usageFlags = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst, .m_name = "frameBuffer"};
            m_imageFramebuffer = std::make_shared<Image>(gpuContext, settings);
            m_imageFramebuffer->create();
            m_imageFramebuffer->setImageLayout(vk::ImageLayout::eGeneral);
            m_passDebug->setStorageImage(0, 1, m_imageFramebuffer.get());
            m_segmentationVolumesPassPathtrace->setStorageImage(0, 1, m_imageFramebuffer.get());
            m_passTonemapper->setStorageImage(0, 0, m_imageFramebuffer.get());
        }
        {
            auto settings = Image::ImageSettings{.m_width = extent.width, .m_height = extent.height, .m_format = vk::Format::eR32G32B32A32Sfloat, .m_usageFlags = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc, .m_name = "accumulationBuffer"};
            m_imageAccumulationBuffer = std::make_shared<Image>(gpuContext, settings);
            m_imageAccumulationBuffer->create();
            m_imageAccumulationBuffer->setImageLayout(vk::ImageLayout::eGeneral);
            m_passDebug->setStorageImage(0, 2, m_imageAccumulationBuffer.get());
            m_segmentationVolumesPassPathtrace->setStorageImage(0, 2, m_imageAccumulationBuffer.get());
        }

        resetFrame(gpuContext);
    }

    void SegmentationVolumes::recordImGui(GPUContext *gpuContext, Camera *camera) {
        ImGui::Begin("Settings");

        const glm::vec3 position = camera->getPosition();
        const glm::vec3 rotation = camera->getSphericalRotation();
        const glm::ivec3 voxel = {static_cast<int>(glm::floor(position.x)), static_cast<int>(glm::floor(position.y)), static_cast<int>(glm::floor(position.z))};
        ImGui::Text("Voxel (X,Y,Z) = (%i,%i,%i)", voxel.x, voxel.y, voxel.z);
        ImGui::Text("Position (X,Y,Z) = (%.3f,%.3f,%.3f)", position.x, position.y, position.z);
        ImGui::Text("Rotation (r,theta,phi) = (%.3f,%.3f,%.3f)", rotation.x, rotation.y, rotation.z);
        ImGui::Text("Frames: %u/%u", m_renderOptions.m_frame, m_renderOptions.m_maxFrames);
        ImGui::InputInt("Max Frames", reinterpret_cast<int *>(&m_renderOptions.m_maxFrames), 1);

        if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (const char *rendererItems[] = {"OFF", "GAMMA_CORRECTION ONLY", "REINHARD + GAMMA_CORRECTION"}; ImGui::Combo("Tone-Mapping", reinterpret_cast<int32_t *>(&m_renderOptions.m_tonemapper), rendererItems, IM_ARRAYSIZE(rendererItems))) {
                resetFrame(gpuContext);
            }

            ImGui::Separator();

            if (const char *rendererItems[] = {"SEGMENTATION_VOLUMES", "DEBUG"}; ImGui::Combo("Renderer", reinterpret_cast<int32_t *>(&m_renderOptions.m_renderer), rendererItems, IM_ARRAYSIZE(rendererItems))) {
                resetFrame(gpuContext);
            }
            if (m_renderOptions.m_renderer == RENDERER_SEGMENTATIONVOLUMES) {
                if (ImGui::InputInt("Samples per Pixel per Frame", reinterpret_cast<int *>(&m_renderOptions.m_spp), 1)) {
                    m_renderOptions.m_spp = glm::clamp(m_renderOptions.m_spp, 1u, 128u);
                    resetFrame(gpuContext);
                }
                if (ImGui::InputInt("Max Bounces", reinterpret_cast<int *>(&m_renderOptions.m_bounces), 1)) {
                    m_renderOptions.m_bounces = glm::clamp(m_renderOptions.m_bounces, 1u, 32u);
                    resetFrame(gpuContext);
                }
            }
            if (m_renderOptions.m_renderer == RENDERER_DEBUG) {
                if (const char *debugItems[] = {"POSITION", "NORMAL", "BASE_COLOR", "OBJECT_DESCRIPTOR", "PRIMITIVE", "COLORMAPS"}; ImGui::Combo("Debug Mode", reinterpret_cast<int32_t *>(&m_renderOptions.m_debug), debugItems, IM_ARRAYSIZE(debugItems))) {
                    resetFrame(gpuContext);
                }
            }

            ImGui::Separator();

            if (ImGui::Checkbox("Level of Detail (LOD)", &m_renderOptions.m_lod)) {
                resetFrame(gpuContext);
            }
            if (m_renderOptions.m_lod) {
                if (ImGui::Checkbox("LOD: Disable on First Frame", &m_renderOptions.m_lodDisableOnFirstFrame)) {
                    resetFrame(gpuContext);
                }
                if (ImGui::Checkbox("LOD: Smooth Level Decision", &m_renderOptions.m_lodDistance)) {
                    resetFrame(gpuContext);
                }
                if (m_renderOptions.m_lodDistance) {
                    if (ImGui::InputInt("LOD: Max Distance Voxel (finest)", reinterpret_cast<int *>(&m_renderOptions.m_lodDistanceVoxel), 1)) {
                        m_renderOptions.m_lodDistanceVoxel = glm::clamp(m_renderOptions.m_lodDistanceVoxel, 0u, 100000u);
                        m_renderOptions.m_lodDistanceOctree = glm::clamp(m_renderOptions.m_lodDistanceOctree, m_renderOptions.m_lodDistanceVoxel, 100000u);
                        resetFrame(gpuContext);
                    }
                    if (ImGui::InputInt("LOD: Max Distance SVDAG (middle)", reinterpret_cast<int *>(&m_renderOptions.m_lodDistanceOctree), 1)) {
                        m_renderOptions.m_lodDistanceOctree = glm::clamp(m_renderOptions.m_lodDistanceOctree, 0u, 100000u);
                        m_renderOptions.m_lodDistanceVoxel = glm::clamp(m_renderOptions.m_lodDistanceVoxel, 0u, m_renderOptions.m_lodDistanceOctree);
                        resetFrame(gpuContext);
                    }
                    ImGui::Text("Finest LOD (traverse SVDAGs and occupancy fields up to 1^3): 0 < t <= %d", m_renderOptions.m_lodDistanceVoxel);
                    ImGui::Text("Middle LOD (traverse SVDAGs up to 4^3): %d < t <= %d", m_renderOptions.m_lodDistanceVoxel, m_renderOptions.m_lodDistanceOctree);
                    ImGui::Text("Coarsest LOD (only AABBs): %d < t", m_renderOptions.m_lodDistanceOctree);
                }
            }

            ImGui::Separator();

            if (ImGui::Checkbox("Spheres", &m_renderOptions.m_spheres)) {
                resetFrame(gpuContext);
            }
        }
        if (ImGui::CollapsingHeader("Environment")) {
            if (ImGui::Checkbox("Environment Map", &m_scene.m_sceneOptions.m_environmentMap)) {
                resetFrame(gpuContext);
            }
            if (ImGui::SliderFloat2("Environment Map Rotation", reinterpret_cast<float *>(&m_scene.m_sceneOptions.m_environmentMapRotation), 0.f, 1.f)) {
                resetFrame(gpuContext);
            }
            if (ImGui::SliderFloat3("Sky Color", reinterpret_cast<float *>(&m_scene.m_sceneOptions.m_skyColor), 0.f, 1.f)) {
                resetFrame(gpuContext);
            }
        }
        bool updateMaterial = false;
        bool updateLight = false;
        uint32_t numLights = 0;
        bool updateTLAS = false;
        m_scene.guiScene(gpuContext, [&] {
            if (m_renderOptions.m_labelMetadata) {
                if (ImGui::Checkbox("Label Metadata Jitter Albedo", &m_renderOptions.m_labelMetadataJitterAlbedo)) {
                    resetFrame(gpuContext);
                }
            } }, &updateMaterial, &updateLight, &numLights, &updateTLAS);
        if (updateMaterial) {
            passesSetMaterialBuffer();
            resetFrame(gpuContext);
        }
        // if (updateLight) {
        //     passesSetLightBuffer();
        //     m_renderOptions.m_numLights = numLights;
        //     resetFrame(gpuContext);
        // }
        if (updateTLAS) {
            m_passDebug->setAccelerationStructure(0, 0, m_scene.m_tlas->getAccelerationStructure());
            m_segmentationVolumesPassPathtrace->setAccelerationStructure(0, 0, m_scene.m_tlas->getAccelerationStructure());
            m_passDebug->setStorageBuffer(0, 10, m_scene.m_objectDescriptorBuffer.get());
            m_segmentationVolumesPassPathtrace->setStorageBuffer(0, 10, m_scene.m_objectDescriptorBuffer.get());
            if (!m_settings.m_rayquery) {
                m_segmentationVolumesPassPathtrace->setStorageBuffer(1, 10, m_scene.m_objectDescriptorBuffer.get());
            }
            resetFrame(gpuContext);
        }
        if (ImGui::CollapsingHeader("Camera")) {
            if (ImGui::Button("B-Spline")) {
                m_bSplineU = 0.f;
            }
            ImGui::InputFloat("B-Spline Speed", &m_bSplineSpeed);
            ImGui::Text("%f", m_bSplineU);
            if (ImGui::Button("Add Control Point")) {
                m_bSplinePosition.m_controlPoints.push_back(camera->getPosition());
                m_bSplineOrientation.m_controlPoints.push_back(camera->getSphericalRotation());
            }

            static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;
            if (ImGui::BeginTable("table1", 7, flags)) {
                ImGui::TableSetupColumn("Index");
                ImGui::TableSetupColumn("Position");
                ImGui::TableSetupColumn("Orientation");
                ImGui::TableSetupColumn("Teleport");
                ImGui::TableSetupColumn("Up");
                ImGui::TableSetupColumn("Down");
                ImGui::TableSetupColumn("Delete");
                ImGui::TableHeadersRow();
                for (int row = 0; row < m_bSplinePosition.m_controlPoints.size(); row++) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", row);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%f,%f,%f", m_bSplinePosition.m_controlPoints[row].x, m_bSplinePosition.m_controlPoints[row].y, m_bSplinePosition.m_controlPoints[row].z);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%f,%f,%f", m_bSplineOrientation.m_controlPoints[row].x, m_bSplineOrientation.m_controlPoints[row].y, m_bSplineOrientation.m_controlPoints[row].z);
                    ImGui::TableSetColumnIndex(3);
                    std::string abc = "controlpoint" + std::to_string(row);
                    ImGui::PushID(abc.c_str());
                    if (ImGui::SmallButton("##teleport")) {
                        gpuContext->getCamera()->fromSphericalCoordinates(m_bSplinePosition.m_controlPoints[row], m_bSplineOrientation.m_controlPoints[row].x, m_bSplineOrientation.m_controlPoints[row].y, m_bSplineOrientation.m_controlPoints[row].z);
                        resetFrame(gpuContext);
                    }
                    ImGui::TableSetColumnIndex(4);
                    if (ImGui::SmallButton("##up")) {
                        if (row > 0) {
                            std::swap(m_bSplinePosition.m_controlPoints[row], m_bSplinePosition.m_controlPoints[row - 1]);
                            std::swap(m_bSplineOrientation.m_controlPoints[row], m_bSplineOrientation.m_controlPoints[row - 1]);
                        }
                    }
                    ImGui::TableSetColumnIndex(5);
                    if (ImGui::SmallButton("##down")) {
                        if (row < m_bSplinePosition.m_controlPoints.size() - 1) {
                            std::swap(m_bSplinePosition.m_controlPoints[row], m_bSplinePosition.m_controlPoints[row + 1]);
                            std::swap(m_bSplineOrientation.m_controlPoints[row], m_bSplineOrientation.m_controlPoints[row + 1]);
                        }
                    }
                    ImGui::TableSetColumnIndex(6);
                    if (ImGui::SmallButton("##delete")) {
                        m_bSplinePosition.m_controlPoints.erase(m_bSplinePosition.m_controlPoints.begin() + row);
                        m_bSplineOrientation.m_controlPoints.erase(m_bSplineOrientation.m_controlPoints.begin() + row);
                    }
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
            if (ImGui::Button("Fig. Datastructure")) {
                gpuContext->getCamera()->fromSphericalCoordinates({433, 838, 380}, 0, 3.575f, 1.906f);
                resetFrame(gpuContext);
            }
            if (ImGui::Button("Fig. Emission")) {
                gpuContext->getCamera()->fromSphericalCoordinates({137, 421, 200}, 0, 4.063f, 1.866f);
                resetFrame(gpuContext);
            }
            if (ImGui::Button("Fig. Metallic")) {
                gpuContext->getCamera()->fromSphericalCoordinates({395, 799, 378}, 0, 2.387f, 1.888f);
                resetFrame(gpuContext);
            }
            if (ImGui::Button("Fig. Plastic")) {
                gpuContext->getCamera()->fromSphericalCoordinates({355, 817, 357}, 0, 3.13f, 1.974f);
                resetFrame(gpuContext);
            }
        }
        if (ImGui::CollapsingHeader("Utility")) {
            if (ImGui::Button("Screenshot")) {
                const auto t = std::time(nullptr);
                const auto tm = *std::localtime(&t);
                std::ostringstream oss;
                oss << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S");
                const std::string path = "screenshot-" + oss.str() + ".png";
                screenshot(gpuContext, m_imageFramebuffer, path);
            }
            if (ImGui::Button("1920x1080")) {
                gpuContext->resizeWindow(1920, 1080);
                resetFrame(gpuContext);
            }
            if (ImGui::Button("1080x1920")) {
                gpuContext->resizeWindow(1080, 1920);
                resetFrame(gpuContext);
            }
            if (ImGui::Button("1280x720")) {
                gpuContext->resizeWindow(1280, 720);
                resetFrame(gpuContext);
            }
            if (ImGui::Button("1080x1080")) {
                gpuContext->resizeWindow(1080, 1080);
                resetFrame(gpuContext);
            }
        }
        if (ImGui::CollapsingHeader("Timings", ImGuiTreeNodeFlags_DefaultOpen)) {
            {
                if (m_renderOptions.m_renderer == RENDERER_DEBUG) {
                    ImGui::Text("PassDebug                                  %.3f", m_passDebug->getTimeAveraged());
                    ImGui::Separator();
                }
            }
            if (m_renderOptions.m_renderer == RENDERER_SEGMENTATIONVOLUMES) {
                ImGui::Text("SegmentationVolumesPassPathtrace           %.3f", m_segmentationVolumesPassPathtrace->getTimeAveraged());
                ImGui::Separator();
            }
            {
                ImGui::Text("PassTonemapper                             %.3f", m_passTonemapper->getTimeAveraged());
            }
        }

        ImGui::End();
    }

    void SegmentationVolumes::update(GPUContext *gpuContext, Camera *camera, const float deltaTime) {
        if (m_bSplineU < FLT_MAX && m_bSplineU + deltaTime * m_bSplineSpeed < m_bSplinePosition.getMaxU()) {
            m_bSplineU += deltaTime * m_bSplineSpeed;
            camera->moveCenter(m_bSplinePosition.evaluate(m_bSplineU));
            const auto orientation = m_bSplineOrientation.evaluate(m_bSplineU);
            camera->setRotation(0.f, orientation.y, orientation.z);
            resetFrame(gpuContext);
        } else {
            m_bSplineU = FLT_MAX;
        }
    }

    void SegmentationVolumes::render(GPUContext *gpuContext, Camera *camera, const uint32_t activeIndex, RendererResult *rendererResult) {
        if (m_renderOptions.m_frame >= m_renderOptions.m_maxFrames) {
            rendererResult->m_image = m_imageFramebuffer.get();
            return;
        }

        const glm::mat4 view_to_clip_space = camera->getViewToClipSpace();
        const glm::mat4 clip_to_view_space = glm::inverse(view_to_clip_space);
        const glm::mat4 world_to_view_space = camera->getWorldToViewSpace();
        const glm::mat4 view_to_world_space = glm::inverse(world_to_view_space);

        const glm::vec3 rayOrigin = camera->getPosition();
        const glm::vec3 rayLeftBottom = Camera::ndcToWorldSpace(-1.f, -1.f, clip_to_view_space, view_to_world_space);
        const glm::vec3 rayLeftTop = Camera::ndcToWorldSpace(-1.f, 1.f, clip_to_view_space, view_to_world_space);
        const glm::vec3 rayRightBottom = Camera::ndcToWorldSpace(1.f, -1.f, clip_to_view_space, view_to_world_space);
        const glm::vec3 rayRightTop = Camera::ndcToWorldSpace(1.f, 1.f, clip_to_view_space, view_to_world_space);

        const bool executeDebug = m_renderOptions.m_renderer == RENDERER_DEBUG;
        const bool executeSegmentationVolumesPass = !executeDebug && m_renderOptions.m_renderer == RENDERER_SEGMENTATIONVOLUMES;

        // pre processing passes
        // 0 <= rng offset < 10
        {
            if (executeDebug) {
                m_preProcessingPass->execute(
                        [&] {
                        },
                        [&] {
                            m_uniformPassDebug->setVariable<glm::mat4>("g_view_to_clip_space", view_to_clip_space);
                            m_uniformPassDebug->setVariable<glm::mat4>("g_world_to_view_space", world_to_view_space);
                            m_uniformPassDebug->setVariable<uint32_t>("g_debug_mode", m_renderOptions.m_debug);
                            m_uniformPassDebug->setVariable<uint32_t>("g_num_lights", m_renderOptions.m_numLights);
                            m_uniformPassDebug->setVariable<uint32_t>("g_environment_map", m_scene.m_sceneOptions.m_environmentMap);
                            m_uniformPassDebug->setVariable<glm::vec2>("g_environment_map_rotation", m_scene.m_sceneOptions.m_environmentMapRotation);
                            m_uniformPassDebug->setVariable<glm::vec3>("g_sky_color", m_scene.m_sceneOptions.m_skyColor);
                            m_uniformPassDebug->setVariable<glm::mat4>("g_vp", m_renderOptions.m_vp);
                            m_uniformPassDebug->setVariable<glm::mat4>("g_vp_prev", m_renderOptions.m_vp_prev);
                            m_uniformPassDebug->setVariable<uint32_t>("g_lod", m_renderOptions.m_lod ? 1 : 0);
                            m_uniformPassDebug->setVariable<uint32_t>("g_lodDisableOnFirstFrame", m_renderOptions.m_lodDisableOnFirstFrame ? 1 : 0);
                            m_uniformPassDebug->setVariable<uint32_t>("g_lodDistance", m_renderOptions.m_lodDistance ? 1 : 0);
                            m_uniformPassDebug->setVariable<uint32_t>("g_lodDistanceVoxel", m_renderOptions.m_lodDistanceVoxel);
                            m_uniformPassDebug->setVariable<uint32_t>("g_lodDistanceOctree", m_renderOptions.m_lodDistanceOctree);
                            m_uniformPassDebug->setVariable<uint32_t>("g_num_instances", m_scene.m_instanceCount);
                            m_uniformPassDebug->setVariable<uint32_t>("g_label_metadata", m_renderOptions.m_labelMetadata ? 1 : 0);
                            m_uniformPassDebug->setVariable<uint32_t>("g_label_metadata_jitter_albedo", m_renderOptions.m_labelMetadataJitterAlbedo ? 1 : 0);
                            m_uniformPassDebug->setVariable<uint32_t>("g_spheres", m_renderOptions.m_spheres ? 1 : 0);
                            m_uniformPassDebug->upload(activeIndex);
                        },
                        [&](vk::CommandBuffer &commandBuffer) {
                            // execute shaders
                            {
                                m_passDebug->m_pushConstants.g_pixels = m_renderOptions.m_pixels;
                                m_passDebug->m_pushConstants.g_ray_origin = rayOrigin;
                                m_passDebug->m_pushConstants.g_ray_left_bottom = rayLeftBottom;
                                m_passDebug->m_pushConstants.g_ray_left_top = rayLeftTop;
                                m_passDebug->m_pushConstants.g_ray_right_bottom = rayRightBottom;
                                m_passDebug->m_pushConstants.g_ray_right_top = rayRightTop;
                                m_passDebug->m_pushConstants.g_frame = m_renderOptions.m_frame;
                                m_passDebug->m_pushConstants.g_rng_init = m_renderOptions.m_time;
                                m_passDebug->m_pushConstants.g_rng_init_offset = 0;

                                m_passDebug->execute(
                                        [&](const vk::CommandBuffer &cbf, const vk::PipelineLayout &pipelineLayout) {
                                            cbf.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(PassDebug::PushConstants), &m_passDebug->m_pushConstants);
                                        },
                                        [](const vk::CommandBuffer &) {
                                        },
                                        commandBuffer);
                            }
                        },
                        0, nullptr, nullptr, nullptr);
            }
        }

        // segmentation volumes passes
        // 10 <= rng offset < 20
        if (executeSegmentationVolumesPass) {
            const vk::Semaphore waitBeforeSegmentationVolumesimelineSemaphore = m_preProcessingPass->getTimelineSemaphore();
            const uint64_t waitBeforeSegmentationVolumesTimelineSemaphoreValue = m_preProcessingPass->getTimelineSemaphoreValue();
            vk::PipelineStageFlags waitBeforeSegmentationVolumesDstStageMask = vk::PipelineStageFlagBits::eComputeShader;

            if (m_renderOptions.m_renderer == RENDERER_SEGMENTATIONVOLUMES) {
                m_SegmentationVolumesPass->execute(
                        [&] {

                        },
                        [&] {
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<glm::mat4>("g_view_to_clip_space", view_to_clip_space);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<glm::mat4>("g_world_to_view_space", world_to_view_space);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<uint32_t>("g_num_lights", m_renderOptions.m_numLights);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<uint32_t>("g_environment_map", m_scene.m_sceneOptions.m_environmentMap);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<glm::vec2>("g_environment_map_rotation", m_scene.m_sceneOptions.m_environmentMapRotation);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<glm::vec3>("g_sky_color", m_scene.m_sceneOptions.m_skyColor);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<glm::mat4>("g_vp", m_renderOptions.m_vp);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<glm::mat4>("g_vp_prev", m_renderOptions.m_vp_prev);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<uint32_t>("g_spp", m_renderOptions.m_spp);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<uint32_t>("g_lod", m_renderOptions.m_lod ? 1 : 0);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<uint32_t>("g_lodDisableOnFirstFrame", m_renderOptions.m_lodDisableOnFirstFrame ? 1 : 0);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<uint32_t>("g_lodDistance", m_renderOptions.m_lodDistance ? 1 : 0);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<uint32_t>("g_lodDistanceVoxel", m_renderOptions.m_lodDistanceVoxel);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<uint32_t>("g_lodDistanceOctree", m_renderOptions.m_lodDistanceOctree);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<uint32_t>("g_bounces", m_renderOptions.m_bounces + 1);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<uint32_t>("g_num_instances", m_scene.m_instanceCount);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<uint32_t>("g_label_metadata", m_renderOptions.m_labelMetadata ? 1 : 0);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<uint32_t>("g_label_metadata_jitter_albedo", m_renderOptions.m_labelMetadataJitterAlbedo ? 1 : 0);
                            m_uniformSegmentationVolumesPassPathtrace->setVariable<uint32_t>("g_spheres", m_renderOptions.m_spheres ? 1 : 0);
                            m_uniformSegmentationVolumesPassPathtrace->upload(activeIndex);

                            if (!m_settings.m_rayquery) {
                                m_uniformSegmentationVolumesPassPathtraceIntersection->setVariable<float>("g_ray_origin_x", rayOrigin.x);
                                m_uniformSegmentationVolumesPassPathtraceIntersection->setVariable<float>("g_ray_origin_y", rayOrigin.y);
                                m_uniformSegmentationVolumesPassPathtraceIntersection->setVariable<float>("g_ray_origin_z", rayOrigin.z);
                                m_uniformSegmentationVolumesPassPathtraceIntersection->setVariable<uint32_t>("g_lod", m_renderOptions.m_lod ? 1 : 0);
                                m_uniformSegmentationVolumesPassPathtraceIntersection->setVariable<uint32_t>("g_lodDisableOnFirstFrame", m_renderOptions.m_lodDisableOnFirstFrame ? 1 : 0);
                                m_uniformSegmentationVolumesPassPathtraceIntersection->setVariable<uint32_t>("g_lodDistance", m_renderOptions.m_lodDistance ? 1 : 0);
                                m_uniformSegmentationVolumesPassPathtraceIntersection->setVariable<uint32_t>("g_lodDistanceVoxel", m_renderOptions.m_lodDistanceVoxel);
                                m_uniformSegmentationVolumesPassPathtraceIntersection->setVariable<uint32_t>("g_lodDistanceOctree", m_renderOptions.m_lodDistanceOctree);
                                m_uniformSegmentationVolumesPassPathtraceIntersection->setVariable<uint32_t>("g_frame", m_renderOptions.m_frame);
                                m_uniformSegmentationVolumesPassPathtraceIntersection->setVariable<uint32_t>("g_spheres", m_renderOptions.m_spheres ? 1 : 0);
                                m_uniformSegmentationVolumesPassPathtraceIntersection->upload(activeIndex);
                            }
                        },
                        [&](vk::CommandBuffer &commandBuffer) {
                            // execute shaders
                            {
                                m_segmentationVolumesPassPathtracePushConstants->m_pushConstants.g_pixels = m_renderOptions.m_pixels;
                                m_segmentationVolumesPassPathtracePushConstants->m_pushConstants.g_ray_origin = rayOrigin;
                                m_segmentationVolumesPassPathtracePushConstants->m_pushConstants.g_ray_left_bottom = rayLeftBottom;
                                m_segmentationVolumesPassPathtracePushConstants->m_pushConstants.g_ray_left_top = rayLeftTop;
                                m_segmentationVolumesPassPathtracePushConstants->m_pushConstants.g_ray_right_bottom = rayRightBottom;
                                m_segmentationVolumesPassPathtracePushConstants->m_pushConstants.g_ray_right_top = rayRightTop;
                                m_segmentationVolumesPassPathtracePushConstants->m_pushConstants.g_frame = m_renderOptions.m_frame;
                                m_segmentationVolumesPassPathtracePushConstants->m_pushConstants.g_rng_init = m_renderOptions.m_time;
                                m_segmentationVolumesPassPathtracePushConstants->m_pushConstants.g_rng_init_offset = 10;

                                m_segmentationVolumesPassPathtrace->execute(
                                        [&](const vk::CommandBuffer &cbf, const vk::PipelineLayout &pipelineLayout) {
                                            cbf.pushConstants(pipelineLayout, m_settings.m_rayquery ? vk::ShaderStageFlagBits::eCompute : vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(SegmentationVolumesPassPathtrace::PushConstants), &m_segmentationVolumesPassPathtracePushConstants->m_pushConstants);
                                        },
                                        [](const vk::CommandBuffer &cbf) {
                                            constexpr vk::MemoryBarrier memoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
                                            cbf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags{}, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
                                        },
                                        commandBuffer);
                            }
                        },
                        1, &waitBeforeSegmentationVolumesimelineSemaphore, &waitBeforeSegmentationVolumesTimelineSemaphoreValue, &waitBeforeSegmentationVolumesDstStageMask);
            }
        }

        // post processing passes
        {
            vk::Semaphore waitBeforePostProcessingTimelineSemaphore;
            uint64_t waitBeforePostProcessingTimelineSemaphoreValue;
            vk::PipelineStageFlags waitBeforePostProcessingDstStageMask = vk::PipelineStageFlagBits::eComputeShader;
            if (executeSegmentationVolumesPass) {
                waitBeforePostProcessingTimelineSemaphore = m_SegmentationVolumesPass->getTimelineSemaphore();
                waitBeforePostProcessingTimelineSemaphoreValue = m_SegmentationVolumesPass->getTimelineSemaphoreValue();
            } else {
                waitBeforePostProcessingTimelineSemaphore = m_preProcessingPass->getTimelineSemaphore();
                waitBeforePostProcessingTimelineSemaphoreValue = m_preProcessingPass->getTimelineSemaphoreValue();
            }

            m_postProcessingPass->execute(
                    [] {
                    },
                    [&] {
                    },
                    [&](vk::CommandBuffer &commandBuffer) {
#ifdef OPTIX_SUPPORT
                        if (executeDenoiser) {
                            m_denoiser->bufferToImage(commandBuffer, m_imageFramebuffer);
                        }
#endif

                        {
                            m_passTonemapper->m_pushConstants.g_pixels = m_renderOptions.m_pixels;
                            m_passTonemapper->m_pushConstants.g_tonemapper = m_renderOptions.m_tonemapper;

                            m_passTonemapper->execute(
                                    [&](const vk::CommandBuffer &cbf, const vk::PipelineLayout &pipelineLayout) {
                                        cbf.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(PassTonemapper::PushConstants), &m_passTonemapper->m_pushConstants);
                                    },
                                    [](const vk::CommandBuffer &cbf) {
                                    },
                                    commandBuffer);
                        }
                    },
                    1, &waitBeforePostProcessingTimelineSemaphore, &waitBeforePostProcessingTimelineSemaphoreValue, &waitBeforePostProcessingDstStageMask);

            rendererResult->m_waitTimelineSemaphore = m_postProcessingPass->getTimelineSemaphore();
            rendererResult->m_waitTimelineSemaphoreValue = m_postProcessingPass->getTimelineSemaphoreValue();
        }

        m_renderOptions.m_time++;
        m_renderOptions.m_vp_prev = m_renderOptions.m_vp;
        m_renderOptions.m_rayOrigin = rayOrigin;
        m_renderOptions.m_rayLeftBottom = rayLeftBottom;
        m_renderOptions.m_rayLeftTop = rayLeftTop;
        m_renderOptions.m_rayRightBottom = rayRightBottom;
        m_renderOptions.m_rayRightTop = rayRightTop;
        m_renderOptions.m_frame++;
        m_renderOptions.m_framePreviousValid = true;
        m_renderOptions.m_validActiveIndex = activeIndex;

        rendererResult->m_image = m_imageFramebuffer.get();
    }

    void SegmentationVolumes::releaseResources() {
        m_scene.release();
    }

    void SegmentationVolumes::releaseShaderResources() {
        RAVEN_PASS_RELEASE(m_preProcessingPass);
        RAVEN_PASS_RELEASE(m_SegmentationVolumesPass);
        RAVEN_PASS_RELEASE(m_postProcessingPass);

        RAVEN_PASS_SHADER_RELEASE(m_passDebug);

        RAVEN_PASS_SHADER_RELEASE(m_segmentationVolumesPassPathtrace);

        RAVEN_PASS_SHADER_RELEASE(m_passTonemapper);
    }

    void SegmentationVolumes::releaseSwapchainResources() {
        RAVEN_IMAGE_RELEASE(m_imageFramebuffer);
        RAVEN_IMAGE_RELEASE(m_imageAccumulationBuffer);
    }

    void SegmentationVolumes::resetFrame(GPUContext *gpuContext) {
        m_renderOptions.m_frame = 0;
        m_renderOptions.m_framePreviousValid = false;
        m_renderOptions.m_time = 0;

        const glm::mat4 view_to_clip_space = gpuContext->getCamera()->getViewToClipSpace();
        const glm::mat4 world_to_view_space = gpuContext->getCamera()->getWorldToViewSpace();
        const glm::mat4 vp = view_to_clip_space * world_to_view_space;
        m_renderOptions.m_vp_prev = vp;
        m_renderOptions.m_vp = vp;

        m_passDebug->invalidateTime();

        m_segmentationVolumesPassPathtrace->invalidateTime();

        m_passTonemapper->invalidateTime();
    }

    void SegmentationVolumes::screenshot(GPUContext *gpuContext, std::shared_ptr<Image> &image, const std::string &path) const {
        gpuContext->m_device.waitIdle();

        if (m_renderOptions.m_tonemapper >= 0) {
            // create target image
            auto settings = Image::ImageSettings{.m_width = image->getWidth(), .m_height = image->getHeight(), .m_format = vk::Format::eR8G8B8A8Unorm, .m_tiling = vk::ImageTiling::eOptimal, .m_usageFlags = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage, .m_name = "screenshotBuffer"};
            auto screenshotImage = std::make_shared<Image>(gpuContext, settings);
            screenshotImage->create();

            const auto priorLayout = image->getLayout();

            gpuContext->executeCommands([image, screenshotImage, priorLayout](vk::CommandBuffer commandBuffer) {
                {
                    vk::ImageMemoryBarrier barrier{};
                    barrier.oldLayout = priorLayout;
                    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = image->getImage();
                    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                    barrier.srcAccessMask = vk::AccessFlagBits::eNone;
                    barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
                    auto sourceStage = vk::PipelineStageFlagBits::eHost;
                    auto destinationStage = vk::PipelineStageFlagBits::eTransfer;
                    commandBuffer.pipelineBarrier(
                            sourceStage, destinationStage,
                            {},
                            nullptr, nullptr, {barrier});
                }
                {
                    vk::ImageMemoryBarrier barrier{};
                    barrier.oldLayout = screenshotImage->getLayout();
                    barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = screenshotImage->getImage();
                    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                    barrier.srcAccessMask = vk::AccessFlagBits::eNone;
                    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
                    auto sourceStage = vk::PipelineStageFlagBits::eHost;
                    auto destinationStage = vk::PipelineStageFlagBits::eTransfer;
                    commandBuffer.pipelineBarrier(
                            sourceStage, destinationStage,
                            {},
                            nullptr, nullptr, {barrier});
                }

                {
                    vk::ImageBlit copyRegion{};
                    copyRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                    copyRegion.srcSubresource.layerCount = 1;
                    copyRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                    copyRegion.dstSubresource.layerCount = 1;
                    copyRegion.srcOffsets[0] = vk::Offset3D{0, 0, 0};
                    copyRegion.srcOffsets[1] = vk::Offset3D{static_cast<int32_t>(image->getWidth()), static_cast<int32_t>(image->getHeight()), 1};
                    copyRegion.dstOffsets[0] = vk::Offset3D{0, 0, 0};
                    copyRegion.dstOffsets[1] = vk::Offset3D{static_cast<int32_t>(image->getWidth()), static_cast<int32_t>(image->getHeight()), 1};
                    auto filter = vk::Filter::eNearest;
                    commandBuffer.blitImage(image->getImage(), vk::ImageLayout::eTransferSrcOptimal, screenshotImage->getImage(), vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion, filter);
                }

                {
                    vk::ImageMemoryBarrier barrier{};
                    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
                    barrier.newLayout = vk::ImageLayout::eGeneral;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = screenshotImage->getImage();
                    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                    barrier.dstAccessMask = vk::AccessFlagBits::eNone;
                    auto sourceStage = vk::PipelineStageFlagBits::eTransfer;
                    auto destinationStage = vk::PipelineStageFlagBits::eHost;
                    commandBuffer.pipelineBarrier(
                            sourceStage, destinationStage,
                            {},
                            nullptr, nullptr, {barrier});
                    screenshotImage->setLayout(vk::ImageLayout::eGeneral);
                }
                {
                    vk::ImageMemoryBarrier barrier{};
                    barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
                    barrier.newLayout = priorLayout;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = image->getImage();
                    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                    barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
                    barrier.dstAccessMask = vk::AccessFlagBits::eNone;
                    auto sourceStage = vk::PipelineStageFlagBits::eTransfer;
                    auto destinationStage = vk::PipelineStageFlagBits::eHost;
                    commandBuffer.pipelineBarrier(
                            sourceStage, destinationStage,
                            {},
                            nullptr, nullptr, {barrier});
                }
            });

            //            constexpr vk::ImageSubresource subResource{vk::ImageAspectFlagBits::eColor, 0, 0};
            //            vk::SubresourceLayout subResourceLayout;
            //            gpuContext->m_device.getImageSubresourceLayout(screenshotImage->getImage(), &subResource, &subResourceLayout);

            std::vector<uint8_t> bytes(screenshotImage->getSizeBytes());
            screenshotImage->download(bytes.data());

            const uint8_t *data = bytes.data();
            //            data += subResourceLayout.offset;

            stbi_write_png(path.c_str(), static_cast<int>(image->getWidth()), static_cast<int>(image->getHeight()), 4, data, static_cast<int>(image->getWidth()) * 4);
        } else {
            std::vector<float> pixels(image->getSizeBytes());
            image->download(pixels.data());
            // ImagePFM::writeFilePFM(pixels, ImagePFM::COLOR, static_cast<int>(image->getWidth()), static_cast<int>(image->getHeight()), path + ".pfm");
            const std::string hdr = path + ".hdr";
            stbi_write_hdr(hdr.c_str(), static_cast<int>(image->getWidth()), static_cast<int>(image->getHeight()), 4, pixels.data());
        }

        gpuContext->m_device.waitIdle();
    }

    void SegmentationVolumes::passesSetMaterialBuffer() const {
        m_passDebug->setStorageBuffer(0, 11, m_scene.m_materialBuffer.get());

        m_segmentationVolumesPassPathtrace->setStorageBuffer(0, 11, m_scene.m_materialBuffer.get());
    }

    // void SegmentationVolumes::passesSetLightBuffer() const {
    //     m_passDebug->setStorageBuffer(0, 11, m_scene.m_lightBuffer.get());
    //
    //     m_segmentationVolumesPassPathtrace->setStorageBuffer(0, 12, m_scene.m_lightBuffer.get());
    // }
} // namespace raven