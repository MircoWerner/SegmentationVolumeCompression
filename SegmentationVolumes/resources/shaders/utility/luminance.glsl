#ifndef UTILITY_LUMINANCE_GLSL
#define UTILITY_LUMINANCE_GLSL

float luminance(in const vec3 s) {
    //return s.x * 0.212671 + s.y * 0.715160 + s.z * 0.072169;
    return dot(s, vec3(0.212671, 0.715160, 0.072169));
}

#endif