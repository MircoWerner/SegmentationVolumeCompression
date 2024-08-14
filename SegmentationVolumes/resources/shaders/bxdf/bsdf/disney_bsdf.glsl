// https://cseweb.ucsd.edu/~tzli/cse272/wi2023/homework1.pdf
#ifndef BSDF_DISNEYA_GLSL
#define BSDF_DISNEYA_GLSL

#include "bsdf.glsl"
#include "disney_diffuse.glsl"
#include "disney_metal.glsl"
#include "disney_sheen.glsl"
#include "disney_clearcoat.glsl"

uint bsdf_disney_lobe_index(in const Material material, in const float rndLobe) {
    const float diffuseWeight = 1.0 - material.metallic;
    const float metalWeight = 1.0;
    const float clearcoatWeight = 0.25 * material.clearcoat;
    const float sumWeights = diffuseWeight + metalWeight + clearcoatWeight;

    if (rndLobe < diffuseWeight / sumWeights) {
        return 0;
    } else if (rndLobe < (diffuseWeight + metalWeight) / sumWeights) {
        return 1;
    } else {
        return 2;
    }
}

vec3 bsdf_disney_evaluate(in const BSDFVertex vertex, in const BSDFFrame frame, in const Material material, in const vec3 dirIn, in const vec3 dirOut) {
    const vec3 fdiffuse = bsdf_disney_diffuse_evaluate(vertex, frame, material, dirIn, dirOut);
    const vec3 fmetal = bsdf_disney_metal_evaluate(vertex, frame, material, dirIn, dirOut);
    const vec3 fsheen = bsdf_disney_sheen_evaluate(vertex, frame, material, dirIn, dirOut);
    const vec3 fclearcoat = bsdf_disney_clearcoat_evaluate(vertex, frame, material, dirIn, dirOut);

    return (1.0) * (1 - material.metallic) * fdiffuse +
    (1 - material.metallic) * material.sheen * fsheen +
    (1.0) * fmetal +
    0.25 * material.clearcoat * fclearcoat;
}

bool bsdf_disney_sample(in const BSDFVertex vertex, in const BSDFFrame frame, in const Material material, in const vec3 dirIn, out vec3 dirOut, in const uint lobe, in const vec2 rndParam) {
    if (lobe == 0) {
        return bsdf_disney_diffuse_sample(vertex, frame, material, dirIn, dirOut, rndParam);
    } else if (lobe == 1) {
        return bsdf_disney_metal_sample(vertex, frame, material, dirIn, dirOut, rndParam);
    } else {
        return bsdf_disney_clearcoat_sample(vertex, frame, material, dirIn, dirOut, rndParam);
    }
}

float bsdf_disney_pdf(in const BSDFVertex vertex, in const BSDFFrame frame, in const Material material, in const vec3 dirIn, in const vec3 dirOut) {
    const float diffusePDF = bsdf_disney_diffuse_pdf(vertex, frame, material, dirIn, dirOut);
    const float metalPDF = bsdf_disney_metal_pdf(vertex, frame, material, dirIn, dirOut);
    const float clearcoatPDF = bsdf_disney_clearcoat_pdf(vertex, frame, material, dirIn, dirOut);

    const float diffuseWeight = 1.0 - material.metallic;
    const float metalWeight = 1.0;
    const float clearcoatWeight = 0.25 * material.clearcoat;
    const float sumWeights = diffuseWeight + metalWeight + clearcoatWeight;

    return (diffuseWeight * diffusePDF + metalWeight * metalPDF + clearcoatWeight * clearcoatPDF) / sumWeights;
}

#endif