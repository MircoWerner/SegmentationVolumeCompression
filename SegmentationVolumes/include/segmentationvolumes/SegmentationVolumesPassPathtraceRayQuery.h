#pragma once

#include "SegmentationVolumesPassPathtrace.h"
#include "raven/passes/PassShaderCompute.h"
#include "raven/util/Paths.h"

namespace raven {
    class SegmentationVolumesPassPathtraceRayQuery final : public PassShaderCompute {
    public:
        explicit SegmentationVolumesPassPathtraceRayQuery(GPUContext *gpuContext, const uint32_t lodType) : PassShaderCompute(gpuContext, "SegmentationVolumesPassPathtraceRayQuery"), m_lodType(lodType) {
        }

    protected:
        uint32_t m_lodType;

        std::vector<std::shared_ptr<Shader>> createShaders() override {
            constexpr auto workGroupSize = vk::Extent3D{8, 8, 1};
            auto specializationMapEntries = std::vector{vk::SpecializationMapEntry{0, 0, sizeof(uint32_t)},
                                                        vk::SpecializationMapEntry{1, 4, sizeof(uint32_t)},
                                                        vk::SpecializationMapEntry{2, 8, sizeof(uint32_t)},
                                                        vk::SpecializationMapEntry{3, 12, sizeof(uint32_t)}};
            auto specializationData = std::vector{workGroupSize.width, workGroupSize.height, workGroupSize.depth, m_lodType};
            return {std::make_shared<Shader>(m_gpuContext, Paths::m_resourceDirectoryPath + "/shaders/main/passes", "segmentationvolumes_pass_pathtrace.comp", vk::ShaderStageFlagBits::eCompute, workGroupSize, specializationMapEntries, specializationData)};
        }

        uint32_t getPushConstantRangeSize() override {
            return sizeof(SegmentationVolumesPassPathtrace::PushConstants);
        }
    };
} // namespace raven