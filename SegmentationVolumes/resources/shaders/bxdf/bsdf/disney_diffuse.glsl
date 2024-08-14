// https://cseweb.ucsd.edu/~tzli/cse272/wi2023/homework1.pdf
#ifndef BSDF_DISNEY_DIFFUSE_GLSL
#define BSDF_DISNEY_DIFFUSE_GLSL

#include "bsdf.glsl"
#include "disney_common.glsl"

float diffuse_fresnelDiffuse90(in const float roughness, in const vec3 dirOut, in const vec3 halfVector) {
    return 0.5 + 2.0 * roughness * dot(halfVector, dirOut) * dot(halfVector, dirOut);
}

float diffuse_fresnelDiffuse(in const float fd90, in vec3 n, in const vec3 omega) {
    return 1.0 + (fd90 - 1.0) * pow(1.0 - max(dot(n, omega), 0), 5);
}

float diffuse_fresnelSubsurface90(in const float roughness, in const vec3 dirOut, in const vec3 halfVector) {
    return roughness * dot(halfVector, dirOut) * dot(halfVector, dirOut);
}

float diffuse_fresnelSubsurface(in const float fss90, in vec3 n, in const vec3 omega) {
    return 1.0 + (fss90 - 1.0) * pow(1.0 - max(dot(n, omega), 0), 5);
}

vec3 bsdf_disney_diffuse_evaluate(in const BSDFVertex vertex, in BSDFFrame frame, in const Material material, in const vec3 dirIn, in const vec3 dirOut) {
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

    // base diffuse
    float fd90 = diffuse_fresnelDiffuse90(material.roughness, dirOut, halfVector);
    vec3 baseDiffuse = material.baseColor / PI
    * diffuse_fresnelDiffuse(fd90, frame.n, dirIn) * diffuse_fresnelDiffuse(fd90, frame.n, dirOut)
    * max(dot(frame.n, dirOut), 0);

    // subsurface
    float fss90 = diffuse_fresnelSubsurface90(material.roughness, dirOut, halfVector);
    vec3 subsurface = 1.25 * material.baseColor / PI
    * (diffuse_fresnelSubsurface(fss90, frame.n, dirIn) * diffuse_fresnelSubsurface(fss90, frame.n, dirOut) * (1.0 / (max(dot(frame.n, dirIn), 0) + max(dot(frame.n, dirOut), 0)) - 0.5) + 0.5)
    * max(dot(frame.n, dirOut), 0);

    // diffuse
    return (1.0 - material.subsurface) * baseDiffuse + material.subsurface * subsurface;
}

bool bsdf_disney_diffuse_sample(in const BSDFVertex vertex, in BSDFFrame frame, in const Material material, in const vec3 dirIn, out vec3 dirOut, in const vec2 rndParam) {
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

float bsdf_disney_diffuse_pdf(in const BSDFVertex vertex, in BSDFFrame frame, in const Material material, in const vec3 dirIn, in const vec3 dirOut) {
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