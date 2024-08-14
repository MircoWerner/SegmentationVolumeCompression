#pragma once

#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace raven {
    struct ObjectDescriptor {
        uint64_t aabbAddress{}; // address to the buffer that contains all AABBs of the object, each object can have its own buffer, but a large shared buffer is possible as well
        uint64_t lodAddress{};  // address to the buffer that contains all LOD information of the section
    };

    struct VoxelAABB {
        int32_t minX{};
        int32_t minY{};
        int32_t minZ{};
        int32_t maxX{};
        int32_t maxY{};
        int32_t maxZ{};
        uint32_t labelId{};
        uint32_t lod{};

        [[nodiscard]] vk::AabbPositionsKHR toVkAABBPosition() const {
            return {static_cast<float>(minX), static_cast<float>(minY), static_cast<float>(minZ), static_cast<float>(maxX), static_cast<float>(maxY), static_cast<float>(maxZ)};
        }

        friend std::ostream &operator<<(std::ostream &stream, const VoxelAABB &aabb) {
            stream << "AABB{ min=(" << aabb.minX << "," << aabb.minY << "," << aabb.minZ << "), max=(" << aabb.maxX << "," << aabb.maxY << "," << aabb.maxZ << "), labelId=" << aabb.labelId << ", lod=" << aabb.lod << " }";
            return stream;
        }
    };

    struct SVDAG {
        uint32_t child0;
        uint32_t child1;
        uint32_t child2;
        uint32_t child3;
        uint32_t child4;
        uint32_t child5;
        uint32_t child6;
        uint32_t child7;

        [[nodiscard]] bool isLeaf() const {
            return child0 == 0xFFFFFFFF;
        }
    };
} // namespace raven