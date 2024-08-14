#ifndef ENVIRONMENT_GLSL
#define ENVIRONMENT_GLSL

vec3 environment_evaluate(in const vec3 direction) {
//    if (direction.z > -0.75 && direction.y < 0.0) {
//        return vec3(0);
//    }
//    if (direction.y < 0.0) {
//        return vec3(0);
//    }
////    if (abs(mod(round(direction.x * 10), 2)) <= 0.5) {
////        return vec3(0);
////    }
////    if (abs(mod(round(direction.z * 10), 2)) <= 0.5) {
////        return vec3(0);
////    }
//    if (abs(10 * direction.z - floor(10 * direction.z)) < 0.35) {
//        return vec3(0);
//    }
//    if (abs(10 * direction.x - floor(10 * direction.x)) < 0.35) {
//        return vec3(0);
//    }
    if (g_environment_map == 0) {
        return g_sky_color;
    }
    const vec2 longitudeLatitude = vec2((atan(direction.x, direction.z) / PI + 1.0) * 0.5 + g_environment_map_rotation.x, asin(-direction.y) / PI + 0.5 + g_environment_map_rotation.y);
    return texture(textureEnvironment, longitudeLatitude).rgb;
}

#endif