#ifndef SVDAG_GLSL
#define SVDAG_GLSL

#include "svdag_common.glsl"

float svdag_traverse(in const uint64_t lodAddress, in vec3 origin, in const vec3 direction, in const uint lodIndex, out int iterations) {
    // assumes that the anchor of the root node is (0, 0, 0) (translate origin accordingly)
    // assumes that the extent of the root node is 16
    // assumes that the origin of the ray is on the surface of the root node (perform AABB intersection before)
    origin = clamp(origin, vec3(0), vec3(16));

    // debug
    int iteration = 0;

    float t = 0.f;

    // init
    int level = LOD_LEVELS;

    SVDAG[LOD_LEVELS + 1] nodes;
    nodes[level] = svdag_fetchNode(lodAddress, lodIndex);

    ivec3[LOD_LEVELS + 1] intersections;
    intersections[level] = ivec3(0);

    vec3 position = origin;

    while (level >= 0 && level <= LOD_LEVELS && iteration++ < 128) {
        // peek
        const SVDAG node = nodes[level];

        // check if node is solid
        if (svdag_isLeaf(node) && svdag_isSolid(node)) {
            return t;
        }

        // check if node is leaf
        if (svdag_isLeaf(node)) {
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
        const ivec3 currentAnchor = intersections[level];
        const int currentExtent = 1 << (level - 1);
        const ivec3 currentCenter = currentAnchor + currentExtent;
        const uint closestChild = svdag_getClosestChild(currentCenter, position, direction);

        // descend
        const uint childIndex = svdag_getChildNodeIndex(node, closestChild);
        nodes[level - 1] = svdag_fetchNode(lodAddress, childIndex);
        intersections[level - 1] = currentAnchor + currentExtent * svdag_vectorizeOctreeChildIndex(closestChild);
        level--;
    }

    iterations = iteration;
    return FLT_MAX;
}

#endif