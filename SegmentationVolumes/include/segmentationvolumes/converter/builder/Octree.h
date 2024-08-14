#pragma once

#include "glm/vec3.hpp"
#include "raven/util/AABB.h"

#include <algorithm>
#include <stdexcept>

namespace raven {
    /**
     * input: {octree_build_info_0, octree_build_info_1, ..., octree_build_info_n-1} (set of octree build infos)
     * output: [octree_nodes], [index_of_octree_0, ..., index_of_octree_n-1] (vector with all octree nodes and vector with indices into the octree nodes,
     *          the i-th index corresponds to the octree constructed from the i-th octree build info)
     */
    class Octree {
    public:
        typedef glm::ivec3 Voxel;

        struct OctreeBuildInfo {
            uint32_t labelId;
            iAABB aabb;
            std::vector<Voxel> voxels;
        };

        void buildOctrees(std::vector<OctreeBuildInfo> &octreeBuildInfos) {
            m_octreeIndices.resize(octreeBuildInfos.size());

            for (uint32_t i = 0; i < octreeBuildInfos.size(); i++) {
                buildOctree(octreeBuildInfos[i], i);
            }
        }

        /**
     * [ 3 bit extent_exponent | 1 bit solid | 12 bit child ]
     * - extent: the extent of the node, has to be a power of two such that we can store the exponent only, i.e. 2^(extent_exponent) = extent
     * - solid: 0 iff all voxels are empty in this octree cell
     * - child: pointer to the first child, the eight children pointer are [ child | child + 1 | ... | child + 7 ], we reserve 11...111 = 0x0FFF to mark an invalid pointer
     */
        typedef uint16_t OctreeNode; // GPU mirror

#define OCTREE_NODE_EXTENT_MAX 16 // 128 // we can store 3 bits exponent -> 2^0..2^7 -> 1..128 extent

#define OCTREE_NODE_EXTENT_BITS UINT16_C(0xE000)
#define OCTREE_NODE_SOLID_BITS UINT16_C(0x1000)
#define OCTREE_NODE_CHILD_BITS UINT16_C(0x0FFF)

#define OCTREE_NODE_EXTENT_SHIFT 13
#define OCTREE_NODE_SOLID_SHIFT 12

#define OCTREE_NODE_INVALID_CHILD UINT16_C(0x0FFF)

#define OCTREE_NODE_EXTENT(A) (1 << (((A) & OCTREE_NODE_EXTENT_BITS) >> OCTREE_NODE_EXTENT_SHIFT))
#define OCTREE_NODE_SOLID(A) (((A) & OCTREE_NODE_SOLID_BITS) >> OCTREE_NODE_SOLID_SHIFT)
#define OCTREE_NODE_CHILD(A) ((A) & OCTREE_NODE_CHILD_BITS)

        std::vector<OctreeNode> m_octrees = {}; // GPU buffer
        std::vector<uint32_t> m_octreeIndices = {};

    private:
        void buildOctree(OctreeBuildInfo &octreeBuildInfo, const uint32_t octreeBuildInfoIndex) {
            std::vector<OctreeNode> octree;
            octree.emplace_back();

            // const uint32_t maxExtent = octreeBuildInfo.aabb.maxExtent();
            // const uint32_t extent = nextPowerOfTwo(maxExtent);
            constexpr uint32_t extent = 16; // enforce 16^3, the traversal implicitly assumes that every AABB contains a 16^3 octree/DAG

            buildCellOctree(octree, 0, octreeBuildInfo.aabb.m_min, extent, 0, octreeBuildInfo.voxels.size(), octreeBuildInfo.voxels);

            m_octreeIndices[octreeBuildInfoIndex] = m_octrees.size();
            m_octrees.insert(m_octrees.end(), octree.begin(), octree.end());
        }

        static void buildCellOctree(std::vector<OctreeNode> &octree, const uint32_t octreeNodeIdx, const glm::ivec3 anchor, const uint16_t extent, const uint32_t firstVoxelId, const uint32_t numVoxels, std::vector<Voxel> &voxels) {
            if (!(extent != 0 && (extent & (extent - 1)) == 0)) { // extent is power of 2, http://www.graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
                throw std::runtime_error("OctreeCellBuilding: Invalid extent.");
            }

            encodeOctreeNode(&octree[octreeNodeIdx], &extent, nullptr, nullptr);
            //    octree[octreeNodeIdx].extent = extent;

            if (numVoxels == 0) {
                constexpr uint16_t child = OCTREE_NODE_INVALID_CHILD;
                encodeOctreeNode(&octree[octreeNodeIdx], nullptr, nullptr, &child);
                return;
            }
            if (numVoxels == glm::pow(extent, 3)) {
                constexpr uint16_t solid = 1;
                constexpr uint16_t child = OCTREE_NODE_INVALID_CHILD;
                encodeOctreeNode(&octree[octreeNodeIdx], nullptr, &solid, &child);
                //        octree[octreeNodeIdx].solid = 1;
                return;
            }

            // numVoxelsZYX

            // split z
            // [ z < middle.z | z > middle.z ]
            // [ # = zSplitNum | # = numVoxels - zSplitNum ]
            // [ # = numVoxels0 | # = numVoxels1 ]
            const uint32_t zSplitNum = reorderOctreeVoxels(firstVoxelId, numVoxels, voxels, anchor, extent, 2);
            const uint32_t numVoxels0 = zSplitNum;
            const uint32_t numVoxels1 = numVoxels - zSplitNum;

            //        const uint32_t anchorZ = anchor.z + extent / 2;
            //        for (int i = 0; i < numVoxels0; i++) {
            //            assert(voxels[firstVoxelId + i].z < anchorZ);
            //        }
            //        for (int i = numVoxels0; i < numVoxels; i++) {
            //            assert(voxels[firstVoxelId + i].z >= anchorZ);
            //        }

            // split y
            // [ z < middle.z and y < middle.y | z < middle.z and y > middle.y | z > middle.z and y < middle.y | z > middle.z and y > middle.y ]
            // [ # = ySplitNum0 | # = xSplitNum - ySplitNum0 | # = ySplitNum1 | # = numVoxels - xSplitNum - ySplitNum1 ]
            // [ # = numVoxels00 | # = numVoxels01 | # = numVoxels10 | # numVoxels11 ]
            const uint32_t ySplitNum0 = reorderOctreeVoxels(firstVoxelId, numVoxels0, voxels, anchor, extent, 1);
            const uint32_t numVoxels00 = ySplitNum0;
            const uint32_t numVoxels01 = numVoxels0 - numVoxels00;
            const uint32_t ySplitNum1 = reorderOctreeVoxels(firstVoxelId + numVoxels0, numVoxels1, voxels, anchor, extent, 1);
            const uint32_t numVoxels10 = ySplitNum1;
            const uint32_t numVoxels11 = numVoxels1 - numVoxels10;
            // split z
            // [ # = numVoxels000 | # = numVoxels100 | # = numVoxels010 | # = numVoxels110 | # = numVoxels001 | # = numVoxels101 | # = numVoxels011 | # = numVoxels111 ]
            const uint32_t zSplitNum0 = reorderOctreeVoxels(firstVoxelId, numVoxels00, voxels, anchor, extent, 0);
            const uint32_t numVoxels000 = zSplitNum0;
            const uint32_t numVoxels001 = numVoxels00 - numVoxels000;
            const uint32_t zSplitNum1 = reorderOctreeVoxels(firstVoxelId + numVoxels00, numVoxels01, voxels, anchor, extent, 0);
            const uint32_t numVoxels010 = zSplitNum1;
            const uint32_t numVoxels011 = numVoxels01 - numVoxels010;
            const uint32_t zSplitNum2 = reorderOctreeVoxels(firstVoxelId + numVoxels00 + numVoxels01, numVoxels10, voxels, anchor, extent, 0);
            const uint32_t numVoxels100 = zSplitNum2;
            const uint32_t numVoxels101 = numVoxels10 - numVoxels100;
            const uint32_t zSplitNum3 = reorderOctreeVoxels(firstVoxelId + numVoxels00 + numVoxels01 + numVoxels10, numVoxels11, voxels, anchor, extent, 0);
            const uint32_t numVoxels110 = zSplitNum3;
            const uint32_t numVoxels111 = numVoxels11 - numVoxels110;

            const auto child = static_cast<uint16_t>(octree.size());
            encodeOctreeNode(&octree[octreeNodeIdx], nullptr, nullptr, &child);
            //    octree[octreeNodeIdx].child = static_cast<int32_t>(octree.size());
            for (int i = 0; i < 8; i++) {
                octree.emplace_back();
            }

            const uint16_t extentHalf = extent / 2;

            buildCellOctree(octree, child + 0b000, anchor + glm::ivec3(0, 0, 0), extentHalf, firstVoxelId, numVoxels000, voxels);
            buildCellOctree(octree, child + 0b001, anchor + glm::ivec3(extentHalf, 0, 0), extentHalf, firstVoxelId + numVoxels000, numVoxels001, voxels);
            buildCellOctree(octree, child + 0b010, anchor + glm::ivec3(0, extentHalf, 0), extentHalf, firstVoxelId + numVoxels00, numVoxels010, voxels);
            buildCellOctree(octree, child + 0b011, anchor + glm::ivec3(extentHalf, extentHalf, 0), extentHalf, firstVoxelId + numVoxels00 + numVoxels010, numVoxels011, voxels);

            buildCellOctree(octree, child + 0b100, anchor + glm::ivec3(0, 0, extentHalf), extentHalf, firstVoxelId + numVoxels0, numVoxels100, voxels);
            buildCellOctree(octree, child + 0b101, anchor + glm::ivec3(extentHalf, 0, extentHalf), extentHalf, firstVoxelId + numVoxels0 + numVoxels100, numVoxels101, voxels);
            buildCellOctree(octree, child + 0b110, anchor + glm::ivec3(0, extentHalf, extentHalf), extentHalf, firstVoxelId + numVoxels0 + numVoxels10, numVoxels110, voxels);
            buildCellOctree(octree, child + 0b111, anchor + glm::ivec3(extentHalf, extentHalf, extentHalf), extentHalf, firstVoxelId + numVoxels0 + numVoxels10 + numVoxels110, numVoxels111, voxels);
        }

        static uint32_t reorderOctreeVoxels(const uint32_t firstVoxelId, const uint32_t numVoxels, std::vector<Voxel> &voxels, glm::ivec3 cellMin, uint32_t extent, int axis) {
            const auto middle = std::partition(voxels.begin() + firstVoxelId, voxels.begin() + firstVoxelId + numVoxels,
                             [cellMin, extent, axis](const Voxel &voxel) -> bool { return voxel[axis] < cellMin[axis] + extent / 2; });
            return std::distance(voxels.begin() + firstVoxelId, middle);
        }

        static void encodeOctreeNode(OctreeNode *octreeNode, const uint16_t *extent, const uint16_t *solid, const uint16_t *child) {
            if (extent != nullptr) {
                if (!(*extent && !(*extent & (*extent - 1)) &&
                      *extent <= OCTREE_NODE_EXTENT_MAX)) { // ensure that extent is a power of two, http://www.graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
                    throw std::runtime_error("OctreeNodeEncoding: Invalid extent.");
                      }
                uint16_t extentExponent = 0;
                for (uint8_t i = 0; i < 8; i++) {
                    if (*extent & (1 << i)) {
                        extentExponent = i;
                        break;
                    }
                }
                *octreeNode = (*octreeNode & ~OCTREE_NODE_EXTENT_BITS) | (extentExponent << OCTREE_NODE_EXTENT_SHIFT);
            }
            if (solid != nullptr) {
                if (!(*solid == 0 || *solid == 1)) {
                    throw std::runtime_error("OctreeNodeEncoding: Solid must be either 0 or 1.");
                }
                *octreeNode = (*octreeNode & ~OCTREE_NODE_SOLID_BITS) | ((*solid & 1) << OCTREE_NODE_SOLID_SHIFT);
            }
            if (child != nullptr) {
                if ((*child & OCTREE_NODE_CHILD_BITS) != *child) { // ensure that the child pointer uses 12 bits only
                    throw std::runtime_error("OctreeNodeEncoding: Invalid child pointer.");
                }
                *octreeNode = (*octreeNode & ~OCTREE_NODE_CHILD_BITS) | *child;
            }
        }

        static void testOctreeNodeEncoding(const uint16_t extent, const uint16_t solid, const uint16_t child) {
            OctreeNode octreeNode;
            encodeOctreeNode(&octreeNode, &extent, &solid, &child);
            const uint16_t e = OCTREE_NODE_EXTENT(octreeNode);
            const uint16_t s = OCTREE_NODE_SOLID(octreeNode);
            const uint16_t c = OCTREE_NODE_CHILD(octreeNode);
            assert(e == extent && s == solid && c == child);
        }

        static void decodeOctreeNode(const OctreeNode *octreeNode, uint16_t *extent, uint16_t *solid, uint16_t *child) {
            *extent = 1 << ((*octreeNode & (1 << OCTREE_NODE_EXTENT_SHIFT)) >> OCTREE_NODE_EXTENT_SHIFT);
            *solid = (*octreeNode >> OCTREE_NODE_SOLID_SHIFT) & 1;
            *child = *octreeNode & OCTREE_NODE_CHILD_BITS;
        }

        static uint32_t nextPowerOfTwo(uint32_t v) {
            // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
            v--;
            v |= v >> 1;
            v |= v >> 2;
            v |= v >> 4;
            v |= v >> 8;
            v |= v >> 16;
            v++;
            return v;
        }
    };
} // namespace raven