// https://cseweb.ucsd.edu/~tzli/cse272/wi2023/homework1.pdf
#ifndef BSDF_DISNEY_METAL_GLSL
#define BSDF_DISNEY_METAL_GLSL

#include "bsdf.glsl"
#include "../../utility/luminance.glsl"

float metal_mapIndexOfRefraction(const float eta) {
    return (eta - 1) * (eta - 1) / ((eta + 1) * (eta + 1));
}

float metal_smithMaskingGTR2(const vec3 omegaLocal, const float smoothnessX, const float smoothnessY) {
    return 1.0 / (1.0 + (sqrt(1.0 + (omegaLocal.x * omegaLocal.x * smoothnessX * smoothnessX + omegaLocal.y
    * omegaLocal.y * smoothnessY * smoothnessY) / (
    omegaLocal.z * omegaLocal.z)) - 1.0) / 2.0);
}

float metal_GTR2(const vec3 halfVectorLocal, const float smoothnessX, const float smoothnessY) {
    return 1.0 / (PI * smoothnessX * smoothnessY * pow(
    halfVectorLocal.x * halfVectorLocal.x / (smoothnessX * smoothnessX) + halfVectorLocal.y * halfVectorLocal.
    y / (smoothnessY * smoothnessY) + halfVectorLocal.z * halfVectorLocal.z, 2));
}

vec3 bsdf_disney_metal_evaluate(in const BSDFVertex vertex, in BSDFFrame frame, in const Material material, in const vec3 dirIn, in const vec3 dirOut) {
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

    vec3 dirIn_local = toLocal(frame, dirIn);
    vec3 dirOut_local = toLocal(frame, dirOut);
    vec3 halfVectorLocal = toLocal(frame, halfVector);
    vec3 baseColor = material.baseColor;
    float roughness = material.roughness;
    float anisotropic = material.anisotropic;
    float aspect = sqrt(1.0 - float(0.9) * anisotropic);
    float smoothnessMin = float(0.0001);
    float smoothnessX = max(smoothnessMin, roughness * roughness / aspect);
    float smoothnessY = max(smoothnessMin, roughness * roughness * aspect);

    const float baseColorLuminance = luminance(material.baseColor);
    vec3 cTint = vec3(1);
    if (baseColorLuminance > 0) {
        cTint = material.baseColor / baseColorLuminance;
    }
    const vec3 ks = (1 - material.specularTint) + material.specularTint * cTint;
    const vec3 c0 = material.specular * metal_mapIndexOfRefraction(material.eta) * (1 - material.metallic) * ks + material.metallic * material.baseColor;
    const vec3 fresnel = c0 + (1 - c0) * pow(1 - abs(dot(halfVector, dirOut)), 5);

    float normalDistrubutionFunction = metal_GTR2(halfVectorLocal, smoothnessX, smoothnessY);
    float shadowingMasking = metal_smithMaskingGTR2(dirIn_local, smoothnessX, smoothnessY) * metal_smithMaskingGTR2(dirOut_local, smoothnessX, smoothnessY);

    return fresnel * normalDistrubutionFunction * shadowingMasking / (4 * abs(dot(frame.n, dirIn)));
}

bool bsdf_disney_metal_sample(in const BSDFVertex vertex, in BSDFFrame frame, in const Material material, in const vec3 dirIn, out vec3 dirOut, in const vec2 rndParam) {
    if (dot(vertex.geometricNormal, dirIn) < 0) {
        return false;
    }

    if (dot(frame.n, dirIn) < 0) {
        frame.t = -frame.t;
        frame.b = -frame.b;
        frame.n = -frame.n;
    }

    float roughness = material.roughness;
    float anisotropic = material.anisotropic;
    float aspect = sqrt(1.0 - float(0.9) * anisotropic);
    float smoothnessMin = float(0.0001);
    float smoothnessX = max(smoothnessMin, roughness * roughness / aspect);
    float smoothnessY = max(smoothnessMin, roughness * roughness * aspect);

    vec3 local_dirIn = toLocal(frame, dirIn);
    vec3 local_micro_normal = disney_sampleVisibleNormals(local_dirIn, smoothnessX, smoothnessY, rndParam);

    vec3 half_vector = toWorld(frame, local_micro_normal);
    dirOut = normalize(reflect(-dirIn, half_vector));
    if (dot(frame.n, dirOut) < 0) {
        dirOut = -dirOut;
    }

    return true;
}

float bsdf_disney_metal_pdf(in const BSDFVertex vertex, in BSDFFrame frame, in const Material material, in const vec3 dirIn, in const vec3 dirOut) {
    if (dot(vertex.geometricNormal, dirIn) < 0 ||
    dot(vertex.geometricNormal, dirOut) < 0) {
        return 0;
    }

    if (dot(frame.n, dirIn) < 0) {
        frame.t = -frame.t;
        frame.b = -frame.b;
        frame.n = -frame.n;
    }

    vec3 halfVector = normalize(dirIn + dirOut);

    if (dot(frame.n, dirOut) <= 0 || dot(frame.n, halfVector) <= 0) {
        return 0;
    }

    vec3 dirIn_local = toLocal(frame, dirIn);
    vec3 halfVectorLocal = toLocal(frame, halfVector);
    float roughness = material.roughness;
    float anisotropic = material.anisotropic;
    float aspect = sqrt(1.0 - float(0.9) * anisotropic);
    float smoothnessMin = float(0.0001);
    float smoothnessX = max(smoothnessMin, roughness * roughness / aspect);
    float smoothnessY = max(smoothnessMin, roughness * roughness * aspect);

    float G = metal_smithMaskingGTR2(dirIn_local, smoothnessX, smoothnessY);
    float D = metal_GTR2(halfVectorLocal, smoothnessX, smoothnessY);
    return G * D / (4 * abs(dot(frame.n, dirIn)));
}

#endif