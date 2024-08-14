#pragma once

#include "raven/passes/PassShaderCompute.h"

namespace raven {
    class SegmentationVolumesPassPathtrace {
    public:
        struct PushConstants {
            glm::ivec2 g_pixels;

            glm::vec3 g_ray_origin;
            glm::vec3 g_ray_left_bottom;
            glm::vec3 g_ray_left_top;
            glm::vec3 g_ray_right_bottom;
            glm::vec3 g_ray_right_top;

            uint32_t g_frame = 0;

            uint32_t g_rng_init;
            uint32_t g_rng_init_offset;
        };

        PushConstants m_pushConstants = {};
    };
} // namespace raven