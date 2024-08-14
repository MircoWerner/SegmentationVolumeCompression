#ifndef SVO_GLSL
#define SVO_GLSL

#define SVO_STACK_SIZE 4
#define SVO_MAX_ITERATIONS 200// increase this when you see view-dependent artifacts

#define SVO_NODE_EXTENT_BITS 0xE000
#define SVO_NODE_SOLID_BITS 0x1000
#define SVO_NODE_CHILD_BITS 0x0FFF

#define SVO_NODE_EXTENT_SHIFT 13
#define SVO_NODE_SOLID_SHIFT 12

#define SVO_NODE_INVALID_CHILD 0x0FFF

#define SVO_NODE_EXTENT(A) (1 << (((A) & SVO_NODE_EXTENT_BITS) >> SVO_NODE_EXTENT_SHIFT))
#define SVO_NODE_SOLID(A) (((A) & SVO_NODE_SOLID_BITS) >> SVO_NODE_SOLID_SHIFT)
#define SVO_NODE_CHILD(A) ((A) & SVO_NODE_CHILD_BITS)

#include "../raystructs.glsl"

// ============== EFFICIENT SVO TRAVERSAL USING THE CLOSEST CHILD NODE ==================== //
uint svo_fetchNode(in const uint64_t lodAddress, in const uint index) {
    // https://github.com/KhronosGroup/Vulkan-Docs/issues/1016
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorBufferInfo.html
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceLimits.html
    buffer_svo svoOffset = buffer_svo(lodAddress + uint64_t(2) * uint64_t(index));
    return svoOffset.g_svo[0];
}

ivec3 svo_vectorizeChildIndex(uint childIndex) {
    // ZYX = lower three bits of child index
    return ivec3(childIndex & 0x1u, (childIndex & 0x2u) >> 1, (childIndex & 0x4u) >> 2);
}

vec3 svo_intersectCartesianPlanes(vec3 e, vec3 d, vec3 origin) {
    vec3 denominator = d;
    vec3 q = (origin - e);
    vec3 t = vec3(denominator.x != 0 ? q.x / denominator.x : FLT_MAX, denominator.y != 0 ? q.y / denominator.y : FLT_MAX, denominator.z != 0 ? q.z / denominator.z : FLT_MAX);
    return vec3(t.x > 0 ? t.x : FLT_MAX, t.y > 0 ? t.y : FLT_MAX, t.z > 0 ? t.z : FLT_MAX);
}

float svo_minVec3Component(vec3 v) {
    return min(v.x, min(v.y, v.z));
}

bool svo_getNextHitWithPlanes(vec3 currentPosition, vec3 d, ivec3 origin, out vec3 nextHit) {
    float t = svo_minVec3Component(svo_intersectCartesianPlanes(currentPosition, d, origin));
    if (t < FLT_MAX) {
        nextHit = currentPosition + t * d;
        return true;
    }
    return false;
}

uint svo_getClosestChild(ivec3 origin, vec3 currentPosition, vec3 d) {
    // zyx, bit is set iff the closest quadrant is the positive one
    // e.g. 101 -> closest child is in +z,-y,+x quadrant relative to the origin
    uint z = origin.z < currentPosition.z ? 1 : (origin.z > currentPosition.z ? 0 : (sign(d.z) >= 0.0 ? 1 : 0));
    uint y = origin.y < currentPosition.y ? 1 : (origin.y > currentPosition.y ? 0 : (sign(d.y) >= 0.0 ? 1 : 0));
    uint x = origin.x < currentPosition.x ? 1 : (origin.x > currentPosition.x ? 0 : (sign(d.x) >= 0.0 ? 1 : 0));
    return (z << 2) | (y << 1) | x;
}

uint svo_fetchChildNode(in const uint64_t lodAddress, uint octreeNode, uint childIndex, uint octreeRootIndex) {
    return svo_fetchNode(lodAddress, octreeRootIndex + SVO_NODE_CHILD(octreeNode) + childIndex);
}

bool svo_isSolid(uint octreeNode) {
    return SVO_NODE_SOLID(octreeNode) == 1;
}

bool svo_isLeaf(uint octreeNode) {
    return SVO_NODE_CHILD(octreeNode) == SVO_NODE_INVALID_CHILD;
}

struct OctreeNodeIntersection {
    uint octreeNode;// store the octree node directly instead of the index into the octree array -> decrease global memory accesses!
    vec3 currentPosition;// current position of the ray in the octree
    ivec3 anchor;
};

float svo_traverse(in const uint64_t lodAddress, vec3 e, vec3 d, vec3 oneDividedByD, uint octreeIndex, ivec3 anchor, out int iterations) {
    uint rootNode = svo_fetchNode(lodAddress, octreeIndex);

    OctreeNodeIntersection octreeStack[SVO_STACK_SIZE];
    int stackPointer = 0;

    float rootMin;
    if (!aabb_intersect(anchor, anchor + ivec3(16), e, oneDividedByD, rootMin)) {
        return FLT_MAX;
    }
    if (svo_isSolid(rootNode)) {
        return rootMin;
    }
    if (svo_isLeaf(rootNode)) {
        return FLT_MAX;
    }

    octreeStack[stackPointer++] = OctreeNodeIntersection(rootNode, e, anchor);

    int maxStackPointer = 0;
    iterations = 0;
    while (stackPointer > 0 && iterations < SVO_MAX_ITERATIONS) {
        maxStackPointer = max(maxStackPointer, stackPointer);
        iterations++;

        OctreeNodeIntersection currentIntersection = octreeStack[stackPointer - 1];// peek stack

        vec3 currentPosition = currentIntersection.currentPosition;// save current position
        uint nextExtent = SVO_NODE_EXTENT(currentIntersection.octreeNode) >> 1;
        //        uint nextExtent = currentIntersection.extent >> 1;
        ivec3 currentOrigin = currentIntersection.anchor + ivec3(nextExtent);

        uint closestChild = svo_getClosestChild(currentOrigin, currentPosition, d);// relative child index 0..7 of the closest child

        vec3 nextHit;
        if (!svo_getNextHitWithPlanes(currentPosition, d, currentOrigin, nextHit)) {
            stackPointer--;// there are no more children we can traverse
        } else {
            octreeStack[stackPointer - 1].currentPosition = nextHit;
        }

        uint childNode = svo_fetchChildNode(lodAddress, currentIntersection.octreeNode, closestChild, octreeIndex);
        ivec3 childAnchor = currentIntersection.anchor + int(nextExtent) * svo_vectorizeChildIndex(closestChild);
        if (svo_isSolid(childNode)) {
            float newT;
            // test if we are behind the intersected node
            if (aabb_intersect(childAnchor, childAnchor + ivec3(nextExtent), e, oneDividedByD, newT)) {
                return newT;// we are done here, because we itersected with the first solid voxel along the ray which is guarateed to be the closest
            }
        }
        if (svo_isLeaf(childNode)) {
            continue;// do not care for this empty leaf
        }

        octreeStack[stackPointer++] = OctreeNodeIntersection(childNode, currentPosition, childAnchor);// push child node on the stack
    }
    return FLT_MAX;
}

#endif