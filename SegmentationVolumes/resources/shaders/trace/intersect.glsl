#ifndef INTERSECT_GLSL
#define INTERSECT_GLSL

#include "../defines.glsl"
#include "../raycommon.glsl"

#include "aabb.glsl"
#include "svo.glsl"
#include "svdag.glsl"
#include "svdag_occupancy_field.glsl"

float intersect_minDistancePointBox(in const vec3 p, in const vec3 bmin, in const vec3 bmax) {
    vec3 d = max(vec3(0), max(bmin - p, p - bmax));
    return length(d);
}

float hitSphere(const vec3 sphereCenter, const float sphereRadius, const vec3 rayOrigin, const vec3 rayDirection) {
    vec3  oc           = rayOrigin - sphereCenter;
    float a            = dot(rayDirection, rayDirection);
    float b            = 2.0 * dot(oc, rayDirection);
    float c            = dot(oc, oc) - sphereRadius * sphereRadius;
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0) {
        return -1.0;
    } else {
        return (-b - sqrt(discriminant)) / (2.0 * a);
    }
}

bool intersect(in const int objectDescriptorId, in const int primitiveId, in vec3 origin, in vec3 direction, in const mat4x3 objectToWorld, in const mat4x3 worldToObject, out float tHit) {
    const ObjectDescriptor objectDescriptor = g_objectDescriptors[objectDescriptorId];
    buffer_aabb aabbs = buffer_aabb(objectDescriptor.aabbAddress);
    const AABB aabb = aabbs.g_aabbs[primitiveId];
    //    if (aabb.materialId != 27) {
    //        return false;
    //    }

    if (g_spheres != 0) {
        const vec3 aabbMin = vec3(aabb.minX, aabb.minY, aabb.minZ);
        const vec3 aabbMax = vec3(aabb.maxX, aabb.maxY, aabb.maxZ);
        const vec3 aabbMinWorld = objectToWorld * vec4(aabbMin.xyz, 1.0);
        const vec3 aabbMaxWorld = objectToWorld * vec4(aabbMax.xyz, 1.0);
        vec3 center = 0.5 * (aabbMinWorld + aabbMaxWorld);
        float radius = 0.5 * min(min(aabbMaxWorld.x - aabbMinWorld.x, aabbMaxWorld.y - aabbMinWorld.y), aabbMaxWorld.z - aabbMinWorld.z);

        tHit = hitSphere(center, radius, origin, direction);
        return tHit >= 0;
    }

    if (g_lodType == LOD_TYPE_SVDAG_OCCUPANCY_FIELD) {
        const vec3 aabbMin = vec3(aabb.minX, aabb.minY, aabb.minZ);
        const vec3 aabbMax = vec3(aabb.maxX, aabb.maxY, aabb.maxZ);
        const vec3 aabbMinWorld = objectToWorld * vec4(aabbMin.xyz, 1.0);
        const vec3 aabbMaxWorld = objectToWorld * vec4(aabbMax.xyz, 1.0);

        origin = worldToObject * vec4(origin, 1);
        direction = worldToObject * vec4(direction, 0);

        if (aabb_intersect(aabbMin, aabbMax, origin, 1 / direction, tHit)) {
            if (g_lod == 0 || (g_frame == 0 && g_lodDisableOnFirstFrame != 0) || objectDescriptor.lodAddress == 0) {
                return true;
            }

            const float dist = g_lodDistance == 0 ? 0 : intersect_minDistancePointBox(vec3(g_ray_origin_x, g_ray_origin_y, g_ray_origin_z), aabbMinWorld, aabbMaxWorld);

            if (dist > g_lodDistanceOctree) { // no LOD, only AABBs
                return true;
            }

            const bool traverseOccupancyFields = dist <= g_lodDistanceVoxel;

            int iterations;
            origin = origin + tHit * direction;
            origin = origin - aabbMin;// translate lod to (0,0,0), LOD is then in [(0,0,0), lodSize]
            tHit += svdag_occupancy_field_traverse(objectDescriptor.lodAddress, origin, direction, aabb.lod, traverseOccupancyFields, iterations);

            if (tHit >= FLT_MAX) {
                return false;
            }
            return true;
        }

        return false;
    }

    if (g_lodType == LOD_TYPE_SVDAG) {
        const vec3 aabbMin = vec3(aabb.minX, aabb.minY, aabb.minZ);
        const vec3 aabbMax = vec3(aabb.maxX, aabb.maxY, aabb.maxZ);
        const vec3 aabbMinWorld = objectToWorld * vec4(aabbMin.xyz, 1.0);
        const vec3 aabbMaxWorld = objectToWorld * vec4(aabbMax.xyz, 1.0);

        origin = worldToObject * vec4(origin, 1);
        direction = worldToObject * vec4(direction, 0);

        if (aabb_intersect(aabbMin, aabbMax, origin, 1 / direction, tHit)) {
            if (g_lod == 0 || (g_frame == 0 && g_lodDisableOnFirstFrame != 0) || objectDescriptor.lodAddress == 0) {
                return true;
            }

            const float dist = g_lodDistance == 0 ? 0 : intersect_minDistancePointBox(vec3(g_ray_origin_x, g_ray_origin_y, g_ray_origin_z), aabbMinWorld, aabbMaxWorld);

            if (dist > g_lodDistanceOctree) { // no LOD, only AABBs
                return true;
            }

            int iterations;
            origin = origin + tHit * direction;
            origin = origin - aabbMin;// translate lod to (0,0,0), LOD is then in [(0,0,0), lodSize]
            tHit += svdag_traverse(objectDescriptor.lodAddress, origin, direction, aabb.lod, iterations);

            if (tHit >= FLT_MAX) {
                return false;
            }
            return true;
        }

        return false;
    }

    if (g_lodType == LOD_TYPE_SVO) {
        const vec3 aabbMin = vec3(aabb.minX, aabb.minY, aabb.minZ);
        const vec3 aabbMax = vec3(aabb.maxX, aabb.maxY, aabb.maxZ);
        const vec3 aabbMinWorld = objectToWorld * vec4(aabbMin.xyz, 1.0);
        const vec3 aabbMaxWorld = objectToWorld * vec4(aabbMax.xyz, 1.0);

        const float dist = g_lodDistance == 0 ? 0 : intersect_minDistancePointBox(vec3(g_ray_origin_x, g_ray_origin_y, g_ray_origin_z), aabbMinWorld, aabbMaxWorld);

        if (g_lod == 0 || (g_frame == 0 && g_lodDisableOnFirstFrame != 0) || objectDescriptor.lodAddress == 0 || dist > g_lodDistanceOctree) {
            if (aabb_intersect(aabbMin, aabbMax, origin, 1 / direction, tHit)) {
                return true;
            }

            return false;
        }

        int iterations;
        tHit = svo_traverse(objectDescriptor.lodAddress, origin, direction, 1 / direction, aabb.lod, ivec3(aabb.minX, aabb.minY, aabb.minZ), iterations);
        if (tHit >= FLT_MAX) {
            return false;
        }
        return true;
    }

    return false;
}

#endif