#ifndef RAYSTRUCTS_GLSL
#define RAYSTRUCTS_GLSL

struct ObjectDescriptor {
    uint64_t aabbAddress; // address to the buffer that contains all AABBs of the object, each object can have its own buffer, but a large shared buffer is possible as well
    uint64_t lodAddress; // address to the buffer that contains all LOD information of the section
};

struct AABB {
    int minX;
    int minY;
    int minZ;
    int maxX;
    int maxY;
    int maxZ;
    uint labelId;
    uint lod;
};

struct SVDAG {
    uint child0;
    uint child1;
    uint child2;
    uint child3;
    uint child4;
    uint child5;
    uint child6;
    uint child7;
};

struct Material {
    vec3 baseColor;
    float specularTransmission;
    float metallic;
    float subsurface;
    float specular;
    float roughness;
    float specularTint;
    float anisotropic;
    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatGloss;
    float eta;

    vec3 emission;
};

#endif