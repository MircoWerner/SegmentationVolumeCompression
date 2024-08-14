#pragma once

#include "raven/passes/PassShaderCompute.h"
#include "raven/util/Paths.h"

namespace raven {
    class DAGGPUPassCompaction final : public PassShaderCompute {
    public:
        explicit DAGGPUPassCompaction(GPUContext *gpuContext) : PassShaderCompute(gpuContext, "DAGGPUPassCompaction") {
        }

        struct PushConstants {
            uint32_t g_level;       // current level
            uint32_t g_level_index; // index of the first node of the current level
            uint32_t g_level_count; // number of nodes in the current level

            uint32_t g_global_offset;
        };

        PushConstants m_pushConstants = {};

    protected:
        std::vector<std::shared_ptr<Shader>> createShaders() override {
            constexpr auto workGroupSize = vk::Extent3D{256, 1, 1};
            auto specializationMapEntries = std::vector{vk::SpecializationMapEntry{0, 0, sizeof(uint32_t)},
                                                        vk::SpecializationMapEntry{1, 4, sizeof(uint32_t)},
                                                        vk::SpecializationMapEntry{2, 8, sizeof(uint32_t)}};
            auto specializationData = std::vector{workGroupSize.width, workGroupSize.height, workGroupSize.depth};
            return {std::make_shared<Shader>(m_gpuContext, Paths::m_resourceDirectoryPath + "/shaders/builder", "dag_4_compaction.comp", vk::ShaderStageFlagBits::eCompute, workGroupSize, specializationMapEntries, specializationData)};
        }

        uint32_t getPushConstantRangeSize() override {
            return sizeof(PushConstants);
        }
    };
} // namespace raven