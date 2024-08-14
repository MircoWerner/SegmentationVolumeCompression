#pragma once

#include "SegmentationVolumesPassPathtraceRayTracing.h"
#include "raven/core/Image.h"
#include "raven/core/Renderer.h"
#include "raven/core/Uniform.h"
#include "raven/passes/PassCompute.h"

#include "PassTonemapper.h"
#include "raven/util/animation/BSpline.h"
#include "scene/SegmentationVolumesScene.h"
#include "segmentationvolumes/PassDebug.h"
#include "segmentationvolumes/SegmentationVolumesPassPathtraceRayQuery.h"

namespace raven {
    class SegmentationVolumes final : public Renderer {
    public:
#define RENDERER_SEGMENTATIONVOLUMES 0
#define RENDERER_DEBUG 1

        struct SegmentationVolumesSettings {
            std::string m_data;
            std::string m_scene;
            bool m_rayquery;
        };

        explicit SegmentationVolumes(SegmentationVolumesSettings settings);

        void initResources(GPUContext *gpuContext) override;
        void initShaderResources(GPUContext *gpuContext) override;
        void initSwapchainResources(GPUContext *gpuContext, vk::Extent2D extent) override;
        void recordImGui(GPUContext *gpuContext, Camera *camera) override;
        void update(GPUContext *gpuContext, Camera *camera, float deltaTime) override;
        void render(GPUContext *gpuContext, Camera *camera, uint32_t activeIndex, RendererResult *rendererResult) override;
        void releaseResources() override;
        void releaseShaderResources() override;
        void releaseSwapchainResources() override;

        [[nodiscard]] float getPathtraceTime() const {
            return m_segmentationVolumesPassPathtrace->getTime();
        }

        [[nodiscard]] float getPathtraceTimeAveraged() const {
            return m_segmentationVolumesPassPathtrace->getTimeAveraged();
        }

        void resetFrame(GPUContext *gpuContext);

        [[nodiscard]] uint64_t getAABBBuffersSize() const {
            return m_scene.getAABBBuffersSize();
        }

        [[nodiscard]] uint64_t getLODBuffersSize() const {
            return m_scene.getLODBuffersSize();
        }

        [[nodiscard]] uint64_t getBLASBuffersSize() const {
            return m_scene.getBLASBuffersSize();
        }

        [[nodiscard]] uint64_t getTLASBufferSize() const {
            return m_scene.getTLASBufferSize();
        }

    private:
        SegmentationVolumesSettings m_settings;

        std::shared_ptr<Image> m_imageFramebuffer;
        std::shared_ptr<Image> m_imageAccumulationBuffer;

        std::shared_ptr<PassCompute> m_preProcessingPass;
        std::shared_ptr<PassDebug> m_passDebug;
        std::shared_ptr<Uniform> m_uniformPassDebug;

        std::shared_ptr<PassCompute> m_SegmentationVolumesPass;
        std::shared_ptr<PassShaderCompute> m_segmentationVolumesPassPathtrace;
        std::shared_ptr<SegmentationVolumesPassPathtrace> m_segmentationVolumesPassPathtracePushConstants;
        std::shared_ptr<Uniform> m_uniformSegmentationVolumesPassPathtrace;
        std::shared_ptr<Uniform> m_uniformSegmentationVolumesPassPathtraceIntersection;

        std::shared_ptr<PassCompute> m_postProcessingPass;
        std::shared_ptr<PassTonemapper> m_passTonemapper;

        struct RenderOptions {
            uint32_t m_time = 0; // total number of executions/frames since application start

            glm::ivec2 m_pixels;

            uint32_t m_frame = 0;
            uint32_t m_maxFrames = 1024;
            uint32_t m_validActiveIndex = 0;

            bool m_framePreviousValid = false;

            uint32_t m_tonemapper = 1;

            uint32_t m_renderer = RENDERER_SEGMENTATIONVOLUMES;

            uint32_t m_debug = 0; // for debug
            uint32_t m_spp = 1;   // for pathtrace

            glm::mat4 m_vp = glm::mat4(1.f);
            glm::mat4 m_vp_prev = glm::mat4(1.f);

            glm::vec3 m_rayOrigin;
            glm::vec3 m_rayLeftBottom;
            glm::vec3 m_rayLeftTop;
            glm::vec3 m_rayRightBottom;
            glm::vec3 m_rayRightTop;

            uint32_t m_numLights = 0;

            uint32_t m_lodType = 2; // #define LOD_TYPE_SVO 0 #define LOD_TYPE_SVDAG 1 #define LOD_TYPE_SVDAG_OCCUPANCY_FIELD 2

            bool m_lod = true;
            bool m_lodDisableOnFirstFrame = true;
            bool m_lodDistance = true;
            uint32_t m_lodDistanceVoxel = 512;   // finest LOD, traverse octrees and occupancy fields up to 1^3, i.e. individual voxels
            uint32_t m_lodDistanceOctree = 1024; // middle LOD, traverse octrees up to 4^3
            // uint32_t m_lodDistanceAABB = ...; // coarsest LOD, only AABBs

            uint32_t m_bounces = 32;

            bool m_labelMetadata = false;
            bool m_labelMetadataJitterAlbedo = false;

            bool m_spheres = false;
        };
        RenderOptions m_renderOptions{};

        SegmentationVolumesScene m_scene;

        void screenshot(GPUContext *gpuContext, std::shared_ptr<Image> &image, const std::string &path) const;

        void passesSetMaterialBuffer() const;
        // void passesSetLightBuffer() const;

        BSpline m_bSplinePosition;
        BSpline m_bSplineOrientation;
        float m_bSplineU = FLT_MAX;
        float m_bSplineSpeed = 1.f;
    };
} // namespace raven