#ifndef RAYCOMMON_GLSL
#define RAYCOMMON_GLSL

struct HitPayload {
    int objectDescriptorId; // gl_InstanceCustomIndexEXT
    int primitiveId; // gl_PrimitiveID
    float t; // distance
};

#endif