#ifndef SVDAG_OCCUPANCY_FIELD_GLSL
#define SVDAG_OCCUPANCY_FIELD_GLSL

#include "svdag_common.glsl"
#include "occupancy_field.glsl"

//#define SVDAG_OCCUPANCY_FIELD_STACK_ARRAY

float svdag_occupancy_field_traverse(in const uint64_t lodAddress, in vec3 origin, in const vec3 direction, in const uint lodIndex, in const bool traverseOccupancyFields, out int iterations) {
    // assumes that the anchor of the root node is (0, 0, 0) (translate origin accordingly)
    // assumes that the extent of the root node is 16
    // assumes that the origin of the ray is on the surface of the root node (perform AABB intersection before)
    origin = clamp(origin, vec3(0), vec3(16));

    // debug
    int iteration = 0;

    float t = 0.f;

    // init
    int level = LOD_LEVELS;

    #ifdef SVDAG_OCCUPANCY_FIELD_STACK_ARRAY
    SVDAG[LOD_LEVELS + 1 - 2] nodes;// the two lowest levels are replaced by occupancy field
    nodes[level - 2] = svdag_fetchNode(lodAddress, lodIndex);

    ivec3[LOD_LEVELS + 1 - 2] intersections;// the two lowest levels are replaced by occupancy field
    intersections[level - 2] = ivec3(0);
    #else
    SVDAG nodeLevel4 = svdag_fetchNode(lodAddress, lodIndex);
    SVDAG nodeLevel3;
    SVDAG nodeLevel2;

    ivec3 intersectionLevel4 = ivec3(0);
    ivec3 intersectionLevel3;
    ivec3 intersectionLevel2;
    #endif

    vec3 position = origin;

    while (level >= 0 && level <= LOD_LEVELS && iteration++ < 128) {
        // peek
        #ifdef SVDAG_OCCUPANCY_FIELD_STACK_ARRAY
        const SVDAG node = nodes[level - 2];
        #else
        const SVDAG node = level == 4 ? nodeLevel4 : level == 3 ? nodeLevel3 : nodeLevel2;
        #endif

        // check if node is solid
        if (level > 2 && svdag_isLeaf(node) && svdag_isSolid(node)) {
            return t;
        }

        // check if node is leaf
        if (svdag_isLeaf(node)) {
            if (level <= 2) {
                #ifdef SVDAG_OCCUPANCY_FIELD_STACK_ARRAY
                const ivec3 currentAnchor = intersections[level - 2];
                #else
                const ivec3 currentAnchor = level == 4 ? intersectionLevel4 : level == 3 ? intersectionLevel3 : intersectionLevel2;
                #endif
                float tField;
                if (occupancy_field_traverse(node.child1, node.child2, position - currentAnchor, direction, traverseOccupancyFields, tField)) {
                    return t + tField;
                }
            }

            // advance position through empty leaf (DDA)
            const vec3 nextT = svdag_nextT(position, direction, level) + t;
            t = min(nextT.x, min(nextT.y, nextT.z));

            position = origin + t * direction;

            // ascend
            const int axis = nextT.x == t ? 0 : (nextT.y == t ? 1 : 2);

            const int p = int(round(position[axis]));

            if (p == 0 || p == 16) {
                return FLT_MAX;
            }

            level = findLSB(p) + 1;

            continue;
        }

        // get closest child
        #ifdef SVDAG_OCCUPANCY_FIELD_STACK_ARRAY
        const ivec3 currentAnchor = intersections[level - 2];
        #else
        const ivec3 currentAnchor = level == 4 ? intersectionLevel4 : level == 3 ? intersectionLevel3 : intersectionLevel2;
        #endif
        const int currentExtent = 1 << (level - 1);
        const ivec3 currentCenter = currentAnchor + currentExtent;
        const uint closestChild = svdag_getClosestChild(currentCenter, position, direction);

        // descend
        const uint childIndex = svdag_getChildNodeIndex(node, closestChild);
        #ifdef SVDAG_OCCUPANCY_FIELD_STACK_ARRAY
        nodes[level - 1 - 2] = svdag_fetchNode(lodAddress, childIndex);
        intersections[level - 1 - 2] = currentAnchor + currentExtent * svdag_vectorizeOctreeChildIndex(closestChild);
        #else
        if (level - 1 == 4) {
            nodeLevel4 = svdag_fetchNode(lodAddress, childIndex);
            intersectionLevel4 = currentAnchor + currentExtent * svdag_vectorizeOctreeChildIndex(closestChild);
        }
        if (level - 1 == 3) {
            nodeLevel3 = svdag_fetchNode(lodAddress, childIndex);
            intersectionLevel3 = currentAnchor + currentExtent * svdag_vectorizeOctreeChildIndex(closestChild);
        }
        if (level - 1 == 2) {
            nodeLevel2 = svdag_fetchNode(lodAddress, childIndex);
            intersectionLevel2 = currentAnchor + currentExtent * svdag_vectorizeOctreeChildIndex(closestChild);
        }
        #endif
        level--;
    }

    iterations = iteration;
    return FLT_MAX;
}

#endif