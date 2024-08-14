#pragma once

#include "raven/passes/PassShaderCompute.h"
#include "raven/util/Paths.h"

namespace raven {
    class DAGGPUPassSort final : public PassShaderCompute {
    public:
        explicit DAGGPUPassSort(GPUContext *gpuContext, const uint32_t workGroupSizeX) : PassShaderCompute(gpuContext, "DAGGPUPassSort"), m_workGroupSizeX(workGroupSizeX) {
        }

        struct PushConstants {
            enum eAlgorithmVariant : uint32_t {
                eLocalBitonicMergeSortExample = 0,
                eLocalDisperse = 1,
                eBigFlip = 2,
                eBigDisperse = 3,
            };

            uint32_t numElements;
            uint32_t globalOffset;
            uint32_t h;
            uint32_t algorithm;
        };

        PushConstants m_pushConstants = {};

    protected:
        std::vector<std::shared_ptr<Shader>> createShaders() override {
            const auto workGroupSize = vk::Extent3D{m_workGroupSizeX, 1, 1};
            auto specializationMapEntries = std::vector{vk::SpecializationMapEntry{0, 0, sizeof(uint32_t)},
                                                        vk::SpecializationMapEntry{1, 4, sizeof(uint32_t)},
                                                        vk::SpecializationMapEntry{2, 8, sizeof(uint32_t)}};
            auto specializationData = std::vector{workGroupSize.width, workGroupSize.height, workGroupSize.depth};
            return {std::make_shared<Shader>(m_gpuContext, Paths::m_resourceDirectoryPath + "/shaders/builder", "dag_1_sort.comp", vk::ShaderStageFlagBits::eCompute, workGroupSize, specializationMapEntries, specializationData)};
        }

        uint32_t getPushConstantRangeSize() override {
            return sizeof(PushConstants);
        }

    private:
        uint32_t m_workGroupSizeX;
    };
} // namespace raven