#pragma once

#include "../../../VkRaven/raven/include/raven/passes/PassShaderCompute.h"
#include "../../../VkRaven/raven/include/raven/util/Paths.h"

namespace raven {
    class PassTonemapper final : public PassShaderCompute {
    public:
        explicit PassTonemapper(GPUContext *gpuContext) : PassShaderCompute(gpuContext, "PassTonemapper") {
        }

        struct PushConstants {
            glm::ivec2 g_pixels;

            uint32_t g_tonemapper;
        };

        PushConstants m_pushConstants = {};

    protected:
        std::vector<std::shared_ptr<Shader>> createShaders() override {
            constexpr auto workGroupSize = vk::Extent3D{8, 8, 1};
            auto specializationMapEntries = std::vector{vk::SpecializationMapEntry{0, 0, sizeof(uint32_t)},
                                                        vk::SpecializationMapEntry{1, 4, sizeof(uint32_t)},
                                                        vk::SpecializationMapEntry{2, 8, sizeof(uint32_t)}};
            auto specializationData = std::vector{workGroupSize.width, workGroupSize.height, workGroupSize.depth};
            return {std::make_shared<Shader>(m_gpuContext, Paths::m_resourceDirectoryPath + "/shaders/main/passes", "pass_tonemapper.comp", vk::ShaderStageFlagBits::eCompute, workGroupSize, specializationMapEntries, specializationData)};
        }

        uint32_t getPushConstantRangeSize() override {
            return sizeof(PushConstants);
        }
    };
} // namespace raven