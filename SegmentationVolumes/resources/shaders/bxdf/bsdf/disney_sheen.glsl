// https://cseweb.ucsd.edu/~tzli/cse272/wi2023/homework1.pdf
#ifndef BSDF_DISNEY_SHEEN_GLSL
#define BSDF_DISNEY_SHEEN_GLSL

#include "bsdf.glsl"
#include "../../utility/luminance.glsl"

vec3 bsdf_disney_sheen_evaluate(in const BSDFVertex vertex, in BSDFFrame frame, in const Material material, in const vec3 dirIn, in const vec3 dirOut) {
    if (dot(vertex.geometricNormal, dirIn) < 0 ||
    dot(vertex.geometricNormal, dirOut) < 0) {
        return vec3(0);
    }

    if (dot(frame.n, dirIn) < 0) {
        frame.t = -frame.t;
        frame.b = -frame.b;
        frame.n = -frame.n;
    }

    vec3 halfVector = normalize(dirIn + dirOut);

    if (dot(frame.n, dirOut) <= 0 || dot(frame.n, halfVector) <= 0) {
        return vec3(0);
    }

    vec3 baseColor = material.baseColor;
    float baseColorLuminance = luminance(baseColor);
    float sheenTint = material.sheenTint;

    vec3 cTint = vec3(1);
    if (baseColorLuminance > 0) {
        cTint = baseColor / baseColorLuminance;
    }

    vec3 cSheen = (1.0 - sheenTint) + sheenTint * cTint;

    return cSheen * pow(1.0 - abs(dot(halfVector, dirOut)), 5) * abs(dot(frame.n, dirOut));
}

bool bsdf_disney_sheen_sample(in const BSDFVertex vertex, in BSDFFrame frame, in const Material material, in const vec3 dirIn, out vec3 dirOut, in const vec2 rndParam) {
    if (dot(vertex.geometricNormal, dirIn) < 0) {
        return false;
    }

    if (dot(frame.n, dirIn) < 0) {
        frame.t = -frame.t;
        frame.b = -frame.b;
        frame.n = -frame.n;
    }

    dirOut = toWorld(frame, disney_sampleCosHemisphere(rndParam));
    return true;
}

float bsdf_disney_sheen_pdf(in const BSDFVertex vertex, in BSDFFrame frame, in const Material material, in const vec3 dirIn, in const vec3 dirOut) {
    if (dot(vertex.geometricNormal, dirIn) < 0 ||
    dot(vertex.geometricNormal, dirOut) < 0) {
        return 0;
    }

    if (dot(frame.n, dirIn) < 0) {
        frame.t = -frame.t;
        frame.b = -frame.b;
        frame.n = -frame.n;
    }

    return max(dot(frame.n, dirOut), 0) / PI;
}

#endif