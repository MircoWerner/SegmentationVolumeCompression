#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

#include "utility/color.glsl"

vec3 material_getBaseColor(in const vec3 baseColor, in uint labelMetadata) {
    if (g_label_metadata != 0 && g_label_metadata_jitter_albedo != 0) {
        uint rngState = labelMetadata;
        const float u = labelMetadata == 0 ? 0 : 2 * nextFloat(rngState) - 1;
        vec3 hsv = rgb2hsv(baseColor);
        hsv.x = clamp(hsv.x + 0.1 * u, 0, 1);
        //        hsv.y = clamp(hsv.y + 0.2 * u, 0, 1);
        //        hsv.z = clamp(hsv.z + 0.25 * u, 0, 1);
        return hsv2rgb(hsv);
    }
    return baseColor;
}

bool material_isLightSource(in const Material material) {
    return any(greaterThan(material.emission, vec3(0)));
}

#endif