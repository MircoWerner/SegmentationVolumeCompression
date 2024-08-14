#ifndef AABB_GLSL
#define AABB_GLSL

#include "../utility/constants.glsl"

vec3 aabb_min3V(in const vec3 v, in const vec3 w) {
    return vec3(min(v.x, w.x), min(v.y, w.y), min(v.z, w.z));
}

vec3 aabb_max3V(in const vec3 v, in const vec3 w) {
    return vec3(max(v.x, w.x), max(v.y, w.y), max(v.z, w.z));
}

bool aabb_intersect(in const vec3 minAABB, in const vec3 maxAABB, in const vec3 origin, in const vec3 reciprocalDirection, out float tMin) {
    tMin = 0.0;
    float tMax = INFINITY;

    const vec3 t1 = (minAABB - origin) * reciprocalDirection;
    const vec3 t2 = (maxAABB - origin) * reciprocalDirection;

    const vec3 tMin2 = aabb_min3V(t1, t2);
    const vec3 tMax2 = aabb_max3V(t1, t2);

    tMin = max(max(tMin2.x, tMin2.y), max(tMin2.z, tMin));
    tMax = min(min(tMax2.x, tMax2.y), min(tMax2.z, tMax));

    return tMin <= tMax;
}

#endif