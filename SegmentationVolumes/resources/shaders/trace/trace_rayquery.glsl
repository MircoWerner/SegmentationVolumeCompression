#ifndef TRACE_RAYQUERY_GLSL
#define TRACE_RAYQUERY_GLSL

#include "../raycommon.glsl"
#include "trace.glsl"
#include "aabb.glsl"

#include "intersect.glsl"

bool trace(in const vec3 origin, in const vec3 direction, in const float tMin, in const float tMax, out HitPayload hitPayload) {
    rayQueryEXT rayQuery;
    rayQueryInitializeEXT(rayQuery,
    topLevelAS,
    gl_RayFlagsOpaqueEXT,
    0xFF,
    origin,
    tMin,
    direction,
    tMax);

    hitPayload.t = tMax;

    while (rayQueryProceedEXT(rayQuery)) {
        float tHit;
        if (intersect(rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false), rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false), origin, direction, rayQueryGetIntersectionObjectToWorldEXT(rayQuery, false), rayQueryGetIntersectionWorldToObjectEXT(rayQuery, false), tHit)) {
            rayQueryGenerateIntersectionEXT(rayQuery, tHit);
            if (tHit < hitPayload.t) {
                hitPayload.objectDescriptorId = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, false);
                hitPayload.primitiveId = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
                hitPayload.t = tHit;
            }
        }
    }

    return hitPayload.t < tMax;
}

#endif