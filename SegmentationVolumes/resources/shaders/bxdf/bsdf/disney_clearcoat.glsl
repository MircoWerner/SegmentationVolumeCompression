// https://cseweb.ucsd.edu/~tzli/cse272/wi2023/homework1.pdf
#ifndef BSDF_DISNEY_CLEARCOAT_GLSL
#define BSDF_DISNEY_CLEARCOAT_GLSL

#include "bsdf.glsl"
#include "../../utility/luminance.glsl"

float clearcoat_mapIndexOfRefraction(in const float eta) {
    return (eta - 1) * (eta - 1) / ((eta + 1) * (eta + 1));
}

float clearcoat_isotropicRoughness(in const float clearcoatGloss) {
    return (1 - clearcoatGloss) * 0.1 + clearcoatGloss * 0.001;
}

float clearcoat_normalDistributionFunction(in const vec3 halfVectorLocal, in const float smoothness) {
    const float smoothnessSq = smoothness * smoothness;
    return (smoothnessSq - 1.0) / (PI * log(smoothnessSq) * (1.0 + (smoothnessSq - 1.0) * halfVectorLocal.z * halfVectorLocal.z));
}

float clearcoat_maskingGTR2(in const vec3 omegaLocal, in const float smoothness) {
    const float smoothnessSq = smoothness * smoothness;
    return 1.0 / (1.0 + (sqrt(1.0 + (omegaLocal.x * omegaLocal.x * smoothnessSq + omegaLocal.y * omegaLocal.y * smoothnessSq) / (omegaLocal.z * omegaLocal.z)) - 1.0) / 2.0);
}

vec3 bsdf_disney_clearcoat_evaluate(in const BSDFVertex vertex, in BSDFFrame frame, in const Material material, in const vec3 dirIn, in const vec3 dirOut) {
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
    vec3 halfVectorLocal = toLocal(frame, halfVector);
    vec3 dirInlocal = toLocal(frame, dirIn);
    vec3 dirOutlocal = toLocal(frame, dirOut);

    if (dot(frame.n, dirOut) <= 0 || dot(frame.n, halfVector) <= 0) {
        return vec3(0);
    }

    const float r0 = clearcoat_mapIndexOfRefraction(1.5);
    const float alphaG = clearcoat_isotropicRoughness(material.clearcoatGloss);

    float fresnel = r0 + (1.0 - r0) * pow(1.0 - abs(dot(halfVector, dirOut)), 5);
    float normalDistrubutionFunction = clearcoat_normalDistributionFunction(halfVectorLocal, alphaG);
    float shadowingMasking = clearcoat_maskingGTR2(dirInlocal, 0.25) * clearcoat_maskingGTR2(dirOutlocal, 0.25);

    return vec3(fresnel * normalDistrubutionFunction * shadowingMasking / (4.0 * abs(dot(frame.n, dirIn))));
}

bool bsdf_disney_clearcoat_sample(in const BSDFVertex vertex, in BSDFFrame frame, in const Material material, in const vec3 dirIn, out vec3 dirOut, in const vec2 rndParam) {
    if (dot(vertex.geometricNormal, dirIn) < 0) {
        return false;
    }

    if (dot(frame.n, dirIn) < 0) {
        frame.t = -frame.t;
        frame.b = -frame.b;
        frame.n = -frame.n;
    }

    const float alphaG = clearcoat_isotropicRoughness(material.clearcoatGloss);
    const float alphaGSq = alphaG * alphaG;

    const float cosHElevation = sqrt(clamp((1.0 - pow(alphaGSq, 1.0 - rndParam.x))/ (1.0 - alphaGSq), 0.0, 1.0));
    const float sinHElevation = sqrt(clamp(1.0 - cosHElevation * cosHElevation, 0.0, 1.0));

    const float hAzimuth = 2.0 * PI * rndParam.y;

    const vec3 localMicroNormal = vec3(sinHElevation * cos(hAzimuth), sinHElevation * sin(hAzimuth), cosHElevation);

    vec3 halfVector = toWorld(frame, localMicroNormal);

    dirOut = reflect(dirIn, halfVector);
    if (dot(frame.n, dirOut) <= 0) {
        dirOut = -dirOut;
    }

    return true;
}

float bsdf_disney_clearcoat_pdf(in const BSDFVertex vertex, in BSDFFrame frame, in const Material material, in const vec3 dirIn, in const vec3 dirOut) {
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
    vec3 halfVectorLocal = toLocal(frame, halfVector);

    if (dot(frame.n, dirOut) <= 0 || dot(frame.n, halfVector) <= 0) {
        return 0;
    }

    const float alphaG = clearcoat_isotropicRoughness(material.clearcoatGloss);

    const float normalDistrubutionFunction = clearcoat_normalDistributionFunction(halfVectorLocal, alphaG);

    return normalDistrubutionFunction * abs(dot(frame.n, halfVector)) / (4.0 * abs(dot(halfVector, dirOut)));
}

#endif