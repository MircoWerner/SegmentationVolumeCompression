#ifndef TRACE_GLSL
#define TRACE_GLSL

#include "../defines.glsl"
#include "../raycommon.glsl"
#include "../raystructs.glsl"
#include "../material.glsl"

bool trace(in const vec3 origin, in const vec3 direction, in const float tMin, in const float tMax, out HitPayload hitPayload);

#define EPSILON_NORMAL 0.0001
vec3 calculateNormal(in vec3 position, in vec3 direction) {
    vec3 normal = vec3(0);
    float dxEdge = abs(round(position.x) - position.x);
    float dyEdge = abs(round(position.y) - position.y);
    float dzEdge = abs(round(position.z) - position.z);
    float dMinEdge = min(dxEdge, min(dyEdge, dzEdge));
    int axis = dxEdge == dMinEdge ? 0 : (dyEdge == dMinEdge ? 1 : 2);
    normal[axis] = -sign(direction[axis]);
    return normal;
}

void intersectionInfo(in HitPayload payload, in vec3 origin, in vec3 direction, out vec3 positionWorld, out vec3 normalWorld, out Material material) {
    const ObjectDescriptor object = g_objectDescriptors[payload.objectDescriptorId];
    buffer_aabb aabbs = buffer_aabb(object.aabbAddress);

    const AABB aabb = aabbs.g_aabbs[payload.primitiveId];
    const vec3 aabbMin = vec3(aabb.minX, aabb.minY, aabb.minZ);
    const vec3 aabbMax = vec3(aabb.maxX, aabb.maxY, aabb.maxZ);
    const vec3 center = (aabbMin + aabbMax) * 0.5;
    positionWorld = origin + payload.t * direction;
    normalWorld = calculateNormal(positionWorld, direction);
    if (g_spheres != 0) {
        normalWorld = normalize(positionWorld - center);
    }

    const uint materialId = g_label_metadata == 0 ? aabb.labelId : aabb.labelId & 0xFFFFu;
    material = g_materials[materialId];

    const uint labelMetadata = g_label_metadata == 0 ? 0 : aabb.labelId >> 16u;
    material.baseColor = material_getBaseColor(material.baseColor, labelMetadata);
}

#endif