#ifndef OCCUPANCY_FIELD_GLSL
#define OCCUPANCY_FIELD_GLSL

#include "../utility/constants.glsl"
#include "aabb.glsl"

#define OCCUPANCY_FIELD_DIMENSION 4

uint occupancy_field_voxelIndexToLinearIndex(in const ivec3 voxel) {
    // voxel \in [0, 3]^3
    // ZYX (4x4x4)
    return voxel.z * 16 + voxel.y * 4 + voxel.x;
}

uint occupancy_field_fetch(in const uint bitFieldUpper, in const uint bitFieldLower, in const ivec3 voxel) {
    if (any(greaterThanEqual(voxel, ivec3(OCCUPANCY_FIELD_DIMENSION))) || any(lessThan(voxel, ivec3(0)))) {
        return 0u;
    }

    const uint linearIndex = occupancy_field_voxelIndexToLinearIndex(voxel);
    if (linearIndex < 32) {
        return (bitFieldLower >> linearIndex) & 1u;
    } else {
        return (bitFieldUpper >> (linearIndex - 32)) & 1u;
    }
}

vec3 occupancy_field_deltaT(in const vec3 direction) {
    return vec3(direction.x == 0 ? FLT_MAX : (sign(direction.x) / direction.x),
    direction.y == 0 ? FLT_MAX : (sign(direction.y) / direction.y),
    direction.z == 0 ? FLT_MAX : (sign(direction.z) / direction.z));
}

vec3 occupancy_field_nextT(in const vec3 position, in const vec3 direction) {
    return vec3(direction.x == 0 ? FLT_MAX : ((floor(position.x) + (sign(direction.x) >= 0.0 ? 1 : 0) - position.x) / direction.x),
    direction.y == 0 ? FLT_MAX : ((floor(position.y) + (sign(direction.y) >= 0.0 ? 1 : 0) - position.y) / direction.y),
    direction.z == 0 ? FLT_MAX : ((floor(position.z) + (sign(direction.z) >= 0.0 ? 1 : 0) - position.z) / direction.z));
}

bool occupancy_field_traverse(in const uint bitFieldUpper, in const uint bitFieldLower, in vec3 origin, in const vec3 direction, in const bool traverseOccupancyFields, out float t) {
    // assert bitField \in [0,OCCUPANCY_FIELD_DIMENSION]^3
    origin = clamp(origin, vec3(0), vec3(OCCUPANCY_FIELD_DIMENSION));

    if (bitFieldUpper == 0u && bitFieldLower == 0u) {
        // trivial empty
        return false;
    }

    if (!traverseOccupancyFields) {
        t = 0;
        return true;
    }

    if (bitFieldUpper == 0xFFFFFFFF && bitFieldLower == 0xFFFFFFFF) {
        // trivial solid
        t = 0;
        return true;
    }

    t = 0;

    const vec3 deltaT = occupancy_field_deltaT(direction);
    const ivec3 deltaVoxel = ivec3(sign(direction));

    vec3 nextT = occupancy_field_nextT(origin, direction);
    ivec3 voxel = ivec3(floor(origin));

    int iteration = 0;
    while (iteration++ < 128) {
        if (occupancy_field_fetch(bitFieldUpper, bitFieldLower, voxel) > 0) {
            // get bit field information of current voxel
            return true;
        }

        t = min(nextT.x, min(nextT.y, nextT.z));

        const int axis = nextT.x == t ? 0 : (nextT.y == t ? 1 : 2);

        nextT[axis] += deltaT[axis];
        voxel[axis] += deltaVoxel[axis];

        if (any(greaterThanEqual(voxel, ivec3(OCCUPANCY_FIELD_DIMENSION))) || any(lessThan(voxel, ivec3(0)))) {
            return false;
        }
    }

    return false;
}

#endif