#version 460
#extension GL_EXT_ray_tracing: require
#extension GL_GOOGLE_include_directive: enable

#include "../../raycommon.glsl"

layout (location = 0) rayPayloadInEXT HitPayload payload;

void main() {
    payload.t = -1.0;
}