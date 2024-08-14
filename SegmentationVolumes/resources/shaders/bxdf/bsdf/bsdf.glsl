#ifndef BSDF_GLSL
#define BSDF_GLSL

#include "../../utility/constants.glsl"
#include "../../material.glsl"

struct BSDFVertex {
//    vec3 position;
    vec3 geometricNormal;
};

struct BSDFFrame {
    vec3 t;
    vec3 b;
    vec3 n;
};

vec3 toLocal(in const BSDFFrame frame, in const vec3 v) {
    return vec3(dot(v, frame.t), dot(v, frame.b), dot(v, frame.n));
}

vec3 toWorld(in const BSDFFrame frame, in const vec3 v) {
    return frame.t * v[0] + frame.b * v[1] + frame.n * v[2];
}

/// "Building an Orthonormal Basis from a 3D Unit Vector Without Normalization"
/// https://backend.orbit.dtu.dk/ws/portalfiles/portal/126824972/onb_frisvad_jgt2012_v2.pdf
BSDFFrame coordinateSystem(in const vec3 n) {
    BSDFFrame frame;
    frame.n = n;
    if (n[2] < (-1 + 1e-6)) {
        frame.t = vec3(0, -1, 0);
        frame.b = vec3(-1, 0, 0);
    } else {
        float a = 1 / (1 + n[2]);
        float b = -n[0] * n[1] * a;
        frame.t = vec3(1 - n[0] * n[0] * a, b, -n[0]);
        frame.b = vec3(b, 1 - n[1] * n[1] * a, -n[1]);
    }
    return frame;
}

#endif