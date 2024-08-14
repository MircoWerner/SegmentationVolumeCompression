#ifndef TRACE_RAYTRACE_GLSL
#define TRACE_RAYTRACE_GLSL

#include "../raycommon.glsl"
#include "trace.glsl"

bool trace(in const vec3 origin, in const vec3 direction, in const float tMin, in const float tMax, out HitPayload hitPayload) {
    traceRayEXT(topLevelAS, // acceleration structure
    gl_RayFlagsOpaqueEXT, // rayFlags
    0xFF, // cullMask
    0, // sbtRecordOffset
    0, // sbtRecordStride
    0, // missIndex
    origin, // ray origin
    tMin, // ray min range
    direction, // ray direction
    tMax, // ray max range
    0// payload (location = 0)
    );

    hitPayload = payload;
    return payload.t >= 0.0;
}

#endif