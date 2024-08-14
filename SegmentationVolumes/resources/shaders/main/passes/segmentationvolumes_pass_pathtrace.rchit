#version 460
#extension GL_EXT_ray_tracing: require
#extension GL_GOOGLE_include_directive: enable

#include "../../raycommon.glsl"

layout (location = 0) rayPayloadInEXT HitPayload payload;
//hitAttributeEXT vec3 attribs;

void main() {
    payload.objectDescriptorId = gl_InstanceCustomIndexEXT;
    payload.primitiveId = gl_PrimitiveID;
    payload.t = gl_HitTEXT;
}
