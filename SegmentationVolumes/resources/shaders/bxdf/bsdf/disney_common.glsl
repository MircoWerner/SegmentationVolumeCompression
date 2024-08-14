// https://cseweb.ucsd.edu/~tzli/cse272/wi2023/homework1.pdf
#ifndef BSDF_DISNEY_COMMON_GLSL
#define BSDF_DISNEY_COMMON_GLSL

vec3 disney_sampleCosHemisphere(in const vec2 rndParam) {
    const float phi = TWO_PI * rndParam[0];
    const float tmp = sqrt(clamp(1 - rndParam[1], 0, 1));
    return vec3(cos(phi) * tmp, sin(phi) * tmp, sqrt(clamp(rndParam[1], 0, 1)));
}

/// See "Sampling the GGX Distribution of Visible Normals", Heitz, 2018.
/// https://jcgt.org/published/0007/04/01/
vec3 disney_sampleVisibleNormals(in vec3 localDirIn, in const float alphaX, in const float alphaY, in const vec2 rndParam) {
    bool changeSign = false;
    if (localDirIn.z < 0) {
        changeSign = true;
        localDirIn = -localDirIn;
    }

    const vec3 hemiDirIn = normalize(vec3(alphaX * localDirIn.x, alphaY * localDirIn.y, localDirIn.z));

    const float r = sqrt(rndParam.x);
    const float phi = 2.0 * PI * rndParam.y;
    const float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    const float s = (1.0 + hemiDirIn.z) / 2.0;
    t2 = (1.0 - s) * sqrt(max(0, 1.0 - t1 * t1)) + s * t2;
    const float ti = sqrt(max(0, 1.0 - t1*t1 - t2*t2));

    const BSDFFrame hemiFrame = coordinateSystem(hemiDirIn);

    const vec3 wm = t1 * hemiFrame.t + t2 * hemiFrame.b + ti * hemiFrame.n;

    const vec3 halfVectorLocal = normalize(vec3(alphaX * wm.x, alphaY * wm.y, max(0, wm.z)));

    return changeSign ? -halfVectorLocal : halfVectorLocal;
}

#endif