#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <iostream>
#include <vector>

namespace raven {
    class DAGTraversalTest {
    public:
        static void test() {
            DAGTraversalTest dagTraversal;

            {
                glm::vec3 origin(0, 1, 0);
                glm::vec3 direction(16, 6, 0);
                direction = glm::normalize(direction);

                float t = dagTraversal.traverse(origin, direction);
                glm::vec3 position = origin + t * direction;

                std::cout << "t=" << t << ", position=(" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;

                assert(glm::abs(t - 11.748f) < 0.001f);
            }

            {
                glm::vec3 origin(16, 7, 0);
                glm::vec3 direction(-16, -6, 0);
                direction = glm::normalize(direction);

                float t = dagTraversal.traverse(origin, direction);
                glm::vec3 position = origin + t * direction;

                std::cout << "t=" << t << ", position=(" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;

                assert(glm::abs(t - 2.136f) < 0.001f);
            }

            {
                glm::vec3 origin(9, 16, 0);
                glm::vec3 direction(0, -1, 0);
                direction = glm::normalize(direction);

                float t = dagTraversal.traverse(origin, direction);
                glm::vec3 position = origin + t * direction;

                std::cout << "t=" << t << ", position=(" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;

                assert(glm::abs(t - 8.f) < 0.001f);
            }

            {
                glm::vec3 origin(12, 0, 0);
                glm::vec3 direction(-2, 3, 0);
                direction = glm::normalize(direction);

                float t = dagTraversal.traverse(origin, direction);
                glm::vec3 position = origin + t * direction;

                std::cout << "t=" << t << ", position=(" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;

                assert(glm::abs(t - 6.00925f) < 0.001f);
            }

            {
                glm::vec3 origin(11.5, 16, 0);
                glm::vec3 direction(0, -1, 0);
                direction = glm::normalize(direction);

                float t = dagTraversal.traverse(origin, direction);
                glm::vec3 position = origin + t * direction;

                std::cout << "t=" << t << ", position=(" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;

                assert(glm::abs(t - 9.f) < 0.001f);
            }

            {
                glm::vec3 origin(0, 1, 0);
                glm::vec3 direction(1, 0, 0);
                direction = glm::normalize(direction);

                float t = dagTraversal.traverse(origin, direction);

                std::cout << "t=" << t << std::endl;

                assert(glm::abs(t - FLT_MAX) < 0.001f);
            }

            // aabb
            std::cout << "aabb test" << std::endl;

            {
                glm::vec3 origin(0, 1, 0);
                glm::vec3 direction(16, 6, 0);
                direction = glm::normalize(direction);
                origin = origin - direction;

                float t = dagTraversal.traverse(origin, direction, glm::vec3(0), glm::vec3(16), glm::ivec3(0));
                glm::vec3 position = origin + t * direction;

                std::cout << "t=" << t << ", position=(" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
            }
            {
                glm::vec3 origin(16, 7, 0);
                glm::vec3 direction(-16, -6, 0);
                direction = glm::normalize(direction);
                origin = origin - direction;

                float t = dagTraversal.traverse(origin, direction, glm::vec3(0), glm::vec3(16), glm::ivec3(0));
                glm::vec3 position = origin + t * direction;

                std::cout << "t=" << t << ", position=(" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
            }

            {
                glm::vec3 origin(9, 16, 0);
                glm::vec3 direction(0, -1, 0);
                direction = glm::normalize(direction);
                origin = origin - direction;

                float t = dagTraversal.traverse(origin, direction, glm::vec3(0), glm::vec3(16), glm::ivec3(0));
                glm::vec3 position = origin + t * direction;

                std::cout << "t=" << t << ", position=(" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
            }

            {
                glm::vec3 origin(12, 0, 0);
                glm::vec3 direction(-2, 3, 0);
                direction = glm::normalize(direction);
                origin = origin - direction;

                float t = dagTraversal.traverse(origin, direction, glm::vec3(0), glm::vec3(16), glm::ivec3(0));
                glm::vec3 position = origin + t * direction;

                std::cout << "t=" << t << ", position=(" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
            }

            {
                glm::vec3 origin(11.5, 16, 0);
                glm::vec3 direction(0, -1, 0);
                direction = glm::normalize(direction);
                origin = origin - direction;

                float t = dagTraversal.traverse(origin, direction, glm::vec3(0), glm::vec3(16), glm::ivec3(0));
                glm::vec3 position = origin + t * direction;

                std::cout << "t=" << t << ", position=(" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
            }

            // anchor
            std::cout << "anchor test" << std::endl;

            {
                glm::vec3 origin(0, 1, 0);
                glm::vec3 direction(16, 6, 0);
                direction = glm::normalize(direction);
                origin = origin - direction;
                origin += glm::vec3(1);

                float t = dagTraversal.traverse(origin, direction, glm::vec3(1), glm::vec3(17), glm::ivec3(1));
                glm::vec3 position = origin + t * direction;

                std::cout << "t=" << t << ", position=(" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
            }
        }

        constexpr static uint32_t invalidPointer() { return 0xFFFFFFFF; }

        struct DAGNode {
            uint32_t solid{}; // [31 bit unused | 1 bit solid] - we exploit the 31 unused bits for sorting
            uint32_t child0 = invalidPointer();
            uint32_t child1 = invalidPointer();
            uint32_t child2 = invalidPointer();
            uint32_t child3 = invalidPointer();

            [[nodiscard]] bool isSolid() const { return solid == 1; }

            [[nodiscard]] bool isLeaf() const { return child0 == invalidPointer(); }

            [[nodiscard]] uint32_t getChildNodeIndex(const uint32_t childIndex) const {
                uint child = 0;
                switch (childIndex) {
                    case 0:
                        child = child0;
                        break;
                    case 1:
                        child = child1;
                        break;
                    case 2:
                        child = child2;
                        break;
                    case 3:
                        child = child3;
                        break;
                    default:
                        return invalidPointer();
                }
                return child;
            }
        };

        struct LODIntersection {
            // glm::vec3 position;
            glm::ivec3 anchor;
            // int extent;
        };

        uint getClosestChild(glm::ivec3 center, glm::vec3 position, glm::vec3 d) {
            // zyx, bit is set iff the closest quadrant is the positive one
            // e.g. 101 -> closest child is in +z,-y,+x quadrant relative to the origin
            uint z = center.z < position.z ? 1 : (center.z > position.z ? 0 : (glm::sign(d.z) >= 0.0 ? 1 : 0));
            uint y = center.y < position.y ? 1 : (center.y > position.y ? 0 : (glm::sign(d.y) >= 0.0 ? 1 : 0));
            uint x = center.x < position.x ? 1 : (center.x > position.x ? 0 : (glm::sign(d.x) >= 0.0 ? 1 : 0));
            return (z << 2) | (y << 1) | x;
        }

        float lod_minVec3Component(glm::vec3 v) { return glm::min(v.x, glm::min(v.y, v.z)); }

        glm::ivec3 vectorizeOctreeChildIndex(uint childIndex) {
            // ZYX = lower three bits of child index
            return glm::ivec3(childIndex & 0x1u, (childIndex & 0x2u) >> 1, (childIndex & 0x4u) >> 2);
        }

#define LOD_LEVELS 4
#define EPSILON 0.001f

        glm::vec3 min3V(glm::vec3 v, glm::vec3 w) { return glm::vec3(glm::min(v.x, w.x), glm::min(v.y, w.y), glm::min(v.z, w.z)); }

        glm::vec3 max3V(glm::vec3 v, glm::vec3 w) { return glm::vec3(glm::max(v.x, w.x), glm::max(v.y, w.y), glm::max(v.z, w.z)); }

        bool intersect_aabb(glm::vec3 minAABB, glm::vec3 maxAABB, glm::vec3 origin, glm::vec3 reciprocalDirection, float *tMin) {
            *tMin = 0.0;
            float tMax = FLT_MAX;

            glm::vec3 t1 = (minAABB - origin) * reciprocalDirection;
            glm::vec3 t2 = (maxAABB - origin) * reciprocalDirection;

            glm::vec3 tMin2 = min3V(t1, t2);
            glm::vec3 tMax2 = max3V(t1, t2);

            *tMin = glm::max(glm::max(tMin2.x, tMin2.y), glm::max(tMin2.z, *tMin));
            tMax = glm::min(glm::min(tMax2.x, tMax2.y), glm::min(tMax2.z, tMax));

            return *tMin <= tMax;
        }

        float traverse(glm::vec3 origin, glm::vec3 direction, glm::vec3 minAABB, glm::vec3 maxAABB, glm::vec3 anchor) {
            float aabbT;
            if (!intersect_aabb(minAABB, maxAABB, origin, glm::vec3(1.0) / direction, &aabbT)) {
                std::cout << "did not hit aabb" << std::endl;
                return FLT_MAX;
            }

            origin = origin + aabbT * direction;
            origin = origin - anchor; // translate lod to (0,0,0), LOD is then in [(0,0,0), lodSize]

            return traverse(origin, direction) + aabbT;
        }

        float traverse(glm::vec3 origin, glm::vec3 direction) {
            // assumes that the anchor of the root node is (0, 0, 0) (translate origin accordingly)
            // assumes that the extent of the root node is 16
            // assumes that the origin of the ray is on the surface of the root node (perform AABB intersection before)

            float t = 0.f;

            //         how much t increases when moving through one cell along each axis
            glm::vec3 deltaTLevels[LOD_LEVELS + 1];
            lod_deltaT(direction, deltaTLevels);

            glm::vec3 nextTLevels[LOD_LEVELS + 1];
            for (uint32_t level = 0; level <= LOD_LEVELS; level++) {
                lod_nextT(origin, direction, &nextTLevels[level], level);
            }

            // init
            int level = LOD_LEVELS;

            DAGNode nodes[LOD_LEVELS + 1];
            nodes[level] = m_dag[0];

            LODIntersection intersections[LOD_LEVELS + 1];
            intersections[level] = LODIntersection(glm::ivec3(0));

            glm::vec3 position = origin;

            while (level <= LOD_LEVELS) {
                // peek
                auto &node = nodes[level];

                // check if node is solid
                if (node.isSolid()) {
                    return t;
                }

                if (node.isLeaf()) {
                    // advance position through empty leaf (DDA)
                    glm::vec3 nextT; // = nextTLevels[level];
                    lod_nextT(position, direction, &nextT, level);
                    nextT += glm::vec3(t);
                    float tMin = glm::min(nextT.x, glm::min(nextT.y, nextT.z));

                    t = tMin;
                    position = origin + t * direction;

                    // ascend
                    int axis = nextT.x == tMin ? 0 : (nextT.y == tMin ? 1 : 2);

                    int p = static_cast<int>(glm::round(position[axis]));

                    if (p == 0 || p == 16) {
                        return FLT_MAX;
                    }

                    int leastSignificantBit = ffs(p);

                    level = leastSignificantBit;

                    continue;
                }

                // get closest child
                glm::ivec3 currentAnchor = intersections[level].anchor;
                int currentExtent = 1 << (level - 1);
                glm::ivec3 currentCenter = currentAnchor + currentExtent;
                uint closestChild = getClosestChild(currentCenter, position, direction);

                // descend
                uint32_t childIndex = node.getChildNodeIndex(closestChild);
                nodes[level - 1] = m_dag[childIndex];
                intersections[level - 1] = LODIntersection(currentAnchor + currentExtent * vectorizeOctreeChildIndex(closestChild));
                level--;
            }

            return FLT_MAX;
        }

    private:
        std::vector<DAGNode> m_dag{
                {.solid = 0, .child0 = 1, .child1 = 2, .child2 = 3, .child3 = 4}, // 16 // 0

                {.solid = 0, .child0 = 5, .child1 = 6, .child2 = 7, .child3 = 8},    // 8 // 1
                {.solid = 0, .child0 = 9, .child1 = 10, .child2 = 11, .child3 = 12}, // 8 // 2
                {.solid = 0},                                                        // 8 // 3
                {.solid = 0},                                                        // 8 // 4

                {.solid = 0},                                                         // 4 // 5
                {.solid = 0},                                                         // 4 // 6
                {.solid = 0},                                                         // 4 // 7
                {.solid = 0, .child0 = 13, .child1 = 14, .child2 = 15, .child3 = 16}, // 4 // 8
                {.solid = 0},                                                         // 4 // 9
                {.solid = 0},                                                         // 4 // 10
                {.solid = 0, .child0 = 17, .child1 = 18, .child2 = 19, .child3 = 20}, // 4 // 11
                {.solid = 0, .child0 = 21, .child1 = 22, .child2 = 23, .child3 = 24}, // 4 // 12

                {.solid = 0},                                                         // 2 // 13
                {.solid = 0},                                                         // 2 // 14
                {.solid = 1},                                                         // 2 // 15
                {.solid = 1},                                                         // 2 // 16
                {.solid = 0, .child0 = 25, .child1 = 26, .child2 = 27, .child3 = 28}, // 2 // 17
                {.solid = 0, .child0 = 29, .child1 = 30, .child2 = 31, .child3 = 32}, // 2 // 18
                {.solid = 1},                                                         // 2 // 19
                {.solid = 0, .child0 = 33, .child1 = 34, .child2 = 35, .child3 = 36}, // 2 // 20
                {.solid = 0},                                                         // 2 // 21
                {.solid = 0},                                                         // 2 // 22
                {.solid = 1},                                                         // 2 // 23
                {.solid = 0},                                                         // 2 // 24

                {.solid = 0}, // 1 // 25
                {.solid = 0}, // 1 // 26
                {.solid = 1}, // 1 // 27
                {.solid = 0}, // 1 // 28
                {.solid = 0}, // 1 // 29
                {.solid = 0}, // 1 // 30
                {.solid = 0}, // 1 // 31
                {.solid = 1}, // 1 // 32
                {.solid = 1}, // 1 // 33
                {.solid = 1}, // 1 // 34
                {.solid = 1}, // 1 // 35
                {.solid = 0}, // 1 // 36
        };

        void lod_deltaT(glm::vec3 direction, glm::vec3 *deltaT) {
            glm::vec3 deltaT0 = glm::vec3(direction.x == 0 ? FLT_MAX : (glm::sign(direction.x) / direction.x),
                                          direction.y == 0 ? FLT_MAX : (glm::sign(direction.y) / direction.y),
                                          direction.z == 0 ? FLT_MAX : (glm::sign(direction.z) / direction.z));

            for (int i = 0; i <= LOD_LEVELS; i++) {
                deltaT[i] = deltaT0 * static_cast<float>(1 << i);
            }
        }

        static float roundDownMultiple(const float n, const float multiple) {
            return glm::floor(n) - glm::mod(glm::floor(n), multiple);
        }

        static float roundUpMultiple(const float n, const float multiple) {
            const float remainder = glm::mod(n, multiple);
            if (remainder == 0) {
                return n;
            }
            return n + multiple - remainder;
        }

        static void lod_nextT(glm::vec3 position, glm::vec3 direction, glm::vec3 *nextT, uint32_t level) {
            const auto multiple = static_cast<float>(1 << level);

            const glm::vec3 deltaVoxel = glm::vec3(glm::sign(direction.x) >= 0.0 ? roundDownMultiple(position.x, multiple) + multiple - position.x : roundUpMultiple(position.x, multiple) - multiple - position.x,
                                                   glm::sign(direction.y) >= 0.0 ? roundDownMultiple(position.y, multiple) + multiple - position.y : roundUpMultiple(position.y, multiple) - multiple - position.y,
                                                   glm::sign(direction.z) >= 0.0 ? roundDownMultiple(position.z, multiple) + multiple - position.z : roundUpMultiple(position.z, multiple) - multiple - position.z);

            *nextT = glm::vec3(direction.x == 0 ? FLT_MAX : (deltaVoxel.x / direction.x),
                               direction.y == 0 ? FLT_MAX : (deltaVoxel.y / direction.y),
                               direction.z == 0 ? FLT_MAX : (deltaVoxel.z / direction.z));
        }
    };
} // namespace raven