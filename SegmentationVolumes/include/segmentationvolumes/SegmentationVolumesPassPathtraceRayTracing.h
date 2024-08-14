#pragma once

#include "SegmentationVolumesPassPathtrace.h"
#include "raven/passes/PassShaderRayTracing.h"
#include "raven/util/Paths.h"

namespace raven {
    class SegmentationVolumesPassPathtraceRayTracing final : public PassShaderRayTracing {
    public:
        explicit SegmentationVolumesPassPathtraceRayTracing(GPUContext *gpuContext, const uint32_t lodType) : PassShaderRayTracing(gpuContext, "SegmentationVolumesPassPathtraceRayTracing"), m_lodType(lodType) {
        }

    protected:
        uint32_t m_lodType;

        std::vector<std::shared_ptr<Shader>> createShaders() override {
            constexpr auto workGroupSize = vk::Extent3D{8, 8, 1};
            auto specializationMapEntries = std::vector{vk::SpecializationMapEntry{0, 0, sizeof(uint32_t)}};
            auto specializationData = std::vector{m_lodType};
            std::vector<vk::SpecializationMapEntry> specializationMapEntriesEmpty{};
            std::vector<uint32_t> specializationDataEmpty{};
            return {
                    std::make_shared<Shader>(m_gpuContext, Paths::m_resourceDirectoryPath + "/shaders/main/passes", "segmentationvolumes_pass_pathtrace.rgen", vk::ShaderStageFlagBits::eRaygenKHR, workGroupSize, specializationMapEntries, specializationData),
                    std::make_shared<Shader>(m_gpuContext, Paths::m_resourceDirectoryPath + "/shaders/main/passes", "segmentationvolumes_pass_pathtrace.rmiss", vk::ShaderStageFlagBits::eMissKHR, workGroupSize, specializationMapEntriesEmpty, specializationDataEmpty),
                    std::make_shared<Shader>(m_gpuContext, Paths::m_resourceDirectoryPath + "/shaders/main/passes", "segmentationvolumes_pass_pathtrace.rchit", vk::ShaderStageFlagBits::eClosestHitKHR, workGroupSize, specializationMapEntriesEmpty, specializationDataEmpty),
                    std::make_shared<Shader>(m_gpuContext, Paths::m_resourceDirectoryPath + "/shaders/main/passes", "segmentationvolumes_pass_pathtrace.rint", vk::ShaderStageFlagBits::eIntersectionKHR, workGroupSize, specializationMapEntries, specializationData)};
        }

        uint32_t getPushConstantRangeSize() override {
            return sizeof(SegmentationVolumesPassPathtrace::PushConstants);
        }

        RayGroups createRayGroups() override {
            return {
                    {{.rayGenShaderIndex = 0}},                                                // ray generation groups
                    {{.missShaderIndex = 1}},                                                  // miss groups
                    {{.hitShaderIndex = 2, .intersectionShaderIndex = 3, .type = eProcedural}} // hit groups
            };
        }
    };
} // namespace raven