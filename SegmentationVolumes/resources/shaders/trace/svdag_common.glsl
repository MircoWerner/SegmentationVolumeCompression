#ifndef SVDAG_COMMONG_GLSL
#define SVDAG_COMMONG_GLSL

#define LOD_LEVELS 4
#define LOD_INVALID_POINTER 0xFFFFFFFF

SVDAG svdag_fetchNode(in const uint64_t lodAddress, in const uint index) {
    // https://github.com/KhronosGroup/Vulkan-Docs/issues/1016
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorBufferInfo.html
    // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceLimits.html
    buffer_svdag svdagOffset = buffer_svdag(lodAddress + uint64_t(32) * uint64_t(index));
    return svdagOffset.g_svdag[0];
}

bool svdag_isSolid(in const SVDAG node) {
    return node.child1 > 0 || node.child2 > 0;
}

bool svdag_isLeaf(in const SVDAG node) {
    return node.child0 == LOD_INVALID_POINTER;
}

float svdag_roundDownMultiple(in const float n, in const float multiple) {
    return floor(n) - mod(floor(n), multiple);
}

float svdag_roundUpMultiple(in const float n, in const float multiple) {
    const float remainder = mod(n, multiple);
    return remainder == 0 ? n : n + multiple - remainder;
}

vec3 svdag_nextT(in const vec3 position, in const vec3 direction, in const uint level) {
    const float multiple = float(1 << level);

    const vec3 deltaVoxel = vec3(sign(direction.x) >= 0.0 ? svdag_roundDownMultiple(position.x, multiple) + multiple - position.x : svdag_roundUpMultiple(position.x, multiple) - multiple - position.x,
    sign(direction.y) >= 0.0 ? svdag_roundDownMultiple(position.y, multiple) + multiple - position.y : svdag_roundUpMultiple(position.y, multiple) - multiple - position.y,
    sign(direction.z) >= 0.0 ? svdag_roundDownMultiple(position.z, multiple) + multiple - position.z : svdag_roundUpMultiple(position.z, multiple) - multiple - position.z);

    return vec3(direction.x == 0 ? FLT_MAX : (deltaVoxel.x / direction.x),
    direction.y == 0 ? FLT_MAX : (deltaVoxel.y / direction.y),
    direction.z == 0 ? FLT_MAX : (deltaVoxel.z / direction.z));
}

uint svdag_getClosestChild(in const ivec3 center, in const vec3 position, in const vec3 d) {
    // zyx, bit is set iff the closest quadrant is the positive one
    // e.g. 101 -> closest child is in +z,-y,+x quadrant relative to the origin
    const uint z = center.z < position.z ? 1 : (center.z > position.z ? 0 : (sign(d.z) >= 0.0 ? 1 : 0));
    const uint y = center.y < position.y ? 1 : (center.y > position.y ? 0 : (sign(d.y) >= 0.0 ? 1 : 0));
    const uint x = center.x < position.x ? 1 : (center.x > position.x ? 0 : (sign(d.x) >= 0.0 ? 1 : 0));
    return (z << 2) | (y << 1) | x;
}

uint svdag_getChildNodeIndex(in const SVDAG node, in const uint childIndex) {
    uint child = 0;
    switch (childIndex) {
        case 0:
        child = node.child0;
        break;
        case 1:
        child = node.child1;
        break;
        case 2:
        child = node.child2;
        break;
        case 3:
        child = node.child3;
        break;
        case 4:
        child = node.child4;
        break;
        case 5:
        child = node.child5;
        break;
        case 6:
        child = node.child6;
        break;
        case 7:
        child = node.child7;
        break;
    }
    return child;
}

ivec3 svdag_vectorizeOctreeChildIndex(in const uint childIndex) {
    // ZYX = lower three bits of child index
    return ivec3(childIndex & 0x1u, (childIndex & 0x2u) >> 1, (childIndex & 0x4u) >> 2);
}

#endif